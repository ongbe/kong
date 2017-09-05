#ifndef _MARKET_TICK_H
#define _MARKET_TICK_H

#include "quote.h"
#include <assert.h>
#include <string.h>

typedef struct {
	char symbol[QUOTE_SYMBOL_LEN];

	time_t last_time;
	double last_price;
	long last_volume;

	double sell_price;
	long sell_volume;
	double buy_price;
	long buy_volume;

	long open_interest;
	long day_volume;
} tick_t;

static inline
void tick_to_quote(const tick_t *tick, struct quote *quot)
{
	strncpy(quot->symbol, tick->symbol, QUOTE_SYMBOL_LEN);

	quot->begin_time = tick->last_time;
	quot->end_time = tick->last_time;

	quot->open = quot->close = quot->high = quot->low = quot->avg = tick->last_price;
	quot->volume = tick->last_volume;
	quot->open_interest = tick->open_interest;
}

template <class InputIterator>
void ticks_to_quote(InputIterator first, InputIterator last, struct quote *result)
{
	struct quote quot;

	tick_to_quote(&(*first), result);

	while (++first != last) {
		tick_to_quote(&(*first), &quot);
		quote_add(result, &quot);
	}
}

#endif
