#ifndef _RC_H
#define _RC_H

typedef struct __rc {
	char market_addr[64];
	char broker_id[64];
	char username[64];
	char password[64];
} rc_t;

void rc_from_file(rc_t *rc, const char * const filepath);
void rc_from_arg(rc_t *rc, int argc, char *argv[]);

#endif
