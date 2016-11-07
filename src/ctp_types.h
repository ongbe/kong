#ifndef _CTP_TYPES_H
#define _CTP_TYPES_H

#include <time.h>

typedef struct __tick_base {
	time_t last_time;
	long   last_volume;
	double last_price;

	long sell_volume;
	double sell_price;
	long buy_volume;
	double buy_price;
} tick_t;

typedef struct __bar_base {
	time_t begin_time;
	time_t end_time;

	long volume;
	double open;
	double close;
	double high;
	double low;
	double avg;
	double wavg;
} bar_t;

struct tick_bar_extra {
	char trading_day[16];
	long day_volume;
	long open_interest;
};

struct futures_tick : public tick_t {
	char contract_code[32];
	struct tick_bar_extra ex;
};

struct futures_bar : public bar_t {
	char contract_code[32];
	struct tick_bar_extra ex;
};

struct futures_contract_base {
	char code[2+1];
	char cn_code[8+1];
	char exchange[32+1];
	char code_format[4+1];
	char main_month[35];
	int byseason;
};

#endif
