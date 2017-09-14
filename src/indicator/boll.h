#ifndef _INDICATOR_BOLL_H
#define _INDICATOR_BOLL_H

#include <math.h>

template <int N1, int N2 = 2>
struct boll {
	enum { period = N1, width = N2 };

	double close;
	double ma;
	double md;
};

template <class T>
static inline double boll_up(const T &t)
{
	return t.ma + t.md * T::width;
}

template <class T>
static inline double boll_down(const T &t)
{
	return t.ma - t.md * T::width;
}

template <class T>
static inline double boll_deviate(const T &t)
{
	if (t.ma == 0)
		return 0;
	else
		return (t.close - t.ma) / (t.md * T::width);
}

template <class CONT>
void boll_append(double close, CONT &cont)
{
	typedef typename CONT::value_type T;
	T t = {close, 0, 0};
	double sum = 0;
	double sum_square = 0;

	cont.push_back(t);

	if (cont.size() >= T::period) {
		auto iter = cont.rbegin();
		for (int i = 0; i < T::period && iter != cont.rend(); i++, iter++)
			sum += iter->close;
		cont.back().ma = sum / T::period;

		iter = cont.rbegin();
		for (int i = 0; i < T::period && iter != cont.rend(); i++, iter++)
			sum_square += pow(iter->close - cont.back().ma, 2);
		// T::preiod-1 is standard, but it's not my type.
		//cont.back().md = sqrt(sum_square / (T::period-1));
		cont.back().md = sqrt(sum_square / (T::period));
	}
}

template <class CONT>
typename CONT::value_type boll_new(double close, CONT &cont)
{
	typename CONT::value_type ret;

	boll_append(close, cont);
	ret = cont.back();
	cont.pop_back();
	return ret;
}


#endif
