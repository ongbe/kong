#include "rc.h"
#include "market_if.hpp"
#include <sqlite3.h>
#include <glog/logging.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace ctp;

static rc_t rc;
static sqlite3 *db;

void on_tick_event(contract_tick &tick, void *udata)
{
	char sql[1024];
	snprintf(sql, sizeof(sql), "INSERT INTO contract_tick"
			"(contract_code, trading_day, time, last_price, last_volume,"
			"sell_price, sell_volume, buy_price, buy_volume, open_volume)"
			" VALUES('%s', '%s', '%s',  %lf, %u, %lf, %u, %lf, %u,  %lf)",
			tick.contract_code, tick.trading_day, tick.last_time,
			tick.last_price, tick.last_day_volume,
			tick.sell_price, tick.sell_volume,
			tick.buy_price, tick.buy_volume,
			tick.open_volume);
	LOG(INFO) << "sql: " << sql;
	int ret = sqlite3_exec(db, sql, NULL, NULL, NULL);
	LOG(INFO) << "sql ret: " << ret;
}

int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);
	FLAGS_log_dir = "./log";
	rc_from_file(&rc, "./ctp.xml");
	sqlite3_open("./ctp.db", &db);

	market_if *mif = new market_if(rc.market_addr, rc.broker_id,
			rc.username, rc.password, rc.contract_codes);
	mif->tick_event = on_tick_event;

	sleep(1000000);
	google::ShutdownGoogleLogging();
	return 0;
}
