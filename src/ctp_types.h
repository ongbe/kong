#ifndef _CTP_TYPES_H
#define _CTP_TYPES_H

#include <time.h>

typedef struct __tick_base {
	time_t last_time;
	long   last_volume;
	double last_price;
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

	long sell_volume;
	double sell_price;
	long buy_volume;
	double buy_price;

	char contract_code[31]; // IntrumentID
	char trading_day[11];
	long day_volume;
	long open_interest;
};

#endif
