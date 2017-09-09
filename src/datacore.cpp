#include "datacore.h"
#include "conf.h"
#include "quote/candlestick.h"
#include <sqlite3.h>
#include <glog/logging.h>
#include <vector>
#include <map>
#include <pthread.h>

typedef candlestick<1> candlestick_type;

static int runflag = 1;
static pthread_t save_thread;
static pthread_cond_t save_cond;
static pthread_mutex_t save_lock;
static std::map<std::string, std::vector<tick_t>> ts;
static pthread_mutex_t ts_lock;

static sqlite3 *db;

static void save_candle(const candlestick_type &candle)
{
	if (!candle.volume) return;

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
			       ticktab.front().last_time / candlestick_type::period ==
			       ticktab.back().last_time / candlestick_type::period) {
				auto cur = find_tick_barrier(ticktab.begin(), ticktab.end(),
							     candlestick_type::period);
				candlestick_type candle;
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

std::vector<contract>& get_contracts()
{
	static std::vector<contract> cons;
	cons.clear();

	char **dbresult;
	int nrow, ncolumn;
	if (SQLITE_OK != sqlite3_get_table
	    (db, "SELECT name, symbol, exchange, byseason,"
	     "symbol_fmt, main_month FROM contract WHERE active = 1",
	     &dbresult, &nrow, &ncolumn, NULL)) {
		LOG(FATAL) << sqlite3_errmsg(db);
		exit(EXIT_FAILURE);
	}

	struct contract con;
	for (int i = 1; i < nrow; i++) {
		snprintf(con.name, sizeof(con.name), "%s", dbresult[i*ncolumn+0]);
		snprintf(con.symbol, sizeof(con.symbol), "%s", dbresult[i*ncolumn+1]);
		snprintf(con.exchange, sizeof(con.exchange), "%s", dbresult[i*ncolumn+2]);
		con.byseason = atoi(dbresult[i*ncolumn+3]);
		snprintf(con.symbol_fmt, sizeof(con.symbol_fmt), "%s", dbresult[i*ncolumn+4]);
		snprintf(con.main_month, sizeof(con.main_month), "%s", dbresult[i*ncolumn+5]);
		cons.push_back(con);
	}

	return cons;
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

	// init save thread
	pthread_create(&save_thread, NULL, run_save_candles, NULL);
	pthread_cond_init(&save_cond, NULL);
	pthread_mutex_init(&save_lock, NULL);
	pthread_mutex_init(&ts_lock, NULL);
}

void datacore_fini()
{
	pthread_mutex_lock(&save_lock);
	runflag = 0;
	pthread_cond_signal(&save_cond);
	pthread_mutex_unlock(&save_lock);
	pthread_join(save_thread, NULL);

	pthread_cond_destroy(&save_cond);
	pthread_mutex_destroy(&save_lock);
	pthread_mutex_destroy(&ts_lock);

	sqlite3_close(db);
}
