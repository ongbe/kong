#ifndef _CONTRACT_TICK_H
#define _CONTRACT_TICK_H

namespace ctp {

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
