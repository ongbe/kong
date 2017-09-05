#ifndef _QUOTE_TICK_H
#define _QUOTE_TICK_H

#include "candlestick.h"
#include <assert.h>
#include <string.h>

typedef struct {
	char symbol[CANDLESTICK_SYMBOL_LEN];

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
void tick_to_candlestick(const tick_t *tick, struct candlestick *candle)
{
	strncpy(candle->symbol, tick->symbol, CANDLESTICK_SYMBOL_LEN);

	candle->begin_time = tick->last_time;
	candle->end_time = tick->last_time;

	candle->open = candle->close = candle->high = candle->low = candle->avg = tick->last_price;
	candle->volume = tick->last_volume;
	candle->open_interest = tick->open_interest;
}

template <class InputIterator>
void ticks_to_candlestick(InputIterator first, InputIterator last, struct candlestick *result)
{
	struct candlestick candle;

	tick_to_candlestick(&(*first), result);

	while (++first != last) {
		tick_to_candlestick(&(*first), &candle);
		candlestick_add(result, &candle);
	}
}

#endif
