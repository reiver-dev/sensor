#ifndef MAIN_H
#define	MAIN_H

#define MESSAGE_NOTIFY 0
#define MESSAGE_ERROR 1

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct arguments {
	char *args[8];
	char *interface;
	int timeout;
	bool promiscuous;
	char *db_host, *db_username, *db_password, *db_schema, *db_table;
	int persist_period;
	bool verbose;

};

typedef struct capture_s{
	char sourcemac[16];
	char destmac[16];
	int content_len;
	int payload_len;
	uint8_t* content;
	uint8_t* payload;
} captured_t;

void message(int message_type, const char* fmt, ...);

#endif	/* MAIN_H */
