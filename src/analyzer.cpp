#include "analyzer.h"
#include "rc.h"
#include <glog/logging.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>

template <class I>
bar_t mkbar(I first, I last)
{
	int index = 0;

	// init bar with first tick
	bar_t bar;
	bar.begin_time = first->last_time;
	bar.end_time = first->last_time;
	bar.open = first->last_price;
	bar.close = first->last_price;
	bar.high = first->last_price;
	bar.low = first->last_price;
	bar.avg = first->last_price;
	bar.wavg = first->last_price * first->last_volume;
	bar.volume = first->last_volume;
	index++;

	// other ticks
	I iter = first;
	++iter;
	for (; iter != last; ++iter) {
		bar.end_time = iter->last_time;
		bar.close = iter->last_price;
		if (bar.high < iter->last_price)
			bar.high = iter->last_price;
		if (bar.low > iter->last_price)
			bar.low = iter->last_price;
		bar.avg += iter->last_price;
		bar.wavg += iter->last_price * iter->last_volume;
		bar.volume += iter->last_volume;
		index++;
	}

	bar.avg /= index;
	bar.wavg /= bar.volume;
	return bar;
}

template <class I>
I find_tick_batch_end(I first, I last, int minutes)
{
	assert(minutes > 0);

	for (I iter = first; iter != last; ++iter) {
		if (iter->last_time / (60*minutes) > first->last_time / (60*minutes))
			return iter;
	}

	return last;
}

namespace ctp {

analyzer::analyzer(): db(NULL)
{
	// init sqlite
	char sql[1024];

	if (SQLITE_OK != sqlite3_open(rc.dbfile, &db)) {
		LOG(FATAL) << sqlite3_errmsg(db);
		exit(EXIT_FAILURE);
	}

	snprintf(sql, sizeof(sql), "PRAGMA journal_mode = %s", rc.journal_mode);
	if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
		LOG(ERROR) << sqlite3_errmsg(db);

	snprintf(sql, sizeof(sql), "PRAGMA synchronous = %s", rc.synchronous);
	if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
		LOG(ERROR) << sqlite3_errmsg(db);

	// query contracts
	char **dbresult;
	int nrow, ncolumn;
	if (SQLITE_OK != sqlite3_get_table(db, "SELECT code, cn_code, exchange, byseason,"
			"code_format, main_month FROM futures_contract_base WHERE active = 1",
			&dbresult, &nrow, &ncolumn, NULL)) {
		LOG(FATAL) << sqlite3_errmsg(db);
		exit(EXIT_FAILURE);
	}

	for (int i = 1; i < nrow; i++) {
		futures_contract_base tra;
		snprintf(tra.code, sizeof(tra.code), "%s", dbresult[i*ncolumn+0]);
		snprintf(tra.cn_code, sizeof(tra.cn_code), "%s", dbresult[i*ncolumn+1]);
		snprintf(tra.exchange, sizeof(tra.exchange), "%s", dbresult[i*ncolumn+2]);
		tra.byseason = atoi(dbresult[i*ncolumn+3]);
		snprintf(tra.code_format, sizeof(tra.code_format), "%s", dbresult[i*ncolumn+4]);
		snprintf(tra.main_month, sizeof(tra.main_month), "%s", dbresult[i*ncolumn+5]);
		contracts.push_back(tra);
	}
}

analyzer::~analyzer()
{
	if (db) {
		sqlite3_close(db);
		db = NULL;
	}
}

std::list<futures_contract_base>&
analyzer::get_contracts()
{
	return contracts;
}

void analyzer::add_tick(struct futures_tick &tick)
{
	// insert tick into futures_tick
	char sql[1024];
	snprintf(sql, sizeof(sql), "INSERT INTO futures_tick"
			"(contract_code, last_time, last_volume, last_price,"
			"sell_volume, sell_price, buy_volume, buy_price,"
			"trading_day, day_volume, open_interest)"
			" VALUES('%s',  %ld, %ld, %lf,  %ld, %lf, %ld, %lf,  '%s', %ld, %ld)",
			tick.contract_code,
			tick.last_time, tick.last_volume, tick.last_price,
			tick.sell_volume, tick.sell_price, tick.buy_volume, tick.buy_price,
			tick.ex.trading_day, tick.ex.day_volume, tick.ex.open_interest);
	if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
		LOG(ERROR) << sqlite3_errmsg(db);

	// add tick to ts
	auto iter = ts.find(tick.contract_code);
	if (iter == ts.end())
		iter = ts.insert(std::make_pair(tick.contract_code, std::list<futures_tick>())).first;
	auto &ticktab = iter->second;
	ticktab.push_back(tick);

	// insert bar into futures_bar_min & minbars
	if (tick.last_time / 60 > ticktab.begin()->last_time / 60) {
		auto cur = find_tick_batch_end(ticktab.begin(), ticktab.end(), 1);
		bar_t bar = mkbar(ticktab.begin(), cur);
		ticktab.erase(ticktab.begin(), cur);
		minbars.push_back(bar);
		snprintf(sql, sizeof(sql), "INSERT INTO futures_bar_min(contract_code,"
				"begin_time, end_time, volume, open, close, high, low, avg, wavg)"
				" VALUES('%s',  %ld, %ld, %ld,  %lf, %lf, %lf, %lf, %.2lf, %.2lf)",
				tick.contract_code,
				bar.begin_time, bar.end_time, bar.volume,
				bar.open, bar.close, bar.high, bar.low, bar.avg, bar.wavg);
		if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
			LOG(ERROR) << sqlite3_errmsg(db);
	}
}

}
