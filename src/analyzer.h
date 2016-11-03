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
	std::map<std::string, std::list<tick_t>> ts;

public:
	analyzer();
	~analyzer();

public:
	std::list<futures_contract_base>& get_contracts();

	void add_tick(struct futures_tick &tick);

	template <class Iterator>
	void add_ticks(Iterator begin, Iterator end)
	{
		for (auto iter = begin; iter != end; ++iter)
			add_tick(*iter);
	}
};

}

#endif
