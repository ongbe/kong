#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "contract_tick.h"
#include <sqlite3.h>
#include <cstdio>

namespace ctp {

class analyzer {
private:
	sqlite3 *db;

public:
	analyzer(): db(NULL)
	{
		sqlite3_open("./ctp.db", &db);
		sqlite3_exec(db, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
		sqlite3_exec(db, "PRAGMA synchronous = NORMAL", NULL, NULL, NULL);
	}

	~analyzer()
	{
		if (db) {
			sqlite3_close(db);
			db = NULL;
		}
	}

public:
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
