#include "conf.h"
#include "datacore/packet.h"
#include <liby/net.h>
#include <liby/packet_parser.h>
#include <glog/logging.h>
#include <signal.h>

/*
 * socket
 */

static int on_packet(struct ysock *ys)
{
	const char *buffer;
	size_t len;

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

		// clear & break: do-parse failed
		if (pp->do_parse(buffer, len, pp->len, ys))
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
	request.begin_time = time(NULL) - 86400 * 7;
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
	ysock_rbuf_head((struct ysock *)data, packlen);

	PACK_DATA(response, buffer, struct pack_query_candles_response);
	char timebuf[32];
	for (size_t i = 0; i < response->nr; i++) {
		strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
			 localtime(&response->candles[i].begin_time));
		LOG(INFO) << "sym:" << response->candles[i].symbol
			  << ", btm:" << timebuf
			  << ", vol:" << response->candles[i].volume
			  << ", int:" << response->candles[i].open_interest
			  << ", close:" << response->candles[i].close
			  << ", avg:" << response->candles[i].avg;
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
	char timebuf[32];

	strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
		 localtime(&response->candle.begin_time));
	LOG(INFO) << "sym:" << response->candle.symbol
		  << ", btm:" << timebuf
		  << ", vol:" << response->candle.volume
		  << ", int:" << response->candle.open_interest
		  << ", close:" << response->candle.close
		  << ", avg:" << response->candle.avg;

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
