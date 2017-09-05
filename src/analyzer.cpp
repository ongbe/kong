#include "analyzer.h"
#include "conf.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <glog/logging.h>

namespace yx {

template <class I>
I find_tick_batch_end(const I &first, const I &last, int minutes)
{
	assert(minutes > 0);

	for (I iter = first; iter != last; ++iter) {
		if (iter->last_time / (60*minutes) > first->last_time / (60*minutes))
			return iter;
	}

	return last;
}

analyzer::analyzer(): db(NULL)
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
	if (SQLITE_OK != sqlite3_get_table(db, "SELECT code, cn_code, exchange, byseason,"
			"code_format, main_month FROM contract_info WHERE active = 1",
			&dbresult, &nrow, &ncolumn, NULL)) {
		LOG(FATAL) << sqlite3_errmsg(db);
		exit(EXIT_FAILURE);
	}

	for (int i = 1; i < nrow; i++) {
		contract_info tra;
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

std::list<contract_info>&
analyzer::get_contracts()
{
	return contracts;
}

void analyzer::add_tick(struct futures_tick &tick)
{
	// find ticktab
	auto iter = ts.find(tick.contract_code);
	if (iter == ts.end())
		iter = ts.insert(std::make_pair(tick.contract_code, std::list<futures_tick>())).first;
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
		auto pre_day_volume = ticktab.rbegin()->ex.day_volume;
		if (tick.ex.day_volume == pre_day_volume)
			return;
		else if (tick.ex.day_volume == 0)
			tick.last_volume = 0;
		else
			tick.last_volume = tick.ex.day_volume - ticktab.rbegin()->ex.day_volume;
	}

	// add tick to ts
	ticktab.push_back(tick);

	// insert bar into minute_detail & minbars
	char sql[1024];
	if (tick.last_time / 60 > ticktab.begin()->last_time / 60) {
		auto cur = find_tick_batch_end(ticktab.begin(), ticktab.end(), 1);
		xbar bar = xbar(ticks_to_bar(ticktab.begin(), cur));
		minbars.push_back(bar);
		ticktab.erase(ticktab.begin(), cur);

		snprintf(sql, sizeof(sql), "INSERT INTO minute_detail(contract_code,"
				"begin_time, end_time, volume, open, close, high, low, avg)"
				" VALUES('%s',  %ld, %ld, %ld,  %lf, %lf, %lf, %lf, %.2lf)",
				tick.contract_code, bar.raw().begin_time, bar.raw().end_time, bar.raw().volume,
				bar.raw().open, bar.raw().close, bar.raw().high, bar.raw().low, bar.raw().avg);
		if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
			LOG(ERROR) << sqlite3_errmsg(db);
	}
}

}
