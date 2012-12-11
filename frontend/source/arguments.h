#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_

#include <stdbool.h>

struct arguments {
	char *config;

	/* main */
	char *capture_interface;
	bool enable_fork;
	bool enable_background;

	/* capture */
	bool capture_promiscuous;
	int capture_buffersize;
	int capture_timeout;

	/* persistance */
	char *head_host, *head_key;
	int head_util_port;
	int head_dump_port;
	int persist_period;
	bool enable_persistance;

	/* survey */
	int nodes_survey_timeout;
	int nodes_survey_initial_timeout;
	int nodes_disconnect_timeout;
	bool nodes_enable_redirect;
	bool nodes_enable_modify_routing;
	int nodes_modify_routing_timeout;

	/* balancing */
	char *balancing_listen_port;
	int balancing_timeout;
	int balancing_initial_seek_timeout;
	int balancing_info_timeout;
	int balancing_session_timeout;
	int balancing_load_count;
	int balancing_interval;


	/* misc */
	bool verbose;


};


struct arguments args_get_default();
int args_parse(int argc, char** argv, struct arguments *arguments);

#endif /* ARGUMENTS_H_ */
