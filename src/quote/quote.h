#ifndef _QUOTE_QUOTE_H
#define _QUOTE_QUOTE_H

#include "quote/contract.h"
#include "quote/candlestick.h"
#include "quote/tick.h"

template<class CONT>
class quote final {
public:
	typedef typename CONT::value_type value_type;

public:
	quote(const char *symbol, size_t max_capacity = 120)
		: max_capacity(max_capacity)
	{
		strncpy(this->symbol, symbol, sizeof(this->symbol));
	}
	~quote() {}

public:
	void add_candle(const value_type &t)
	{
		if (candlestick_period_compare(&candles.back(), &t) == 0)
			candlestick_merge(&candles.back(), &t);
		else
			candles.push_back(t);

		if (candles.size() > max_capacity)
			candles.erase(candles.begin() + (max_capacity/5));
	}

	template<class QUOTET>
	QUOTET* duplicate()
	{
		QUOTET *ret = new QUOTET(symbol);
		typename QUOTET::value_type result, *tmp;

		auto iter = candles.begin();
		candlestick_convert(&(*iter), &result);
		iter++;
		for (; iter != candles.end(); ++iter) {
			tmp = candlestick_convert
				<value_type, typename QUOTET::value_type>
				(&(*iter));
			if (candlestick_period_compare(&result, tmp) == 0) {
				candlestick_merge(&result, tmp);
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
private:
	CONT candles;
	size_t max_capacity;
};

#endif
