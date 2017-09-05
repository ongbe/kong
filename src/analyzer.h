#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "quote/contract.h"
#include "quote/candlestick.h"
#include "quote/tick.h"
#include <string>
#include <list>
#include <map>
#include <sqlite3.h>

namespace kong {

class analyzer {
private:
	sqlite3 *db;
	std::list<contract> contracts;
	std::list<candlestick> candles;
	std::map<std::string, std::list<tick_t>> ts;

public:
	analyzer();
	~analyzer();

public:
	std::list<contract>& get_contracts();

	void add_tick(tick_t &tick);

	template <class InputIterator>
	void add_ticks(InputIterator first, InputIterator last)
	{
		for (auto iter = first; iter != last; ++iter)
			add_tick(*iter);
	}
};

}

#endif
