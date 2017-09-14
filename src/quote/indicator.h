#ifndef _QUOTE_INDICATOR_H
#define _QUOTE_INDICATOR_H

#include <math.h>
#include <iterator>
#include <algorithm>

#ifdef DEPRECATED_INDICATOR
template<class InputIterator>
static inline
typename std::iterator_traits<InputIterator>::value_type
average(InputIterator first, InputIterator last)
{
	typename std::iterator_traits<InputIterator>::value_type ret = *first++;
	int nr = 1;

	while (first != last) {
		ret += *first++;
		nr++;
	}

	return ret / nr;
}

template<class InputIterator>
static inline
typename std::iterator_traits<InputIterator>::value_type
standard_deviation(InputIterator first, InputIterator last,
		   typename std::iterator_traits<InputIterator>::value_type &val)
{
	typename std::iterator_traits<InputIterator>::value_type ret = *first++;
	int nr = 1;

	while (first != last) {
		ret += pow(*first++ - val, 2);
		nr++;
	}

	return sqrt(ret / nr);
}

template<class DESTCONT, class SRCCONT>
static inline
void MA(DESTCONT &dest, const SRCCONT &src, size_t nr)
{
	typedef typename DESTCONT::value_type value_type;
	assert(nr > 0);
	nr -= 1;

	size_t i = 0;
	auto iter = src.begin();
	for (; iter != src.end(); i++, ++iter) {
		if (i < nr)
			dest.push_back(value_type(0));
		else
			dest.push_back(average(iter-nr, iter+1));
	}
}

template<class DESTCONT, class SRCCONT>
static inline
void STD(DESTCONT &dest, const SRCCONT &src, size_t nr)
{
	typedef typename DESTCONT::value_type value_type;
	assert(nr > 0);
	nr -= 1;

	SRCCONT mid;
	MA(mid, src, nr);

	size_t i = 0;
	auto iter = src.begin();
	auto iter_mid = mid.begin();
	for (; iter != src.end(); i++, ++iter, ++iter_mid) {
		if (i < nr)
			dest.push_back(value_type(0));
		else
			dest.push_back(standard_deviation(iter-nr, iter+1, *iter_mid));
	}
}
#endif

template<class InputIterator>
static inline
typename std::iterator_traits<InputIterator>::value_type
MA(InputIterator first, InputIterator last,
   typename std::iterator_traits<InputIterator>::difference_type nr)
{
	assert(nr > 0);
	typedef typename std::iterator_traits<InputIterator>::value_type value_type;
	typedef typename std::iterator_traits<InputIterator>::difference_type difference_type;

	value_type ret(0);
	difference_type diff = std::count_if(first, last, [] (value_type &) {return true;});

	if (diff < nr)
		return ret;

	for (difference_type i(0); i < nr; ++i, ++first)
		ret += *first;
	return ret / nr;
}

template<class InputIterator>
static inline
typename std::iterator_traits<InputIterator>::value_type
STD(InputIterator first, InputIterator last,
    typename std::iterator_traits<InputIterator>::difference_type nr)
{
	assert(nr != 0);
	typedef typename std::iterator_traits<InputIterator>::value_type value_type;
	typedef typename std::iterator_traits<InputIterator>::difference_type difference_type;

	value_type ret(0);
	value_type ma = MA(first, last, nr);

	if (ma == 0)
		return ret;

	for (difference_type i(0); i < nr; ++i, ++first)
		ret += pow(*first - ma, 2);
	return sqrt(ret / nr);
}

#endif
