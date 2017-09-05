#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "quote/contract.h"
#include "quote/tick.h"
#include "quote/quote.h"
#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>

namespace kong {

class analyzer {
	typedef candlestick<1> candlestick_type;
	typedef quote<candlestick_type, std::vector<candlestick_type>> quote_type;

public:
	analyzer();
	~analyzer();

public:
	std::vector<contract>& get_contracts();

	void add_tick(tick_t &tick);

	template <class InputIterator>
	void add_ticks(InputIterator first, InputIterator last)
	{
		for (auto iter = first; iter != last; ++iter)
			add_tick(*iter);
	}

private:
	sqlite3 *db;
	std::vector<quote_type> quotes;
	std::map<std::string, std::vector<tick_t>> ts;
};

}

#endif
