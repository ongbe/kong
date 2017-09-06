#include "datacore.h"
#include "conf.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <glog/logging.h>

namespace kong {

datacore::datacore(): db(NULL)
{
	// init sqlite
	char sql[1024];

	if (SQLITE_OK != sqlite3_open(conf.dbfile, &db)) {
		LOG(FATAL) << sqlite3_errmsg(db);
		exit(EXIT_FAILURE);
	}

	snprintf(sql, sizeof(sql), "PRAGMA journal_mode = %s", conf.journal_mode);
	if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
		LOG(ERROR) << sqlite3_errmsg(db);

	snprintf(sql, sizeof(sql), "PRAGMA synchronous = %s", conf.synchronous);
	if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
		LOG(ERROR) << sqlite3_errmsg(db);

	// query contracts
	char **dbresult;
	int nrow, ncolumn;
	if (SQLITE_OK != sqlite3_get_table(db, "SELECT name, symbol, exchange, byseason,"
			"symbol_fmt, main_month FROM contract WHERE active = 1",
			&dbresult, &nrow, &ncolumn, NULL)) {
		LOG(FATAL) << sqlite3_errmsg(db);
		exit(EXIT_FAILURE);
	}

	for (int i = 1; i < nrow; i++) {
		struct contract con;
		snprintf(con.name, sizeof(con.name), "%s", dbresult[i*ncolumn+0]);
		snprintf(con.symbol, sizeof(con.symbol), "%s", dbresult[i*ncolumn+1]);
		snprintf(con.exchange, sizeof(con.exchange), "%s", dbresult[i*ncolumn+2]);
		con.byseason = atoi(dbresult[i*ncolumn+3]);
		snprintf(con.symbol_fmt, sizeof(con.symbol_fmt), "%s", dbresult[i*ncolumn+4]);
		snprintf(con.main_month, sizeof(con.main_month), "%s", dbresult[i*ncolumn+5]);
		quotes.push_back(quote_type(con));
	}
}

datacore::~datacore()
{
	if (db) {
		sqlite3_close(db);
		db = NULL;
	}
}

std::vector<contract>& datacore::get_contracts()
{
	static std::vector<contract> cons;

	cons.clear();
	for (auto &item : quotes)
		cons.push_back(item.con);

	return cons;
}

void datacore::add_tick(tick_t &tick)
{
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

	// add candle
	if (tick.last_time / candlestick_type::period ==
	    ticktab.begin()->last_time / candlestick_type::period)
		return;

	candlestick_type candle;
	auto cur = find_tick_barrier(ticktab.begin(), ticktab.end(), &candle);
	ticks_to_candlestick(ticktab.begin(), cur, &candle);
	ticktab.erase(ticktab.begin(), cur);

	if (!candle.volume)
		return;

	for (auto &item : quotes)
		if (strcpy(candle.symbol, item.con.symbol) == 0)
			item.add_candle(candle);

	// persist candle
	char sql[1024];
	snprintf(sql, sizeof(sql), "INSERT INTO candlestick(symbol, begin_time, end_time,"
		 "open, close, high, low, avg, volume, open_interest)"
		 " VALUES('%s', %ld, %ld, %lf, %lf, %lf, %lf, %.2lf, %ld, %ld)",
		 tick.symbol, candle.begin_time, candle.end_time,
		 candle.open, candle.close, candle.high, candle.low, candle.avg,
		 candle.volume, candle.open_interest);
	if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
		LOG(ERROR) << sqlite3_errmsg(db);
}

}
