#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_

#include <stdbool.h>

struct arguments {
	char *config;
	/* main */
	char *interface;
	bool promiscuous;
	int buffersize;
	/* mysql */
	char *db_host, *db_username, *db_password, *db_schema, *db_table;
	int db_port;
	/* periods */
	int capture_timeout;
	int dissection_period;
	int persist_period;
	/* debug */
	bool enable_persistance;
	bool enable_redirect;
	bool enable_fork;
	/* misc */
	bool verbose;
	bool background;

};

struct arguments args_get_default();
int args_parse(int argc, char** argv, struct arguments *arguments);

#endif /* ARGUMENTS_H_ */
