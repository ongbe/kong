#include "analyzer.h"
#include "rc.h"
#include <glog/logging.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>

template <class Iterator>
bar_t mkbar(Iterator begin, Iterator end)
{
	int index = 0;

	// init bar with first tick
	bar_t bar;
	bar.begin_time = begin->last_time;
	bar.end_time = begin->last_time;
	bar.open = begin->last_price;
	bar.close = begin->last_price;
	bar.high = begin->last_price;
	bar.low = begin->last_price;
	bar.avg = begin->last_price;
	bar.wavg = begin->last_price * begin->last_volume;
	bar.volume = begin->last_volume;
	index++;

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
		bar.avg += iter->last_price;
		bar.wavg += iter->last_price * iter->last_volume;
		bar.volume += iter->last_volume;
		index++;
	}

	bar.avg /= index;
	bar.wavg /= bar.volume;
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

namespace ctp {

analyzer::analyzer(): db(NULL)
{
	char sql[1024];
	if (sqlite3_open(rc.dbfile, &db) == -1) {
		LOG(FATAL) << "open " << rc.dbfile << " failed";
		exit(EXIT_FAILURE);
	}
	snprintf(sql, sizeof(sql), "PRAGMA journal_mode = %s", rc.journal_mode);
	sqlite3_exec(db, sql, NULL, NULL, NULL);
	snprintf(sql, sizeof(sql), "PRAGMA synchronous = %s", rc.synchronous);
	sqlite3_exec(db, sql, NULL, NULL, NULL);

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
			"(contract_code, trading_day, last_time, last_volume, last_price,"
			"sell_price, sell_volume, buy_price, buy_volume, day_volume, open_interest)"
			" VALUES('%s', '%s',  %ld, %ld, %lf,  %lf, %ld, %lf, %ld,  %ld, %ld)",
			tick.contract_code, tick.trading_day,
			tick.t.last_time, tick.t.last_volume, tick.t.last_price,
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
				"begin_time, end_time, volume, open, close, high, low, avg, wavg)"
				" VALUES('%s', '%s',  %ld, %ld,  %ld, %lf, %lf, %lf, %lf, %lf, %lf)",
				tick.contract_code, tick.trading_day,
				bar.begin_time, bar.end_time,
				bar.volume, bar.open, bar.close, bar.high, bar.low, bar.avg, bar.wavg);
		sqlite3_exec(db, sql, NULL, NULL, NULL);
	}
}

}
