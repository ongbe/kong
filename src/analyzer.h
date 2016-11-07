#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "ctp_types.h"
#include <sqlite3.h>
#include <list>
#include <map>
#include <string>

namespace ctp {

class analyzer {
private:
	sqlite3 *db;
	std::list<futures_contract_base> contracts;
	std::list<bar_t> minbars;
	std::map<std::string, std::list<futures_tick>> ts;

public:
	analyzer();
	~analyzer();

public:
	std::list<futures_contract_base>& get_contracts();

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
