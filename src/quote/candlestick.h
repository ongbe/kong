#ifndef _QUOTE_CANDLESTICK_H
#define _QUOTE_CANDLESTICK_H

#include <assert.h>
#include <string.h>
#include <time.h>
#include <iostream>

#define CANDLESTICK_SYMBOL_LEN 16

#define CANDLESTICK_PERIOD_MINUTE 60
#define CANDLESTICK_PERIOD_HOUR (60*60)
#define CANDLESTICK_PERIOD_DAY (60*60*24)
#define CANDLESTICK_PERIOD_WEEK (60*60*24*7)
#define CANDLESTICK_PERIOD_MONTH (60*60*24*30)

static inline
const char * CANDLESTICK_PERIOD_NAME(time_t period)
{
	if (period == CANDLESTICK_PERIOD_MINUTE)
		return "minute";
	else if (period == CANDLESTICK_PERIOD_HOUR)
		return "hour";
	else if (period == CANDLESTICK_PERIOD_DAY)
		return "day";
	else if (period == CANDLESTICK_PERIOD_WEEK)
		return "week";
	else if (period == CANDLESTICK_PERIOD_MONTH)
		return "month";
	else
		return "unknown";
}

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
typedef candlestick<CANDLESTICK_PERIOD_MINUTE/60> candlestick_minute;
typedef candlestick<CANDLESTICK_PERIOD_HOUR/60> candlestick_hour;
typedef candlestick<CANDLESTICK_PERIOD_DAY/60> candlestick_day;
typedef candlestick<CANDLESTICK_PERIOD_WEEK/60> candlestick_week;
typedef candlestick<CANDLESTICK_PERIOD_MONTH/60> candlestick_month;

template<class T>
struct is_candlestick_none_type {
	enum { value = false };
};

template<>
struct is_candlestick_none_type<candlestick_none> {
	enum { value = true };
};

template<class T>
bool is_candlestick_none(const T &t)
{
	return is_candlestick_none_type<T>::value;
}

template<class T>
static inline
bool candlestick_check(const T &t)
{
	if (t.begin_time > t.end_time)
		return false;
	else if (t.begin_time/T::period != t.end_time/T::period)
		return false;
	else
		return true;
}

template<class T>
static inline
int candlestick_period_compare(const T &former, const T &later)
{
	if (former.begin_time/T::period == later.begin_time/T::period)
		return 0;
	else if (former.begin_time/T::period > later.begin_time/T::period)
		return 1;
	else
		return -1;
}

template<unsigned int N1, unsigned int N2>
static inline
struct candlestick<N1> & operator+=(struct candlestick<N1> &former,
				    const struct candlestick<N2> &later)
{
	typedef struct candlestick<N1> T1;
	typedef struct candlestick<N2> T2;

	assert(strncmp(former.symbol, later.symbol, sizeof(former.symbol)) == 0);
	assert(candlestick_check(former));
	assert(candlestick_check(later));
	if (former.end_time > later.begin_time) {
		std::cerr << former << std::endl;
		std::cerr << later << std::endl;
	}
	assert(former.end_time <= later.begin_time);
	assert(T1::period >= T2::period);
	assert(former.begin_time / T1::period == later.begin_time / T1::period);

	former.end_time = later.end_time;
	former.close = later.close;
	if (former.high < later.high)
		former.high = later.high;
	if (former.low > later.low)
		former.low = later.low;
	former.avg = (former.avg * former.volume + later.close * later.volume)
		/ (former.volume + later.volume);
	former.volume += later.volume;
	former.open_interest = later.open_interest;

	return former;
}

template<unsigned int N1, unsigned int N2>
static inline
struct candlestick<N1> operator+(const struct candlestick<N1> &former,
				 const struct candlestick<N2> &later)
{
	struct candlestick<N1> ret(former);
	ret += later;
	return ret;
}

template<unsigned int N>
std::ostream & operator<<(std::ostream &os, const struct candlestick<N> &t)
{
	char btime[32];

	strftime(btime, sizeof(btime), "%Y-%m-%d %H:%M:%S",
		 localtime(&t.begin_time));

	os << "sym:" << t.symbol
	   << ", btm:" << btime
	   << ", per:" << N
	   << ", vol:" << t.volume
	   << ", int:" << t.open_interest
	   << ", close:" << t.close
	   << ", avg:" << t.avg;

	return os;
}

template<class TO, class FROM>
TO * candlestick_convert(FROM *t)
{
	assert((int)TO::period >= (int)FROM::period);
	return reinterpret_cast<TO*>(t);
}

template<class TO, class FROM>
void candlestick_convert(TO *dest, FROM *src)
{
	assert((int)TO::period >= (int)FROM::period);
	*dest = *(reinterpret_cast<TO*>(src));
}

#endif
