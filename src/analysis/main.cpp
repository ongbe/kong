#include "conf.h"
#include "console.h"
#include "utils.h"
#include "quote/candlestick.h"
#include "quote/quote.h"
#include "quote/indicator.h"
#include "datacore/packet.h"
#include <liby/net.h>
#include <liby/packet_parser.h>
#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <map>
#include <algorithm>
#include <math.h>
#include <signal.h>
#include <pthread.h>

typedef quote<std::vector<candlestick_hour>, std::vector<double>> quote_type;
static std::vector<quote_type> quotes;
static pthread_mutex_t qut_lock;

static std::map<int, quebuf_t *> cmdque;
static pthread_mutex_t cmdque_lock;

/*
 * socket
 */

static void on_idle(struct ysock *ys)
{
	// send cmdque
	for (auto &item : cmdque) {
		struct ysock *conn;
		conn = ysock_find_by_fd(item.first);
		if (conn)
			ysock_write(conn, quebuf_rawbuf_out_pos(item.second),
				    quebuf_size(item.second));
		else
			LOG(WARNING) << "can't find connection " << item.first;
		quebuf_delete(item.second);
	}
	cmdque.clear();

	// keep alive
	static time_t alive = time(NULL);

	if (time(NULL) - alive < 60)
		return;

	alive = time(NULL);

	struct packhdr hdr;
	struct pack_alive request;
	hdr.cmd = PACK_ALIVE;
	ysock_write(ys, &hdr, sizeof(hdr));
	ysock_write(ys, &request, sizeof(request));
}

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
	ysock_on_idle(client, on_idle);
	ysock_on_packet(client, on_packet);

	struct packhdr hdr;
	struct pack_query_contracts_request request;
	hdr.cmd = PACK_QUERY_CONTRACTS;
	ysock_write(client, &hdr, sizeof(hdr));
	ysock_write(client, &request, sizeof(request));
}

static int do_parse_alive(const char *buffer, size_t buflen,
			  size_t packlen, void *data)
{
	ysock_rbuf_head((struct ysock *)data, packlen);
	return 0;
}

static int do_parse_query_contracts_response(const char *buffer, size_t buflen,
					     size_t packlen, void *data)
{
	PACK_DATA(response, buffer, struct pack_query_contracts_response);

	if (packlen+response->nr*sizeof(struct contract) > buflen)
		return PACKET_INCOMPLETE;

	ysock_rbuf_head((struct ysock *)data, packlen);

	struct ysock *client = (struct ysock *)data;
	struct packhdr hdr;
	struct pack_query_candles_request request;

	time_t now = time(NULL);
	struct tm *tnow = localtime(&now);
	// convert to real year
	int year = tnow->tm_year + 1900;
	// convert to real month [1 ~ 12]
	int mon = tnow->tm_mon + 1;
	std::vector<std::string> values;

	for (size_t i = 0; i < response->nr; i++) {
		struct contract *con = response->contracts + i;

		hdr.cmd = PACK_QUERY_CANDLES;
		sprintf(request.period, "hour");
		request.begin_time = time(NULL) - 86400;
		request.end_time = time(NULL);

		boost::split(values, con->main_month, boost::is_any_of(" "));
		for (size_t i = 0; i < values.size(); i++) {
			if (mon >= atoi(values[i].c_str()))
				year++;

			if (strcmp(con->symbol_fmt, "Ymm") == 0)
				year %= 10;
			else
				year %= 100;

			snprintf(request.symbol, sizeof(request.symbol), "%s%d%02d",  con->symbol,
				 year, atoi(values[i].c_str()));
			year = tnow->tm_year + 1900;

			// write wbuf
			ysock_write(client, &hdr, sizeof(hdr));
			ysock_write(client, &request, sizeof(request));
			// head rbuf
			ysock_rbuf_head(client, sizeof(*con));
		}
	}

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

		pthread_mutex_lock(&qut_lock);
		auto iter = quotes.begin();
		for (; iter != quotes.end(); ++iter) {
			if (strcmp(candle->symbol, iter->symbol) == 0) {
				iter->add_candle(*candle);
				break;
			}
		}
		if (iter == quotes.end()) {
			quote_type quote(candle->symbol);
			quote.add_candle(*candle);
			quotes.push_back(quote);
		}
		pthread_mutex_unlock(&qut_lock);

		ysock_rbuf_head((struct ysock *)data, sizeof(response->candles[i]));
	}

	struct ysock *client = (struct ysock *)data;
	struct packhdr hdr;
	struct pack_subscribe_request request;
	hdr.cmd = PACK_SUBSCRIBE;
	ysock_write(client, &hdr, sizeof(hdr));
	ysock_write(client, &request, sizeof(request));

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

	pthread_mutex_lock(&qut_lock);
	auto iter = quotes.begin();
	for (; iter != quotes.end(); ++iter) {
		if (strcmp(candle->symbol, iter->symbol) == 0) {
			iter->add_candle(*candle);
			break;
		}
	}
	if (iter == quotes.end()) {
		quote_type quote(candle->symbol);
		quote.add_candle(*candle);
		quotes.push_back(quote);
	}
	pthread_mutex_unlock(&qut_lock);

	return 0;
}

static struct packet_parser pptab[] = {
	PACKET_PARSER_INIT(PACK_ALIVE, PACK_LEN(struct pack_alive), do_parse_alive),
	PACKET_PARSER_INIT(PACK_QUERY_CONTRACTS, PACK_LEN(struct pack_query_contracts_response),
			   do_parse_query_contracts_response),
	PACKET_PARSER_INIT(PACK_QUERY_CANDLES, PACK_LEN(struct pack_query_candles_response),
			   do_parse_query_candles_response),
	PACKET_PARSER_INIT(PACK_SUBSCRIBE, PACK_LEN(struct pack_subscribe_response),
			   do_parse_subscribe_response),
	PACKET_PARSER_INIT(PACK_PUBLISH, PACK_LEN(struct pack_publish), do_parse_publish),
};

/*
 * console
 */

static void on_cmd_quit(const char *line)
{
	raise(SIGINT);
}

static void on_cmd_info(const char *line)
{
	char subcmd[64];
	if (sscanf(line, "info %s", subcmd) != 1) {
		LOG(WARNING) << "info: syntax error";
		return;
	}
	int len = strlen(subcmd) < 10 ? strlen(subcmd) : 10;
	if (strncmp(subcmd, "connection", len) == 0)
		LOG(INFO) << ysock_status();
	else
		LOG(WARNING) << "info: unknown subcmd";
}

static void on_cmd_send(const char *line)
{
	char fd[64];
	char packet[4096];
	char buffer[4096];
	size_t nbyte;

	sscanf(line, "send %s %[^\r\n]", fd, packet);
	nbyte = string_to_bytes(buffer, sizeof(buffer), packet);
	quebuf_t *qb = quebuf_new(nbyte);
	quebuf_write(qb, buffer, nbyte);
	pthread_mutex_lock(&cmdque_lock);
	cmdque.insert(std::make_pair(atoi(fd), qb));
	pthread_mutex_unlock(&cmdque_lock);

}

static void on_cmd_quote(const char *line)
{
	char symbol[64];
	if (sscanf(line, "quote %s", symbol) != 1) {
		LOG(WARNING) << "quote: syntax error";
		return;
	}

	pthread_mutex_lock(&qut_lock);
	for (auto iter = quotes.begin(); iter != quotes.end(); ++iter) {
		if (strcmp(symbol, iter->symbol) == 0) {
			auto qclose = iter->get_close();

			std::vector<double> aa;
			aa.push_back(0);
			for (auto iter = qclose.begin()+1; iter != qclose.end(); ++iter)
				aa.push_back(std::max(*iter - *(iter-1), 0.0));

			std::vector<double> ab;
			ab.push_back(0);
			for (auto iter = qclose.begin()+1; iter != qclose.end(); ++iter)
				ab.push_back(fabs(*iter - *(iter-1)));

			int nr = 1;
			for (auto &item : iter->candles) {
				auto mid = MA(qclose.rend()-nr, qclose.rend(), 23);
				auto md = STD(qclose.rend()-nr, qclose.rend(), 23);
				auto _ac = EMA(aa.rend()-nr, aa.rend(), 23);
				auto _ad = EMA(ab.rend()-nr, ab.rend(), 23);
				auto rsi = 100*_ac;
				if (!_ad)
					rsi = 0;
				else
					rsi /= _ad;
				nr++;

				LOG(INFO) << item
					  << ", ma:" << mid
					  << ", dvt:" << (md == 0 ? 0 : (item.close-mid)/md/2)
					  << ", rsi:" << rsi;
			}
			break;
		}
	}
	pthread_mutex_unlock(&qut_lock);
}

static struct command cmdtab[] = {
	{ "help", on_cmd_help, "display the manual" },
	{ "history", on_cmd_history, "display history of cmdtab" },
	{ "!", on_cmd_history_exec, "!<num>" },
	{ "quit", on_cmd_quit, "stop this server" },
	{ "info", on_cmd_info, "info <subcmd>" },
	{ "send", on_cmd_send, "send packet to client" },
	{ "quote", on_cmd_quote, "print quote information" },
	{ NULL, NULL }
};

static void * start_console(void *arg)
{
	console_init("ana> ");
	for (size_t i = 0; i < sizeof(cmdtab)/sizeof(struct command); i++) {
		if (cmdtab[i].name)
			console_add_command(&cmdtab[i]);
	}
	console_run();
	return NULL;
}

/*
 * main
 */

static void signal_handler(int sig)
{
	if (sig == SIGINT) {
		console_stop();
		ysock_stop();
	}
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
	pthread_mutex_init(&qut_lock, NULL);
	pthread_mutex_init(&cmdque_lock, NULL);

	// run console
	pthread_t console_thread;
	pthread_create(&console_thread, NULL, start_console, NULL);

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

	// stop console
	pthread_join(console_thread, NULL);

	// fini
	pthread_mutex_destroy(&qut_lock);
	pthread_mutex_destroy(&cmdque_lock);
	google::ShutdownGoogleLogging();
	return 0;
}
