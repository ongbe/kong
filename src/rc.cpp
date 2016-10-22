#include "rc.h"
#include <cstring>
#include <string>
using std::string;
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

void rc_from_file(rc_t *rc, const char * const filepath)
{
	boost::property_tree::ptree tree;
	boost::property_tree::xml_parser::read_xml(filepath, tree);

	strncpy(rc->market_addr, tree.get<string>("conf.market.market_addr").c_str(), sizeof(rc->market_addr));
	strncpy(rc->broker_id, tree.get<string>("conf.market.broker_id").c_str(), sizeof(rc->broker_id));
	strncpy(rc->username, tree.get<string>("conf.market.username").c_str(), sizeof(rc->username));
	strncpy(rc->password, tree.get<string>("conf.market.password").c_str(), sizeof(rc->password));

	int index = 0;
	for (auto &item : tree.get_child("conf.market.contracts")) {
		if (item.first == "<xmlcomment>")
			continue;

		string code = item.second.get<string>("<xmlattr>.name");
		strncpy(rc->contract_codes + index, code.c_str(), sizeof(rc->contract_codes) - index);
		index += code.size();
		rc->contract_codes[index++] = ' ';
	}
	rc->contract_codes[--index] = 0;
}
