#include "plugins/ctp_market_if.h"
#include "conf.h"
#include "datacore.h"
#include <liby/plugin.h>

#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <pthread.h>

static int runflag = 1;
static pthread_t upload_thread;
static std::vector<tick_t> ticktab;
static pthread_mutex_t tick_mutex;

static ctp::market_if *mif;

void on_login_event(ctp::market_if *mif)
{
	time_t now = time(NULL);
	struct tm *tnow = localtime(&now);
	// convert to real year
	int year = tnow->tm_year + 1900;
	// convert to real month [1 ~ 12]
	int mon = tnow->tm_mon + 1;

	std::vector<contract> tab;
	get_contracts(tab);
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

			snprintf(buf, sizeof(buf), "%s%d%02d", iter->symbol,
				 year, atoi(values[i].c_str()));
			mif->subscribe_market_data(buf, 1);
			year = tnow->tm_year + 1900;
		}
	}
}

void on_tick_event(ctp::market_if *mif, const tick_t &tick)
{
	pthread_mutex_lock(&tick_mutex);
	ticktab.push_back(tick);
	pthread_mutex_unlock(&tick_mutex);
}

static void * run_upload_ticks(void *)
{
	while (runflag) {
		if (ticktab.size() == 0) {
			usleep(100 * 1000);
			continue;
		}

		pthread_mutex_lock(&tick_mutex);
		add_ticks(ticktab.begin(), ticktab.end());
		ticktab.clear();
		pthread_mutex_unlock(&tick_mutex);
	}

	return NULL;
}

LIBY_PLUGIN int plugin_init()
{
	// run upload ticks
	pthread_create(&upload_thread, NULL, run_upload_ticks, NULL);
	pthread_mutex_init(&tick_mutex, NULL);

	// run market_if
	mif = new ctp::market_if(conf.get<std::string>("conf.market.market_addr"),
				 conf.get<std::string>("conf.market.broker_id"),
				 conf.get<std::string>("conf.market.username"),
				 conf.get<std::string>("conf.market.password"));
	mif->login_event = on_login_event;
	mif->tick_event = on_tick_event;
	mif->run();

	LOG(INFO) << "plugin init - ctp_market";
	return 0;
}

LIBY_PLUGIN void plugin_exit()
{
	delete mif;
	runflag = 0;
	pthread_join(upload_thread, NULL);
	pthread_mutex_destroy(&tick_mutex);
	LOG(INFO) << "plugin exit - ctp_market";
}
