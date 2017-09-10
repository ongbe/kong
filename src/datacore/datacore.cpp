#include "datacore.h"
#include "conf.h"
#include "packet.h"
#include "quote/candlestick.h"
#include "quote/quote.h"
#include <liby/net.h>
#include <liby/packet_parser.h>
#include <sqlite3.h>
#include <glog/logging.h>
#include <assert.h>
#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <pthread.h>

static int runflag = 1;
static pthread_t save_thread;
static pthread_cond_t save_cond;
static pthread_mutex_t save_lock;

static std::map<std::string, std::vector<tick_t>> ts;
static pthread_mutex_t ts_lock;

typedef quote<candlestick_minute> quote_type;
static std::vector<quote_type> quotes;
static pthread_mutex_t qut_lock;

static sqlite3 *db;

/*
 * persist
 */

static void save_candle(const candlestick_minute &candle)
{
	if (!candle.volume) return;

	// memory
	pthread_mutex_lock(&qut_lock);
	for (auto &item : quotes)
		if (strcmp(candle.symbol, item.con.symbol) == 0)
			item.add_candle(candle);
	pthread_mutex_unlock(&qut_lock);

	// persist
	char sql[1024];
	snprintf(sql, sizeof(sql), "INSERT INTO candlestick(symbol, begin_time, end_time,"
		 "open, close, high, low, avg, volume, open_interest)"
		 " VALUES('%s', %ld, %ld, %lf, %lf, %lf, %lf, %.2lf, %ld, %ld)",
		 candle.symbol, candle.begin_time, candle.end_time,
		 candle.open, candle.close, candle.high, candle.low, candle.avg,
		 candle.volume, candle.open_interest);
	if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
		LOG(ERROR) << sqlite3_errmsg(db);
}

template<class CONT>
size_t get_contracts(CONT &cont)
{
	char **dbresult;
	int nrow, ncolumn;
	if (SQLITE_OK != sqlite3_get_table
	    (db, "SELECT name, symbol, exchange, byseason,"
	     "symbol_fmt, main_month FROM contract WHERE active = 1",
	     &dbresult, &nrow, &ncolumn, NULL)) {
		LOG(FATAL) << sqlite3_errmsg(db);
		return 0;
	}

	typename CONT::value_type con;
	for (int i = 1; i < nrow; i++) {
		snprintf(con.name, sizeof(con.name), "%s", dbresult[i*ncolumn+0]);
		snprintf(con.symbol, sizeof(con.symbol), "%s", dbresult[i*ncolumn+1]);
		snprintf(con.exchange, sizeof(con.exchange), "%s", dbresult[i*ncolumn+2]);
		con.byseason = atoi(dbresult[i*ncolumn+3]);
		snprintf(con.symbol_fmt, sizeof(con.symbol_fmt), "%s", dbresult[i*ncolumn+4]);
		snprintf(con.main_month, sizeof(con.main_month), "%s", dbresult[i*ncolumn+5]);
		cont.push_back(con);
	}

	return cont.size();
}

template<class CONT>
size_t get_candles(const char *symbol, const char *period,
		   time_t begin_time, time_t end_time, CONT &cont)
{
	assert(cont.size() == 0);

	std::stringstream sql("SELECT symbol, begin_time, end_time,"
			      "volume, open_interest, open, close,"
			      "high, low, avg FROM v_candlestick_");
	sql << period;
	sql << " WHERE symbol = '" << symbol << "'";
	sql << " AND begin_time >= " << begin_time;
	sql << " AND end_time < " << end_time;

	char **dbresult;
	int nrow, ncolumn;
	if (SQLITE_OK != sqlite3_get_table(db, sql.str().c_str(), &dbresult,
					   &nrow, &ncolumn, NULL)) {
		LOG(FATAL) << sqlite3_errmsg(db);
		return 0;
	}

	typename CONT::value_type can;
	for (int i = 1; i < nrow; i++) {
		snprintf(can.symbol, sizeof(can.symbol), "%s", dbresult[i*ncolumn+0]);
		can.begin_time = strtoul(dbresult[i*ncolumn+1], NULL, 10);
		can.end_time = strtoul(dbresult[i*ncolumn+2], NULL, 10);
		can.volume = strtol(dbresult[i*ncolumn+3], NULL, 10);
		can.open_interest = strtol(dbresult[i*ncolumn+4], NULL, 10);
		can.open = strtod(dbresult[i*ncolumn+5], NULL);
		can.close = strtod(dbresult[i*ncolumn+6], NULL);
		can.high = strtod(dbresult[i*ncolumn+7], NULL);
		can.low = strtod(dbresult[i*ncolumn+8], NULL);
		can.avg = strtod(dbresult[i*ncolumn+9], NULL);
		cont.push_back(can);
	}

	return cont.size();
}

/*
 * packet_parser
 */

static int do_parse_subscribe(void *sess, char *buffer, size_t len)
{
	return 0;
}

static int do_parse_query_candles(void *sess, char *buffer, size_t len)
{
	if (len < PACK_LEN(struct request_query_candles))
		return 0;

	PACK_DATA(request, buffer, struct request_query_candles);
	std::vector<candlestick_none> candles;
	get_candles(request->symbol, request->period, request->begin_time,
		    request->end_time, candles);

	struct reponse_query_candles response;
	response.nr = candles.size();
	ysock_write((struct ysock *)sess, &response, sizeof(response));
	for (auto &item : candles)
		ysock_write((struct ysock *)sess, &item, sizeof(item));

	ysock_rbuf_head((struct ysock *)sess, PACK_LEN(struct request_query_candles));
	return 0;
}

static struct packet_parser pptab[] = {
	PACKET_PARSER_INIT(PACK_SUBSCRIBE, do_parse_subscribe),
	PACKET_PARSER_INIT(PACK_QUERY_CANDLES, do_parse_query_candles),
};

/*
 * add ticks
 */

static void * run_save_candles(void *)
{
	struct timespec now;

	pthread_mutex_lock(&save_lock);
	while (runflag) {
		clock_gettime(CLOCK_REALTIME, &now);
		now.tv_sec += 60 * 10;
		pthread_cond_timedwait(&save_cond, &save_lock, &now);

		pthread_mutex_lock(&ts_lock);
		for (auto &item : ts) {
			auto &ticktab = item.second;
			while (ticktab.size() &&
			       ticktab.front().last_time / candlestick_minute::period ==
			       ticktab.back().last_time / candlestick_minute::period) {
				auto cur = find_tick_barrier(ticktab.begin(), ticktab.end(),
							     candlestick_minute::period);
				candlestick_minute candle;
				ticks_to_candlestick(ticktab.begin(), cur, &candle);
				ticktab.erase(ticktab.begin(), cur);
				save_candle(candle);
			}
		}
		pthread_mutex_unlock(&ts_lock);
	}
	pthread_mutex_unlock(&save_lock);

	return NULL;
}

void add_tick(tick_t &tick)
{
	pthread_mutex_lock(&ts_lock);

	// find ticktab
	auto iter = ts.find(tick.symbol);
	if (iter == ts.end())
		iter = ts.insert(std::make_pair(tick.symbol, std::vector<tick_t>())).first;
	auto &ticktab = iter->second;

	/*
	 * calculation of last volume
	 * 1) - first tick, set last_volume = 0
	 * 2) - day_volume == pre_day_volume, do nothing
	 * 3) - day_volume == 0, set last_volume = 0
	 * 4) - day_volume != 0, set last_volume = day_volume - pre_day_volume
	 */
	if (ticktab.empty()) {
		tick.last_volume = 0;
	} else {
		auto pre_day_volume = ticktab.rbegin()->day_volume;
		if (tick.day_volume == pre_day_volume)
			return;
		else if (tick.day_volume == 0)
			tick.last_volume = 0;
		else
			tick.last_volume = tick.day_volume - pre_day_volume;
	}

	// add tick
	ticktab.push_back(tick);

	pthread_mutex_unlock(&ts_lock);
}

void datacore_init()
{
	// init sqlite
	char sql[1024];

	if (SQLITE_OK != sqlite3_open(conf.get<std::string>("conf.sqlite.dbfile").c_str(), &db)) {
		LOG(FATAL) << sqlite3_errmsg(db);
		exit(EXIT_FAILURE);
	}

	snprintf(sql, sizeof(sql), "PRAGMA journal_mode = %s",
		 conf.get<std::string>("conf.sqlite.journal_mode").c_str());
	if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
		LOG(ERROR) << sqlite3_errmsg(db);

	snprintf(sql, sizeof(sql), "PRAGMA synchronous = %s",
		 conf.get<std::string>("conf.sqlite.synchronous").c_str());
	if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
		LOG(ERROR) << sqlite3_errmsg(db);

	// init quote
	std::vector<contract> cons;
	get_contracts(cons);
	for (auto &item : cons)
		quotes.push_back(quote_type(item));

	// init save thread
	pthread_create(&save_thread, NULL, run_save_candles, NULL);
	pthread_cond_init(&save_cond, NULL);
	pthread_mutex_init(&save_lock, NULL);
	pthread_mutex_init(&ts_lock, NULL);
	pthread_mutex_init(&qut_lock, NULL);

	// init packet parser
	for (size_t i = 0; i < sizeof(pptab)/sizeof(struct packet_parser); i++)
		register_packet_parser(&pptab[i]);
}

void datacore_fini()
{
	for (size_t i = 0; i < sizeof(pptab)/sizeof(struct packet_parser); i++)
		unregister_packet_parser(&pptab[i]);

	pthread_mutex_lock(&save_lock);
	runflag = 0;
	pthread_cond_signal(&save_cond);
	pthread_mutex_unlock(&save_lock);
	pthread_join(save_thread, NULL);

	pthread_cond_destroy(&save_cond);
	pthread_mutex_destroy(&save_lock);
	pthread_mutex_destroy(&ts_lock);
	pthread_mutex_destroy(&qut_lock);

	sqlite3_close(db);
}