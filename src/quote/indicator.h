#ifndef _QUOTE_INDICATOR_H
#define _QUOTE_INDICATOR_H

#include <math.h>
#include <iterator>

template<class InputIterator>
double average(InputIterator first, InputIterator last, int nr)
{
	assert(nr > 0 && first != last);

	if (nr == 1 || first+1 == last)
		return *first;

	// weight == 1 / nr
	return (*first + average(++first, last, nr-1)*(nr-1)) / nr;
}

template<class InputIterator>
double weighted_average(InputIterator first, InputIterator last,
			int nr, double weight)
{
	assert(nr > 0 && first != last);

	if (nr == 1 || first+1 == last)
		return *first;

	return weight*(*first) + (1-weight)*weighted_average(++first, last, nr-1, weight);
}

template<class InputIterator>
double variance(InputIterator first, InputIterator last, int nr, double mid)
{
	assert(nr > 0 && first != last);

	if (nr == 1 || first+1 == last)
		return pow(*first-mid, 2);

	return (pow(*first-mid, 2) + variance(++first, last, nr-1, mid)*(nr-1)) / nr;
}

template<class InputIterator>
static inline
typename std::iterator_traits<InputIterator>::value_type
MA(InputIterator first, InputIterator last, int nr)
{
	return average(first, last, nr);
}

template<class InputIterator>
static inline
typename std::iterator_traits<InputIterator>::value_type
EMA(InputIterator first, InputIterator last, int nr)
{
	return weighted_average(first, last, nr, 2.0/(nr+1));
}

template<class InputIterator>
static inline
typename std::iterator_traits<InputIterator>::value_type
STD(InputIterator first, InputIterator last, int nr)
{
	typedef typename std::iterator_traits<InputIterator>::value_type value_type;
	value_type mid = MA(first, last, nr);
	return sqrt(variance(first, last, nr, mid));
}

#endif
