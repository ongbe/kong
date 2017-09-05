#ifndef _QUOTE_CANDLESTICK_H
#define _QUOTE_CANDLESTICK_H

#include <assert.h>
#include <string.h>
#include <time.h>

#define CANDLESTICK_SYMBOL_LEN 16

struct candlestick {
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

static inline
void candlestick_add(struct candlestick *former, const struct candlestick *later)
{
	assert(strncmp(former->symbol, later->symbol, CANDLESTICK_SYMBOL_LEN) == 0);
	assert(former->end_time <= later->begin_time);

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

static inline
struct candlestick candlestick_merge(const struct candlestick *former,
				     const struct candlestick *later)
{
	struct candlestick candlestick = *former;
	candlestick_add(&candlestick, later);
	return candlestick;
}

#endif
