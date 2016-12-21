#ifndef _YX_BAR_BASE_H
#define _YX_BAR_BASE_H

namespace yx {

template <typename T>
class bar_traits {
public:
	typedef typename T::value_type value_type;
};

template <typename T>
class bar_policy {
public:
	typedef typename bar_traits<T>::value_type value_type;

	static value_type merge(const value_type &fst, const value_type &snd)
	{
		return fst + snd;
	}

	static const value_type& value(const T &tp)
	{
		return tp.value;
	}
};

template <typename T,
		  typename Policy = bar_policy<T>,
		  typename Traits = bar_traits<T> >
class bar_base {
public:
	typedef bar_base<T, Policy, Traits> self_type;
	typedef typename Traits::value_type value_type;

	bar_base(const T &rhs) : __vbar(Policy::value(rhs)) {}
	template <class InputIterator>
	bar_base(const InputIterator first, const InputIterator last)
	{
		__vbar = *first;

		InputIterator iter = first;
		++iter;
		for (; iter != last; ++iter)
			__vbar = Policy::merge(__vbar, Policy::value(*iter));
	}

public:
	self_type operator+(T const &tp)
	{
		return self_type(Policy::merge(__vbar, Policy::value(tp)));
	}

	self_type& operator+=(T const &tp)
	{
		__vbar = Policy::merge(__vbar, Policy::value(tp));
		return *this;
	}

	const value_type& raw()
	{
		return __vbar;
	}

private:
	value_type __vbar;
};

}

#endif /* _YX_BAR_BASE */
