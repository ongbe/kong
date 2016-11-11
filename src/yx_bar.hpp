#ifndef _YX_BAR_H
#define _YX_BAR_H

#include "yx_types.h"
#include <assert.h>

namespace yx {

template <class T, class Defs>
class bar_base {
public:
	typedef T value_type;
	typedef typename Defs::tick_type tick_type;
	typedef typename Defs::bar_rep_type bar_rep_type;

	explicit bar_base(const tick_type &rhs) : _bar(static_cast<T*>(this)->get_bar_from_tick(rhs)) {}
	bar_base(const bar_rep_type &rhs) : _bar(rhs) {}
	template <class InputIterator>
	bar_base(const InputIterator first, const InputIterator last)
	{
		value_type v(*first);
		InputIterator iter = first;
		++iter;
		for (; iter != last; ++iter)
			v._bar = static_cast<T*>(this)->merge(v._bar, value_type(*iter)._bar);

		_bar = v._bar;
	}

	value_type operator+(value_type &v)
	{
		return value_type(static_cast<T*>(this)->merge(_bar, v._bar));
	}

	value_type& operator+=(value_type &v)
	{
		_bar = static_cast<T*>(this)->merge(_bar, v._bar);
		return static_cast<T>(*this);
	}

	value_type& operator+=(value_type &&v)
	{
		_bar = static_cast<T*>(this)->merge(_bar, v._bar);
		return static_cast<T>(*this);
	}

	const bar_rep_type& raw()
	{
		return _bar;
	}

protected:
	bar_rep_type _bar;
};

struct xbar_defs {
	typedef tick_t    tick_type;
	typedef bar_t     bar_rep_type;
};

class xbar : public bar_base<xbar, xbar_defs> {
public:
	typedef typename xbar_defs::tick_type tick_type;
	typedef typename xbar_defs::bar_rep_type bar_rep_type;

	explicit xbar(const tick_type &rhs) : bar_base(rhs) {}
	template <class InputIterator>
	xbar(const InputIterator first, const InputIterator last)
		: bar_base(first, last) {}
	xbar(const bar_rep_type &rhs) : bar_base(rhs) {}

	static bar_rep_type get_bar_from_tick(const tick_type &tick)
	{
		bar_rep_type bar;

		bar.begin_time = tick.last_time;
		bar.end_time = tick.last_time;

		bar.volume = tick.last_volume;
		bar.open = bar.close = bar.high = bar.low =
			bar.avg = bar.wavg = tick.last_price;

		return bar;
	}

	static bar_rep_type merge(const bar_rep_type &fst, const bar_rep_type &snd)
	{
		assert(fst.end_time <= snd.begin_time);

		bar_rep_type bar = fst;

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
};

}

#endif
