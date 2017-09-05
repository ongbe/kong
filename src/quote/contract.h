#ifndef _QUOTE_CONTRACT_H
#define _QUOTE_CONTRACT_H

#define CON_NAME_LEN 16
#define CON_SYMBOL_LEN 16
#define CON_EXCHANGE_LEN 32
#define CON_SYMBOL_FORMAT_LEN 8
#define CON_MAIN_MONTH_LEN 32

struct contract {
	char name[CON_NAME_LEN];
	char symbol[CON_SYMBOL_LEN];
	char exchange[CON_EXCHANGE_LEN];
	char symbol_fmt[CON_SYMBOL_FORMAT_LEN];
	char main_month[CON_MAIN_MONTH_LEN];
	int byseason;
};

#endif
