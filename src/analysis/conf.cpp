#include "conf.h"
#include <boost/property_tree/xml_parser.hpp>

boost::property_tree::ptree conf;

void conf_init(const char *filename)
{
	boost::property_tree::xml_parser::read_xml(filename, conf);
}
