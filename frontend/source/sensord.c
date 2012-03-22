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
#include <queue.h>

#include "dbrelated.h"
#include "arguments.h"
#include "kvset.h"

sensor_t sensor;
sensor_options_t opts;

int persist_callback(Queue_t in){
	sensor_dissected_t *packet = queue_pop(in);
	db_execute_statement(
			packet->timestamp,
			packet->mac_source,
			packet->mac_dest,
			packet->content,
			packet->payload,
			packet->payload_length
			);

	free(packet);
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
	const char *sections[] = {"main", "capture", "dissection", "persistance", "balancing", "misc"};
	kvset kvs = InitKVS(5, sections);

	AddKVS(kvs, 0, KVS_STRING, "interface", &arguments->interface);
	AddKVS(kvs, 0, KVS_BOOL, "fork", &arguments->enable_fork);
	AddKVS(kvs, 0, KVS_BOOL, "background", &arguments->background);

	AddKVS(kvs, 1, KVS_BOOL, "promiscuous", &arguments->background);
	AddKVS(kvs, 1, KVS_UINT32, "buffersize", &arguments->background);
	AddKVS(kvs, 1, KVS_UINT32, "timeout", &arguments->capture_timeout);

	AddKVS(kvs, 2, KVS_UINT32, "timeout", &arguments->dissection_period);

	AddKVS(kvs, 3, KVS_BOOL, "enable", &arguments->enable_persistance);
	AddKVS(kvs, 3, KVS_STRING, "host", &arguments->db_host);
	AddKVS(kvs, 3, KVS_UINT32, "port", &arguments->db_port);
	AddKVS(kvs, 3, KVS_STRING, "login", &arguments->db_username);
	AddKVS(kvs, 3, KVS_STRING, "password", &arguments->db_password);
	AddKVS(kvs, 3, KVS_STRING, "schema", &arguments->db_schema);
	AddKVS(kvs, 3, KVS_STRING, "table", &arguments->db_table);
	AddKVS(kvs, 3, KVS_UINT32, "timeout", &arguments->persist_period);

	AddKVS(kvs, 4, KVS_UINT32, "timeout", &arguments->balancing_period);
	AddKVS(kvs, 4, KVS_UINT32, "survey_timeout", &arguments->survey_period);

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

	if (arguments.background) {
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

	opts.balancing.survey_timeout = arguments.survey_period;
	opts.balancing.timeout = arguments.balancing_period;

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


