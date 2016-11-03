#ifndef _CTP_TYPES_H
#define _CTP_TYPES_H

#include <time.h>

typedef struct __tick_base {
	time_t last_time;
	double last_price;
	long   last_volume;
} tick_t;

typedef struct __bar_base {
	time_t begin_time;
	time_t end_time;

	double open;
	double close;
	double high;
	double low;
	long volume;
} bar_t;

struct futures_contract_base {
	char code[2+1];
	char cn_code[8+1];
	char exchange[32+1];
	char code_format[4+1];
	char main_month[35];
	int byseason;
};

struct futures_tick {
	tick_t t;

	double sell_price;
	long sell_volume;
	double buy_price;
	long buy_volume;

	char contract_code[31]; // IntrumentID
	char trading_day[11];
	long day_volume;
	long open_interest;
};

#endif
