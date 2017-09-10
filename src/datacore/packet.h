#ifndef _DATACORE_PACKET_H
#define _DATACORE_PACKET_H

#include "quote/candlestick.h"
#include <stdint.h>
#include <time.h>

#define CMD_LEN (sizeof(struct pack_hdr))
#define PACK_LEN(pack_type) (CMD_LEN + sizeof(pack_type))

#define PACK_HDR(hdrname, ptr) struct pack_hdr *hdrname = (struct pack_hdr*)((char*)ptr)
#define PACK_DATA(dataname, ptr, type) type *dataname = ((type*)((char*)ptr + CMD_LEN))

#define PACK_INIT(packname, hdrname, dataname, type) \
	char packname[CMD_LEN + sizeof(type)]; \
	PACK_HDR(hdrname, packname); \
	PACK_DATA(dataname, packname, type);

struct pack_hdr {
	uint16_t cmd;
	char data[0];
};

#define PACK_ALIVE 0x0181
#define PACK_CANDLE 0x0281
#define PACK_SUBSCRIBE 0x0381
#define PACK_QUERY_CANDLES 0x0382

struct reponse_candle {
	candlestick_none candle;
};

struct request_query_candles {
	// 2017-08-31 or 2017-08-31 12:00:00
	char symbol[CANDLESTICK_SYMBOL_LEN];
	// minute, hour, day, week, month
	char period[7];
	time_t begin_time;
	time_t end_time;
};

struct reponse_query_candles {
	unsigned int nr;
	candlestick_none candle[0];
};

#endif
