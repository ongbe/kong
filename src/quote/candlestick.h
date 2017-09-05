#ifndef _QUOTE_CANDLESTICK_H
#define _QUOTE_CANDLESTICK_H

#include <assert.h>
#include <string.h>
#include <time.h>

#define CANDLESTICK_SYMBOL_LEN 16

template <unsigned int N>
struct candlestick {
	enum { period = N*60 };

	char symbol[CANDLESTICK_SYMBOL_LEN];

	time_t begin_time;
	time_t end_time;

	double open;
	double close;
	double high;
	double low;
	double avg;

	long volume;
	long open_interest;
};

template <class T>
static inline
bool candlestick_check(const T * const candle)
{
	if (candle->begin_time > candle->end_time)
		return false;
	else if (candle->begin_time/T::period != candle->end_time/T::period)
		return false;
	else
		return true;
}

template <class T>
void candlestick_add(T *former, const T *later)
{
	assert(strncmp(former->symbol, later->symbol, CANDLESTICK_SYMBOL_LEN) == 0);
	assert(candlestick_check(former));
	assert(candlestick_check(later));
	assert(former->end_time <= later->begin_time);
	assert(former->begin_time/T::period == later->end_time/T::period);

	former->end_time = later->end_time;
	former->close = later->close;
	if (former->high < later->high)
		former->high = later->high;
	if (former->low > later->low)
		former->low = later->low;
	former->avg = (former->avg * former->volume + later->close * later->volume)
		/ (former->volume + later->volume);
	former->volume += later->volume;
	former->open_interest = later->open_interest;
}

template <class T>
static inline
T candlestick_merge(const T *former, const T *later)
{
	T ret = *former;
	candlestick_add(&ret, later);
	return ret;
}

#endif
