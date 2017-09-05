#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "market/contract.h"
#include "market/quote.h"
#include "market/tick.h"
#include <string>
#include <list>
#include <map>
#include <sqlite3.h>

namespace kong {

class analyzer {
private:
	sqlite3 *db;
	std::list<contract> contracts;
	std::list<quote> quotes;
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
