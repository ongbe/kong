#include "conf.h"
#include <cstring>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

conf_t conf;

void conf_from_file(conf_t *conf, const char * const filepath)
{
	using std::string;

	boost::property_tree::ptree tree;
	boost::property_tree::xml_parser::read_xml(filepath, tree);

	strncpy(conf->market_addr, tree.get<string>("conf.market.market_addr").c_str(), sizeof(conf->market_addr));
	strncpy(conf->broker_id, tree.get<string>("conf.market.broker_id").c_str(), sizeof(conf->broker_id));
	strncpy(conf->username, tree.get<string>("conf.market.username").c_str(), sizeof(conf->username));
	strncpy(conf->password, tree.get<string>("conf.market.password").c_str(), sizeof(conf->password));

	strncpy(conf->dbfile, tree.get<string>("conf.sqlite.dbfile").c_str(), sizeof(conf->dbfile));
	strncpy(conf->journal_mode, tree.get<string>("conf.sqlite.journal_mode").c_str(), sizeof(conf->journal_mode));
	strncpy(conf->synchronous, tree.get<string>("conf.sqlite.synchronous").c_str(), sizeof(conf->synchronous));
}
