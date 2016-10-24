#include "rc.h"
#include "market_if.hpp"
#include "analyzer.hpp"
#include <glog/logging.h>
#include <vector>
#include <pthread.h>
using namespace ctp;

static rc_t rc;
static analyzer aly;
static std::vector<contract_tick> ticktab;
static pthread_mutex_t tick_mutex;

void on_tick_event(contract_tick &tick, void *udata)
{
	pthread_mutex_lock(&tick_mutex);
	ticktab.push_back(tick);
	pthread_mutex_unlock(&tick_mutex);
}

void run_analyzer()
{
	while (1) {
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

int main(int argc, char *argv[])
{
	// init
	FLAGS_log_dir = "./log";
	FLAGS_minloglevel = google::INFO;
	FLAGS_stderrthreshold = google::INFO;
	FLAGS_colorlogtostderr = true;
	google::InitGoogleLogging(argv[0]);

	rc_from_file(&rc, "./ctp.xml");
	pthread_mutex_init(&tick_mutex, NULL);

	// run market_if
	market_if *mif = new market_if(rc.market_addr, rc.broker_id,
			rc.username, rc.password, rc.contract_codes);
	mif->tick_event = on_tick_event;

	// run tarder_if

	// run analyzer
	run_analyzer();

	// fini
	pthread_mutex_destroy(&tick_mutex);
	google::ShutdownGoogleLogging();
	return 0;
}
