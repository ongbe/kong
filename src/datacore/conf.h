#ifndef _DATACORE_CONF_H
#define _DATACORE_CONF_H

#include <boost/property_tree/ptree.hpp>

extern boost::property_tree::ptree conf;

void conf_init(const char *filename);

#endif
