#ifndef _QUOTE_QUOTE_H
#define _QUOTE_QUOTE_H

#include "quote/contract.h"
#include "quote/candlestick.h"
#include "quote/tick.h"
#include <vector>

template<class T, class CONT = std::vector<T>>
class quote final {
public:
	typedef T candlestick_type;

public:
	quote(const struct contract &con, size_t max_capacity = 120)
		: con(con), max_capacity(max_capacity) {}
	~quote() {}

public:
	void add_candle(const T &t)
	{
		if (candlestick_period_compare(&candles.back(), &t))
			candlestick_add(&candles.back(), &t);
		else
			candles.push_back(t);

		if (candles.size() > max_capacity)
			candles.erase(candles.begin() + (max_capacity/5));
	}

	template<class QUOTET>
	QUOTET* duplicate()
	{
		QUOTET *ret = new QUOTET(con);
		typename QUOTET::candlestick_type result, *tmp;

		auto iter = candles.begin();
		candlestick_convert(&(*iter), &result);
		iter++;
		for (; iter != candles.end(); ++iter) {
			tmp = candlestick_convert
				<candlestick_type,
				 typename QUOTET::candlestick_type>
				(&(*iter));
			if (candlestick_period_compare(&result, tmp) == 0) {
				candlestick_add(&result, tmp);
			} else {
				ret->add_candle(result);
				result = *tmp;
			}
		}
		ret->add_candle(result);
		return ret;
	}

public:
	struct contract con;
private:
	CONT candles;
	size_t max_capacity;
};

#endif
