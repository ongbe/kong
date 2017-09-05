#ifndef _QUOTE_QUOTE_H
#define _QUOTE_QUOTE_H

#include "quote/contract.h"
#include "quote/candlestick.h"
#include "quote/tick.h"

template <class T, class CONT>
class quote final {
public:
	quote(const struct contract &con)
		: con(con) {}
	~quote() {}

public:
	void add_candle(const T &t)
	{
		if (candlestick_period_compare(&candles.back(), &t))
			candlestick_add(&candles.back(), &t);
		else
			candles.push_back(t);
	}

public:
	struct contract con;
private:
	CONT candles;
};

#endif
