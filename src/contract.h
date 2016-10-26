#ifndef _CONTRACT_H
#define _CONTRACT_H

namespace ctp {

struct contract {
	char code[2+1];
	char cn_code[8+1];
	char exchange[32+1];
	char code_format[4+1];
	char main_month[35];
	int byseason;
};

struct contract_tick {
	char contract_code[31]; // IntrumentID
	char trading_day[11];
	char last_time[32];
	double last_price;
	int last_day_volume;
	double sell_price;
	int sell_volume;
	double buy_price;
	int buy_volume;
	double open_volume;
};

}

#endif
