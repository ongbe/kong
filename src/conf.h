#ifndef _CONF_H
#define _CONF_H

typedef struct __conf {
	char market_addr[64];
	char broker_id[64];
	char username[64];
	char password[64];

	char dbfile[256];
	char journal_mode[12];
	char synchronous[12];
} conf_t;

void conf_from_file(conf_t *conf, const char * const filepath);
void conf_from_arg(conf_t *conf, int argc, char *argv[]);

extern conf_t conf;

#endif
