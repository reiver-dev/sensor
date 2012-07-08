#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <signal.h>
#include <net/if.h>

#include <unistd.h>     //for fork
#include <sys/stat.h>
#include <sys/wait.h>

#include <sensor.h>

#include "dbrelated.h"
#include "arguments.h"
#include "kvset.h"

sensor_t sensor;
sensor_options_t opts;

int persist_callback(sensor_dissected_t *packet){
	db_execute_statement(
			packet->timestamp,
			packet->mac_source,
			packet->mac_dest,
			packet->content,
			packet->payload,
			packet->payload_length
			);
	return 0;
}

void break_loop() {
	sensor_breakloop(sensor);
}


void detach() {
	pid_t pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);
	if (setsid() < 0) exit(EXIT_FAILURE);

	// closing outputs
	fflush(stdout);
	fflush(stderr);
	fflush(stdin);

	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	close(STDIN_FILENO);

	// redirect stdout to /dev/null
	int devnull = open("log.txt", O_RDWR);
	dup2(devnull, STDOUT_FILENO);
}

void forking() {

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	pid_t pid = fork();
	printf("FORKED:%i\n",pid);
	while (pid) {
		if (pid < 0){
			exit(EXIT_FAILURE);
		}
		if (pid > 0){
			int status;
			waitpid(pid, &status, 0);
			if (WIFEXITED(status)) {
				printf("Child exited\n");
				exit(EXIT_SUCCESS);
			}
			printf("Child crashed\n");
			pid = fork();
		}
	}
	printf("This is child\n");
}

int read_config(struct arguments *arguments) {
	char *filename = arguments->config;
	const char *sections[] = {"main", "capture", "dissection", "persistance", "survey", "balancing", "misc"};
	kvset kvs = InitKVS(6, sections);
	/* main */
	AddKVS(kvs, 0, KVS_STRING, "Interface", &arguments->interface);
	AddKVS(kvs, 0, KVS_BOOL, "Fork", &arguments->enable_fork);
	AddKVS(kvs, 0, KVS_BOOL, "Background", &arguments->enable_background);
	/* capture */
	AddKVS(kvs, 1, KVS_BOOL, "Promiscuous", &arguments->promiscuous);
	AddKVS(kvs, 1, KVS_UINT32, "Buffersize", &arguments->buffersize);
	AddKVS(kvs, 1, KVS_UINT32, "Timeout", &arguments->capture_timeout);
	/* dissection */
	AddKVS(kvs, 2, KVS_UINT32, "Timeout", &arguments->dissection_period);
	/* persistance */
	AddKVS(kvs, 3, KVS_BOOL, "Enable", &arguments->enable_persistance);
	AddKVS(kvs, 3, KVS_STRING, "Host", &arguments->db_host);
	AddKVS(kvs, 3, KVS_UINT32, "Port", &arguments->db_port);
	AddKVS(kvs, 3, KVS_STRING, "Login", &arguments->db_username);
	AddKVS(kvs, 3, KVS_STRING, "Password", &arguments->db_password);
	AddKVS(kvs, 3, KVS_STRING, "Schema", &arguments->db_schema);
	AddKVS(kvs, 3, KVS_STRING, "Table", &arguments->db_table);
	AddKVS(kvs, 3, KVS_UINT32, "Timeout", &arguments->persist_period);

	/* survey */
	AddKVS(kvs, 4, KVS_UINT32, "Timeout", &arguments->survey_timeout);
	AddKVS(kvs, 4, KVS_UINT32, "InitialTimeout", &arguments->survey_initial_timeout);
	AddKVS(kvs, 4, KVS_UINT32, "NodeDisconnectTimeout", &arguments->survey_nodedisconnect_timeout);

	/* balancing */
	AddKVS(kvs, 5, KVS_UINT32, "Timeout", &arguments->balancing_timeout);
	AddKVS(kvs, 5, KVS_UINT32, "InitialTimeout", &arguments->balancing_initial_timeout);
	AddKVS(kvs, 5, KVS_UINT32, "InfoTimeout", &arguments->balancing_info_timeout);
	AddKVS(kvs, 5, KVS_UINT32, "SessionTimeout", &arguments->balancing_session_timeout);
	AddKVS(kvs, 5, KVS_UINT32, "ModifyTimeout", &arguments->balancing_modify_timeout);
	AddKVS(kvs, 5, KVS_UINT32, "LoadCount", &arguments->balancing_load_count);
	AddKVS(kvs, 5, KVS_UINT32, "LoadInterval", &arguments->balancing_interval);
	AddKVS(kvs, 5, KVS_BOOL, "Redirect", &arguments->enable_redirect);
	AddKVS(kvs, 5, KVS_BOOL, "Modify", &arguments->enable_modify);

	LoadKVS(kvs, filename);
	DestroyKVS(kvs);
	return 0;
}

int main(int argc, char** argv) {
	struct arguments arguments = args_get_default();
	args_parse(argc, argv, &arguments);

	if (arguments.config) {
		read_config(&arguments);
	}

	if (arguments.enable_background) {
		detach();
	}

	if (arguments.enable_fork) {
		forking();
	}

	signal(SIGHUP, SIG_DFL);
	signal(SIGINT, break_loop);
	signal(SIGTERM, break_loop);
	signal(SIGQUIT, break_loop);

	sensor = sensor_init();

	if (arguments.enable_persistance) {
		db_init(&arguments);
		db_connect();
		db_prepare_statement();
		sensor_set_persist_callback(sensor, persist_callback);
	} else {
		sensor_set_persist_callback(sensor, 0);
	}


	opts.capture.promiscuous = arguments.promiscuous;
	strncpy(opts.device_name, arguments.interface, IF_NAMESIZE);
	opts.capture.buffersize = arguments.buffersize;

	opts.capture.timeout = arguments.capture_timeout;
	opts.persist.timeout = arguments.persist_period;
	opts.dissect.timeout = arguments.dissection_period;
	opts.balancing.enable_redirect = arguments.enable_redirect;
	opts.balancing.enable_modify = arguments.enable_modify;

	opts.survey.timeout = arguments.survey_timeout;
	opts.survey.initial_timeout = arguments.survey_initial_timeout;
	opts.survey.node_disconnect_timeout = arguments.survey_nodedisconnect_timeout;

	opts.balancing.timeout = arguments.balancing_timeout;
	opts.balancing.info_timeout = arguments.balancing_info_timeout;
	opts.balancing.session_timeout = arguments.balancing_session_timeout;
	opts.balancing.initial_timeout = arguments.balancing_initial_timeout;
	opts.balancing.modify_timeout = arguments.balancing_modify_timeout;
	opts.balancing.load_count = arguments.balancing_load_count;
	opts.balancing.load_interval = arguments.balancing_interval;

	sensor_set_options(sensor, opts);
	sensor_set_dissection_default(sensor);
	sensor_loop(sensor);

	if (arguments.enable_persistance) {
		db_close_statement();
		db_disconnect();
	}

	sensor_destroy(sensor);

	return EXIT_SUCCESS;
}


