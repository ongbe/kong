#ifndef _QUOTE_CANDLESTICK_H
#define _QUOTE_CANDLESTICK_H

#include <assert.h>
#include <string.h>
#include <time.h>

#define CANDLESTICK_SYMBOL_LEN 16

template<unsigned int N>
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

template<>
struct candlestick<0> {
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

typedef candlestick<0> candlestick_none;
typedef candlestick<1> candlestick_minute;
typedef candlestick<60> candlestick_hour;
typedef candlestick<24*60> candlestick_day;
typedef candlestick<7*24*60> candlestick_week;
typedef candlestick<30*24*60> candlestick_month;

template<class T>
static inline
bool candlestick_check(const T *candle)
{
	if (candle->begin_time > candle->end_time)
		return false;
	else if (candle->begin_time/T::period != candle->end_time/T::period)
		return false;
	else
		return true;
}

template<class T>
static inline
int candlestick_period_compare(const T *former, const T *later)
{
	if (former->begin_time / T::period == later->begin_time / T::period)
		return 0;
	else
		return 1;
}

template<class T>
static inline
void candlestick_add(T *former, const T *later)
{
	assert(strncmp(former->symbol, later->symbol, sizeof(former->symbol)) == 0);
	assert(candlestick_check(former));
	assert(candlestick_check(later));
	assert(former->end_time <= later->begin_time);
	assert(candlestick_period_compare(former, later) == 0);

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

template<class T>
static inline
T candlestick_merge(const T *former, const T *later)
{
	T ret = *former;
	candlestick_add(&ret, later);
	return ret;
}

template<class FROM, class TO>
TO* candlestick_convert(FROM *t)
{
	assert((int)FROM::period < (int)TO::period);
	return reinterpret_cast<TO*>(t);
}

template<class FROM, class TO>
void candlestick_convert(FROM *src, TO *dest)
{
	assert((int)FROM::period < (int)TO::period);
	*dest = *(reinterpret_cast<TO*>(src));
}

#endif
