#ifndef _QUOTE_QUOTE_H
#define _QUOTE_QUOTE_H

#include "quote/contract.h"
#include "quote/candlestick.h"
#include "quote/tick.h"

#define DIRT_OPEN 1
#define DIRT_CLOSE 2
#define DIRT_HIGH 4
#define DIRT_LOW 8
#define DIRT_AVG 16
#define DIRT_VOLUME 32
#define DIRT_OPEN_INTEREST 64

template<class CONT, class CACHECONT>
class quote final {
public:
	typedef typename CONT::value_type value_type;

public:
	quote(const char *symbol, size_t max_capacity = 120)
		: candles_dirt(~0), max_capacity(max_capacity)
	{
		strncpy(this->symbol, symbol, sizeof(this->symbol));
	}
	~quote() {}

public:
	void add_candle(const value_type &t)
	{
		if (candles.size() && candlestick_period_compare(candles.back(), t) == 0)
			candles.back() += t;
		else
			candles.push_back(t);

		if (candles.size() > max_capacity)
			candles.erase(candles.begin() + (max_capacity/5));

		candles_dirt = ~0;
	}

	CACHECONT & get_close()
	{
		if (candles_dirt & DIRT_CLOSE) {
			c_close.clear();
			for (auto &item : candles)
				c_close.push_back(item.close);
			candles_dirt &= ~DIRT_CLOSE;
		}

		return c_close;
	}

	CACHECONT & get_avg()
	{
		if (candles_dirt & DIRT_AVG) {
			c_avg.clear();
			for (auto &item : candles)
				c_avg.push_back(item.avg);
			candles_dirt &= ~DIRT_AVG;
		}

		return c_avg;
	}

	template<class QUOTET>
	QUOTET* duplicate()
	{
		QUOTET *ret = new QUOTET(symbol);
		typename QUOTET::value_type result, *tmp;

		auto iter = candles.begin();
		candlestick_convert(&result, &(*iter));
		iter++;
		for (; iter != candles.end(); ++iter) {
			tmp = candlestick_convert
				<typename QUOTET::value_type, value_type>
				(&(*iter));
			if (candlestick_period_compare(result, *tmp) == 0) {
				result += *tmp;
			} else {
				ret->add_candle(result);
				result = *tmp;
			}
		}
		ret->add_candle(result);
		return ret;
	}

public:
	char symbol[CANDLESTICK_SYMBOL_LEN];
	CONT candles;
private:
	CACHECONT c_close;
	CACHECONT c_avg;
	unsigned char candles_dirt;
	size_t max_capacity;
};

#endif
