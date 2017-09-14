#ifndef _QUOTE_TICK_H
#define _QUOTE_TICK_H

#include "quote/candlestick.h"
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

template<class T>
static inline
void tick_to_candlestick(const tick_t *tick, T *candle)
{
	strncpy(candle->symbol, tick->symbol, CANDLESTICK_SYMBOL_LEN);

	candle->begin_time = tick->last_time;
	candle->end_time = tick->last_time;

	candle->open = candle->close = candle->high = candle->low
		= candle->avg = tick->last_price;
	candle->volume = tick->last_volume;
	candle->open_interest = tick->open_interest;
}

template<class T, class InputIterator>
static inline
void ticks_to_candlestick(InputIterator first, InputIterator last, T *result)
{
	T candle;

	tick_to_candlestick(&(*first), result);

	while (++first != last) {
		tick_to_candlestick(&(*first), &candle);
		*result += candle;
	}
}

template<class InputIterator>
static inline
InputIterator find_tick_barrier(const InputIterator &first,
				const InputIterator &last, int period)
{
	for (auto iter = first; iter != last; ++iter) {
		if (iter->last_time / period > first->last_time / period)
			return iter;
	}

	return last;
}

#endif
