#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "yx/types.h"
#include "yx/xbar.hpp"
#include <string>
#include <list>
#include <map>
#include <sqlite3.h>

namespace yx {

class analyzer {
private:
	sqlite3 *db;
	std::list<contract_info> contracts;
	std::list<xbar> minbars;
	std::map<std::string, std::list<futures_tick>> ts;

public:
	analyzer();
	~analyzer();

public:
	std::list<contract_info>& get_contracts();

	void add_tick(struct futures_tick &tick);

	template <class I>
	void add_ticks(I first, I last)
	{
		for (auto iter = first; iter != last; ++iter)
			add_tick(*iter);
	}
};

}

#endif
