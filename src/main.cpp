#include "rc.h"
#include "market_if.hpp"
#include "analyzer.hpp"
#include <glog/logging.h>
#include <ctime>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <pthread.h>
#include <signal.h>
using namespace ctp;

static int runflag = 1;
static rc_t rc;
static analyzer aly;
static std::vector<contract_tick> ticktab;
static pthread_mutex_t tick_mutex;

void on_login_event(market_if *sender, void *udata)
{
	char buf[256];
	int count = 0;
	int index = 0;

	time_t now = time(NULL);
	struct tm *tnow = localtime(&now);

	std::list<contract> tab = aly.get_contracts();
	for (auto iter = tab.begin(); iter != tab.end(); ++iter) {
		// convert to real year
		int year = tnow->tm_year + 1900;
		// convert to real month [1 ~ 12]
		int mon = tnow->tm_mon + 1;

		std::vector<std::string> values;
		boost::split(values, iter->main_month, boost::is_any_of(" "));

		for (size_t i = 0; i < values.size(); i++) {
			// 2 month after now is perfect
			if (mon + 2 < atoi(values[i].c_str())) {
				mon = atoi(values[i].c_str());
				break;
			}
			// or 1.2 month after now is the deadline
			if (mon + 1 < atoi(values[i].c_str()) && tnow->tm_mday < 24) {
				mon = atoi(values[i].c_str());
				break;
			}
			// if not found, set mon to the first value
			if (i + 1 == values.size()) {
				year++;
				mon = atoi(values[0].c_str());
			}
		}

		if (strcmp(iter->code_format, "YYmm") == 0)
			index += snprintf(buf+index, sizeof(buf)-index, "%s%d%02d ", iter->code, year%100, mon);
		else
			index += snprintf(buf+index, sizeof(buf)-index, "%s%d%02d ", iter->code, year%10, mon);

		count++;
	}

	sender->subscribe_market_data(buf, count);
}

void on_tick_event(market_if *sender, void *udata, contract_tick &tick)
{
	pthread_mutex_lock(&tick_mutex);
	ticktab.push_back(tick);
	pthread_mutex_unlock(&tick_mutex);
}

void run_analyzer()
{
	while (runflag) {
		if (ticktab.size() == 0) {
			usleep(100 * 1000);
			continue;
		}

		pthread_mutex_lock(&tick_mutex);
		aly.add_ticks(ticktab.begin(), ticktab.end());
		ticktab.clear();
		pthread_mutex_unlock(&tick_mutex);
	}
}

void signal_handler(int sig)
{
	runflag = 0;
}

int main(int argc, char *argv[])
{
	// init glog
	FLAGS_log_dir = "./log";
	FLAGS_minloglevel = google::INFO;
	FLAGS_stderrthreshold = google::INFO;
	FLAGS_colorlogtostderr = true;
	google::InitGoogleLogging(argv[0]);

	// init rc
	rc_from_file(&rc, "./ctp.xml");

	// init mutex for ticktab
	pthread_mutex_init(&tick_mutex, NULL);

	// set signals
	signal(SIGINT, signal_handler);

	// run market_if
	market_if *mif = new market_if(rc.market_addr,
			rc.broker_id, rc.username, rc.password);
	mif->login_event = on_login_event;
	mif->tick_event = on_tick_event;
	mif->run();

	// run tarder_if

	// run analyzer
	run_analyzer();

	// fini
	delete mif;
	pthread_mutex_destroy(&tick_mutex);
	google::ShutdownGoogleLogging();
	return 0;
}
