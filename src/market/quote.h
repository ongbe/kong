#ifndef _MARKET_QUOTE_H
#define _MARKET_QUOTE_H

#include <assert.h>
#include <string.h>
#include <time.h>

#define QUOTE_SYMBOL_LEN 16

struct quote {
	char symbol[QUOTE_SYMBOL_LEN];

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
void quote_add(struct quote *former, const struct quote *later)
{
	assert(strncmp(former->symbol, later->symbol, QUOTE_SYMBOL_LEN) == 0);
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
struct quote quote_merge(const struct quote *former,
			 const struct quote *later)
{
	struct quote quote = *former;
	quote_add(&quote, later);
	return quote;
}

#endif
