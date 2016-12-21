#ifndef _YX_XBAR_H
#define _YX_XBAR_H

#include "yx/types.h"
#include "yx/bar_base.hpp"
#include <assert.h>

namespace yx {

template <>
class bar_traits<bar_t> {
public:
	typedef bar_t value_type;
};

template <>
class bar_policy<bar_t> {
public:
	static bar_t merge(const bar_t &fst, const bar_t &snd)
	{
		assert(fst.end_time <= snd.begin_time);

		bar_t bar = fst;

		bar.end_time = snd.end_time;
		bar.close = snd.close;
		if (bar.high < snd.high)
			bar.high = snd.high;
		if (bar.low > snd.low)
			bar.low = snd.low;
		bar.avg = (bar.avg * bar.merge_count + snd.close) / (bar.merge_count + 1);
		bar.wavg = (bar.wavg * bar.volume + snd.close * snd.volume) / (bar.volume + snd.volume);
		bar.volume += snd.volume;
		bar.merge_count++;

		return bar;
	}

	static const bar_t& value(const bar_t &tp)
	{
		return tp;
	}
};

typedef bar_base<bar_t> xbar;

inline bar_t tick_to_bar(const tick_t &tick)
{
	bar_t bar;

	bar.begin_time = tick.last_time;
	bar.end_time = tick.last_time;

	bar.volume = tick.last_volume;
	bar.open = bar.close = bar.high = bar.low =
		bar.avg = bar.wavg = tick.last_price;

	return bar;
}

template <class InputIterator>
inline bar_t ticks_to_bar(const InputIterator first, const InputIterator last)
{
	xbar bar(tick_to_bar(*first));

	InputIterator iter = first;
	++iter;
	for (; iter != last; ++iter)
		bar += tick_to_bar(*iter);

	return bar.raw();
}

}

#endif /* _YX_XBAR_H */
