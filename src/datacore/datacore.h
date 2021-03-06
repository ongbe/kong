#ifndef _DATACORE_DATACORE_H
#define _DATACORE_DATACORE_H

#include "quote/tick.h"
#include "quote/contract.h"
#include <vector>

size_t get_contracts(std::vector<contract> &cont);

void add_tick(tick_t &tick);

template<class InputIterator>
void add_ticks(InputIterator first, InputIterator last)
{
	for (auto iter = first; iter != last; ++iter)
		add_tick(*iter);
}

void datacore_init();
void datacore_fini();

#endif
