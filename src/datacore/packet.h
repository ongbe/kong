#ifndef _DATACORE_PACKET_H
#define _DATACORE_PACKET_H

#include "quote/contract.h"
#include "quote/candlestick.h"
#include <liby/packet.h>
#include <time.h>

#define PACK_QUERY_CONTRACTS 0x0181
#define PACK_QUERY_CANDLES   0x0281
#define PACK_SUBSCRIBE       0x0381
#define PACK_PUBLISH         0x0481

struct pack_query_contracts_request {
	char trash[0];
} __attribute__((packed));

struct pack_query_contracts_response {
	unsigned int nr;
	struct contract contracts[0];
} __attribute__((packed));

struct pack_query_candles_request {
	// 2017-08-31 or 2017-08-31 12:00:00
	char symbol[CANDLESTICK_SYMBOL_LEN];
	time_t period;
	time_t begin_time;
	time_t end_time;
} __attribute__((packed));

struct pack_query_candles_response {
	time_t period;
	unsigned int nr;
	candlestick_none candles[0];
} __attribute__((packed));

struct pack_subscribe_request {
	char trash[0];
} __attribute__((packed));

struct pack_subscribe_response {
	char trash[0];
} __attribute__((packed));

struct pack_publish {
	time_t period;
	candlestick_none candle;
} __attribute__((packed));

#endif
