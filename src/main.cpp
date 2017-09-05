#include "conf.h"
#include "analyzer.h"
#include "ctp/market_if.h"
#include <signal.h>
#include <pthread.h>
#include <ctime>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>

static std::vector<tick_t> ticktab;
static pthread_mutex_t tick_mutex;

static int runflag = 1;
static kong::analyzer *aly;

void on_login_event(ctp::market_if *sender, void *udata)
{
	time_t now = time(NULL);
	struct tm *tnow = localtime(&now);
	// convert to real year
	int year = tnow->tm_year + 1900;
	// convert to real month [1 ~ 12]
	int mon = tnow->tm_mon + 1;

	std::vector<contract> tab = aly->get_contracts();
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

			if (strcmp(iter->symbol_fmt, "Ymm") == 0)
				year %= 10;
			else
				year %= 100;

			snprintf(buf, sizeof(buf), "%s%d%02d", iter->symbol, year, atoi(values[i].c_str()));
			sender->subscribe_market_data(buf, 1);
			year = tnow->tm_year + 1900;
		}
	}
}

void on_tick_event(ctp::market_if *sender, void *udata, tick_t &tick)
{
	pthread_mutex_lock(&tick_mutex);
	ticktab.push_back(tick);
	pthread_mutex_unlock(&tick_mutex);
}

void* run_analyzer(void *)
{
	while (runflag) {
		if (ticktab.size() == 0) {
			usleep(100 * 1000);
			continue;
		}

		pthread_mutex_lock(&tick_mutex);
		aly->add_ticks(ticktab.begin(), ticktab.end());
		ticktab.clear();
		pthread_mutex_unlock(&tick_mutex);
	}

	return NULL;
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

	// init conf
	conf_from_file(&conf, "./kong.xml");

	// set signals
	signal(SIGINT, signal_handler);

	// init mutex for ticktab
	pthread_mutex_init(&tick_mutex, NULL);

	// run analyzer
	aly = new kong::analyzer;
	pthread_t aly_pthread;
	pthread_create(&aly_pthread, NULL, run_analyzer, NULL);

	// run market_if
	ctp::market_if *mif = new ctp::market_if(conf.market_addr,
		conf.broker_id, conf.username, conf.password);
	mif->login_event = on_login_event;
	mif->tick_event = on_tick_event;
	mif->run();

	// run tarder_if

	// wait aly
	pthread_join(aly_pthread, NULL);

	// fini
	delete aly;
	delete mif;
	pthread_mutex_destroy(&tick_mutex);
	google::ShutdownGoogleLogging();
	return 0;
}
