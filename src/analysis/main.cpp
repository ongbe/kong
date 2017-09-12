#include "conf.h"
#include "quote/candlestick.h"
#include "quote/quote.h"
#include "datacore/packet.h"
#include <liby/net.h>
#include <liby/packet_parser.h>
#include <glog/logging.h>
#include <vector>
#include <signal.h>

typedef quote<std::vector<candlestick_hour>> quote_type;
static std::vector<quote_type> quotes;

template<class T>
static void print_candle(const T &t)
{
	char btime[32], etime[32];

	strftime(btime, sizeof(btime), "%Y-%m-%d %H:%M:%S",
		 localtime(&t.begin_time));

	strftime(etime, sizeof(etime), "%Y-%m-%d %H:%M:%S",
		 localtime(&t.end_time));

	LOG(INFO) << "sym:" << t.symbol
		  << ", btm:" << btime
		  << ", etm:" << etime
		  << ", vol:" << t.volume
		  << ", int:" << t.open_interest
		  << ", close:" << t.close
		  << ", avg:" << t.avg;
}

/*
 * socket
 */

static int on_packet(struct ysock *ys)
{
	const char *buffer;
	size_t len;
	int rc;

	while ((len = ysock_rbuf_get(ys, &buffer)) >= PACK_CMD_LEN) {
		struct packet_parser *pp = find_packet_parser(CONV_W(buffer));

		// clear & break: find parser failed
		if (!pp) {
			LOG(WARNING) << "unknown packet: 0x"
				     << std::hex << CONV_W(buffer);
			ysock_rbuf_head(ys, len);
			break;
		}

		// break: pack-len is less than the minial length defined in paser
		if (len < pp->len) break;

		// handle do-parse result
		rc = pp->do_parse(buffer, len, pp->len, ys);
		if (rc == PACKET_INCOMPLETE)
			break;
		else if (rc == PACKET_BROKEN)
			LOG(ERROR) << "broken packet: 0x"
				   << std::hex << CONV_W(buffer);

		// can't determine if do-parse has failed?
		if (ysock_rbuf_get(ys, &buffer) == len) {
			ysock_rbuf_head(ys, pp->len);
			LOG(ERROR) << "packet-parser not head rbuf, packet: 0x"
				   << std::hex << CONV_W(buffer);
		}
	}

	return 0;
}

static void on_connected(struct ysock *client)
{
	ysock_on_packet(client, on_packet);

	struct packhdr hdr;
	struct pack_query_candles_request request;

	hdr.cmd = PACK_QUERY_CANDLES;
	sprintf(request.symbol, "ag1712");
	sprintf(request.period, "hour");
	request.begin_time = time(NULL) - 86400;
	request.end_time = time(NULL);
	ysock_write(client, &hdr, sizeof(hdr));
	ysock_write(client, &request, sizeof(request));

	hdr.cmd = PACK_SUBSCRIBE;
	ysock_write(client, &hdr, sizeof(hdr));
}

static int do_parse_alive(const char *buffer, size_t buflen,
			  size_t packlen, void *data)
{
	ysock_rbuf_head((struct ysock *)data, packlen);
	return 0;
}

static int do_parse_query_candles_response(const char *buffer, size_t buflen,
					   size_t packlen, void *data)
{
	PACK_DATA(response, buffer, struct pack_query_candles_response);

	if (packlen+response->nr*sizeof(candlestick_none) > buflen)
		return PACKET_INCOMPLETE;

	ysock_rbuf_head((struct ysock *)data, packlen);

	for (size_t i = 0; i < response->nr; i++) {
		auto *candle = (candlestick_hour*)(response->candles+i);

		for (auto iter = quotes.begin(); iter != quotes.end(); ++iter) {
			if (strcmp(candle->symbol, iter->symbol) == 0) {
				iter->add_candle(*candle);
				break;
			} else if (iter == quotes.end() - 1) {
				quote_type quote(candle->symbol);
				quote.add_candle(*candle);
				quotes.push_back(quote);
				break;
			}
		}

		ysock_rbuf_head((struct ysock *)data, sizeof(response->candles[i]));
	}

	return 0;
}

static int do_parse_subscribe_response(const char *buffer, size_t buflen,
				       size_t packlen, void *data)
{
	ysock_rbuf_head((struct ysock *)data, packlen);
	return 0;
}

static int do_parse_publish(const char *buffer, size_t buflen,
			    size_t packlen, void *data)
{
	ysock_rbuf_head((struct ysock *)data, packlen);

	PACK_DATA(response, buffer, struct pack_publish);
	auto *candle = (candlestick_hour*)(&response->candle);

	for (auto iter = quotes.begin(); iter != quotes.end(); ++iter) {
		if (strcmp(candle->symbol, iter->symbol) == 0) {
			iter->add_candle(*candle);
			break;
		} else if (iter == quotes.end() - 1) {
			quote_type quote(candle->symbol);
			quote.add_candle(*candle);
			quotes.push_back(quote);
			break;
		}
	}

	return 0;
}

static struct packet_parser pptab[] = {
	PACKET_PARSER_INIT(PACK_ALIVE, PACK_LEN(struct pack_alive), do_parse_alive),
	PACKET_PARSER_INIT(PACK_SUBSCRIBE, PACK_LEN(struct pack_subscribe_response),
			   do_parse_subscribe_response),
	PACKET_PARSER_INIT(PACK_PUBLISH, PACK_LEN(struct pack_publish), do_parse_publish),
	PACKET_PARSER_INIT(PACK_QUERY_CANDLES, PACK_LEN(struct pack_query_candles_response),
			   do_parse_query_candles_response),
};

/*
 * main
 */

static void signal_handler(int sig)
{
	ysock_stop();
}

int main(int argc, char *argv[])
{
	// init
	conf_init("./kong.xml");

	FLAGS_log_dir = conf.get<std::string>("conf.kong.logdir").c_str();
	FLAGS_minloglevel = google::INFO;
	FLAGS_stderrthreshold = google::INFO;
	FLAGS_colorlogtostderr = true;
	google::InitGoogleLogging(argv[0]);

	signal(SIGINT, signal_handler);

	// run server
	for (size_t i = 0; i < sizeof(pptab)/sizeof(struct packet_parser); i++)
		register_packet_parser(&pptab[i]);
	struct ysock client;
	unsigned short port = htons(conf.get<unsigned short>("conf.kong.port"));
	unsigned int ipaddr = inet_addr(conf.get<std::string>("conf.ana.server").c_str());
	if (argc >= 2)
		ipaddr = inet_addr(argv[1]);
	ysock_init(&client);
	if (ysock_connect(&client, port, ipaddr, on_connected) == -1)
		exit(EXIT_FAILURE);
	ysock_exec();
	ysock_close(&client);
	for (size_t i = 0; i < sizeof(pptab)/sizeof(struct packet_parser); i++)
		unregister_packet_parser(&pptab[i]);

	// fini
	google::ShutdownGoogleLogging();
	return 0;
}
