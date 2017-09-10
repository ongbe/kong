#ifndef _DATACORE_PACKET_H
#define _DATACORE_PACKET_H

#include "quote/candlestick.h"
#include <liby/packet.h>
#include <time.h>

#define PACK_SUBSCRIBE 0x0281
#define PACK_PUBLISH PACK_SUBSCRIBE
#define PACK_QUERY_CANDLES 0x0381

struct pack_subscribe {};

struct pack_publish {
	candlestick_none candle;
} __attribute__((packed));

struct pack_query_candles_request {
	// 2017-08-31 or 2017-08-31 12:00:00
	char symbol[CANDLESTICK_SYMBOL_LEN];
	// minute, hour, day, week, month
	char period[7];
	time_t begin_time;
	time_t end_time;
} __attribute__((packed));

struct pack_query_candles_response {
	unsigned int nr;
	candlestick_none candles[0];
} __attribute__((packed));

#endif
