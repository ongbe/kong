#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "ctp_types.h"
#include <sqlite3.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <map>
#include <string>

namespace ctp {

template <class Iterator>
bar_t mkbar(Iterator begin, Iterator end)
{
	// init bar with first tick
	bar_t bar;
	bar.begin_time = begin->last_time;
	bar.end_time = begin->last_time;
	bar.open = begin->last_price;
	bar.close = begin->last_price;
	bar.high = begin->last_price;
	bar.low = begin->last_price;
	bar.volume = begin->last_volume;

	// other ticks
	Iterator iter = begin;
	++iter;
	for (; iter != end; ++iter) {
		bar.end_time = iter->last_time;
		bar.close = iter->last_price;
		if (bar.high < iter->last_price)
			bar.high = iter->last_price;
		if (bar.low > iter->last_price)
			bar.low = iter->last_price;
		bar.volume += iter->last_volume;
	}

	return bar;
}

template <class Iterator>
Iterator find_tick_batch_end(Iterator begin, Iterator end, int minutes)
{
	assert(minutes > 0);

	for (Iterator iter = begin; iter != end; ++iter) {
		if (iter->last_time / (60*minutes) > begin->last_time / (60*minutes))
			return iter;
	}

	return end;
}

class analyzer {
private:
	sqlite3 *db;
	std::list<futures_contract_base> contracts;
	std::list<bar_t> minbars;
	std::map<std::string, std::list<tick_t>> ts;

public:
	analyzer(): db(NULL)
	{
		sqlite3_open("./ctp.db", &db);
		sqlite3_exec(db, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
		sqlite3_exec(db, "PRAGMA synchronous = NORMAL", NULL, NULL, NULL);

		char **dbresult;
		int nrow, ncolumn;
		sqlite3_get_table(db, "SELECT code, cn_code, exchange, byseason,"
				"code_format, main_month FROM futures_contract_base WHERE active = 1",
				&dbresult, &nrow, &ncolumn, NULL);

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

	~analyzer()
	{
		if (db) {
			sqlite3_close(db);
			db = NULL;
		}
	}

public:
	std::list<futures_contract_base>& get_contracts()
	{
		return contracts;
	}

	inline void add_tick(struct futures_tick &tick)
	{
		// insert tick into futures_tick
		char sql[1024];
		snprintf(sql, sizeof(sql), "INSERT INTO futures_tick"
				"(contract_code, trading_day, last_time, last_price, last_volume,"
				"sell_price, sell_volume, buy_price, buy_volume, day_volume, open_interest)"
				" VALUES('%s', '%s',  %ld, %lf, %ld,  %lf, %ld, %lf, %ld,  %ld, %ld)",
				tick.contract_code, tick.trading_day,
				tick.t.last_time, tick.t.last_price, tick.t.last_volume,
				tick.sell_price, tick.sell_volume, tick.buy_price, tick.buy_volume,
				tick.day_volume, tick.open_interest);
		sqlite3_exec(db, sql, NULL, NULL, NULL);

		// add tick to ts
		auto iter = ts.find(tick.contract_code);
		if (iter == ts.end())
			iter = ts.insert(std::make_pair(tick.contract_code, std::list<tick_t>())).first;
		auto &ticktab = iter->second;
		ticktab.push_back(tick.t);

		// insert bar into futures_bar_min & minbars
		if (tick.t.last_time / 60 > ticktab.begin()->last_time / 60) {
			auto cur = find_tick_batch_end(ticktab.begin(), ticktab.end(), 1);
			bar_t bar = mkbar(ticktab.begin(), cur);
			ticktab.erase(ticktab.begin(), cur);
			minbars.push_back(bar);
			snprintf(sql, sizeof(sql), "INSERT INTO futures_bar_min(contract_code, trading_day,"
					"begin_time, end_time, open, close, high, low, volume)"
					" VALUES('%s', '%s',  %ld, %ld,  %lf, %lf, %lf, %lf, %ld)",
					tick.contract_code, tick.trading_day,
					bar.begin_time, bar.end_time,
					bar.open, bar.close, bar.high, bar.low, bar.volume);
			sqlite3_exec(db, sql, NULL, NULL, NULL);
		}
	}

	template <class Iterator>
	void add_ticks(Iterator begin, Iterator end)
	{
		for (auto iter = begin; iter != end; ++iter)
			add_tick(*iter);
	}
};

}

#endif
