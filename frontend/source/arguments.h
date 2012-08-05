#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_

#include <stdbool.h>

struct arguments {
	char *config;

	/* main */
	char *interface;
	bool enable_fork;
	bool enable_background;

	/* capture */
	bool promiscuous;
	int buffersize;
	int capture_timeout;

	/* persistance */
	char *head_host, *head_key;
	int head_util_port;
	int head_dump_port;
	int persist_period;
	bool enable_persistance;

	/* survey */
	int survey_timeout;
	int survey_initial_timeout;
	int survey_nodedisconnect_timeout;

	/* balancing */
	int balancing_timeout;
	int balancing_initial_timeout;
	int balancing_info_timeout;
	int balancing_session_timeout;
	int balancing_modify_timeout;
	int balancing_load_count;
	int balancing_interval;
	bool enable_redirect;
	bool enable_modify;

	/* misc */
	bool verbose;


};


struct arguments args_get_default();
int args_parse(int argc, char** argv, struct arguments *arguments);

#endif /* ARGUMENTS_H_ */
