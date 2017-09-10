#ifndef _DATACORE_H
#define _DATACORE_H

#include "quote/tick.h"
#include "quote/contract.h"

template<class CONT>
size_t get_contracts(CONT &cont);

template<class CONT>
size_t get_candles(const char *symbol, const char *period,
		   time_t begin_time, time_t end_time, CONT &cont);

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
