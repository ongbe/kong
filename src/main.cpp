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
static std::vector<futures_tick> ticktab;
static pthread_mutex_t tick_mutex;

void on_login_event(market_if *sender, void *udata)
{
	time_t now = time(NULL);
	struct tm *tnow = localtime(&now);
	// convert to real year
	int year = tnow->tm_year + 1900;
	// convert to real month [1 ~ 12]
	int mon = tnow->tm_mon + 1;

	std::list<futures_contract_base> tab = aly.get_contracts();
	for (auto iter = tab.begin(); iter != tab.end(); ++iter) {
		// not clear how to deal with contracts which affected byseason
		if (!iter->byseason)
			continue;

		char buf[256];
		std::vector<std::string> values;
		boost::split(values, iter->main_month, boost::is_any_of(" "));

		for (size_t i = 0; i < values.size(); i++) {
			if (mon >= atoi(values[i].c_str()))
				year++;

			if (strcmp(iter->code_format, "Ymm") == 0)
				year %= 10;
			else
				year %= 100;

			snprintf(buf, sizeof(buf), "%s%d%02d", iter->code, year, atoi(values[i].c_str()));
			sender->subscribe_market_data(buf, 1);
			year = tnow->tm_year + 1900;
		}
	}
}

void on_tick_event(market_if *sender, void *udata, futures_tick &tick)
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
