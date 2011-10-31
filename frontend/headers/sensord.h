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

void message(int message_type, const char* fmt, ...);

#endif	/* MAIN_H */
