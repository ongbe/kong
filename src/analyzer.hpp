#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "contract.h"
#include <sqlite3.h>
#include <cstdio>
#include <cstdlib>
#include <list>

namespace ctp {

class analyzer {
private:
	sqlite3 *db;
	std::list<contract> contracts;

public:
	analyzer(): db(NULL)
	{
		sqlite3_open("./ctp.db", &db);
		sqlite3_exec(db, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
		sqlite3_exec(db, "PRAGMA synchronous = NORMAL", NULL, NULL, NULL);

		char **dbresult;
		int nrow, ncolumn;
		sqlite3_get_table(db, "SELECT code, cn_code, exchange, byseason,"
				"code_format, main_month FROM contract WHERE active = 1",
				&dbresult, &nrow, &ncolumn, NULL);

		for (int i = 1; i < nrow; i++) {
			contract tra;
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
	std::list<contract>& get_contracts()
	{
		return contracts;
	}

	inline void add_tick(struct contract_tick &tick)
	{
		static char sql[1024];
		snprintf(sql, sizeof(sql), "INSERT INTO contract_tick"
				"(contract_code, trading_day, last_time, last_price, last_volume,"
				"sell_price, sell_volume, buy_price, buy_volume, open_volume)"
				" VALUES('%s', '%s', '%s',  %lf, %u, %lf, %u, %lf, %u,  %lf)",
				tick.contract_code, tick.trading_day, tick.last_time,
				tick.last_price, tick.last_day_volume,
				tick.sell_price, tick.sell_volume,
				tick.buy_price, tick.buy_volume,
				tick.open_volume);
		sqlite3_exec(db, sql, NULL, NULL, NULL);
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
