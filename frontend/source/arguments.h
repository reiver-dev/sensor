#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_

#include <stdbool.h>

struct arguments {
	char *config;

	/* main */
	char *interface;
	bool enable_fork;
	bool background;

	/* capture */
	bool promiscuous;
	int buffersize;
	int capture_timeout;

	/* dissection */
	int dissection_period;

	/* persistance */
	char *db_host, *db_username, *db_password, *db_schema, *db_table;
	int db_port;
	int persist_period;
	bool enable_persistance;

	/* balancing */
	int survey_period;
	int balancing_period;
	int spoof_period;

	/* debug */
	bool enable_redirect;

	/* misc */
	bool verbose;


};

struct arguments args_get_default();
int args_parse(int argc, char** argv, struct arguments *arguments);

#endif /* ARGUMENTS_H_ */
