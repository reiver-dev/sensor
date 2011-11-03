#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <argp.h>
#include <signal.h>

#include "sensor.h"
#include "sensord.h"
#include "queue.h"

const char * argp_program_version = "Sensor v0.0.1";
const char * argp_program_bug_address = "<Bushev A.S.>";
const uint8_t max_arguments = 12;

const struct argp_option options[] = {
		// main
		{"interface", 'i', "IFACE", 0, "Specify interface to listen (DEFAULT eth0)"},
		{"promisc", 'p', 0, 0, "Specify promiscuous mode for interface (DEFAULT false)"},
		{"timeout", 't', 0, 0, "Specify capture timeout (DEFAULT 0)"},

		// db
		{"host", 'H', "HOSTNAME", 0, "MySQL database host address (DEFAULT 127.0.0.1)"},
		{"username", 'U', "UNAME", 0, "MySQL database username (MANDATORY)"},
		{"password", 'P', "PASS", 0, "MySQL database password (MANDATORY)"},
		{"schema", 'S', "SCHEMA", 0, "MySQL database schema (DEFAULT sniffer)"},
		{"tablename", 'N', "TABLENAME", 0, "MySQL database tablename (DEFAULT sniffer)"},
		{"period", 'P', "MINUTES", 0, "Specify DB persistance period (DEFAULT 2)"},

		// misc
		{"verbose", 'v', 0, 0, "Verbose mode"},
		{ 0 }
};


error_t parse_options(int key, char *arg, struct argp_state *state){
	struct arguments *arguments = state->input;

	switch(key){
	// main
	case 'i':
		arguments->interface = arg;
		break;
	case 'p':
		arguments->promiscuous = true;
		break;
	case 't':
		arguments->timeout = atoi(arg);
		break;

	// for database
	case 'H':
		arguments->db_host = arg;
		break;
	case 'U':
		arguments->db_username = arg;
		break;
	case 'P':
		arguments->db_password = arg;
		break;
	case 'S':
		arguments->db_schema = arg;
		break;
	case 'N':
		arguments->db_table = arg;
		break;
	case 'T':
		arguments->persist_period = atoi(arg);
		break;

	// misc
	case 'v':
		arguments->verbose = true;
		break;

	case ARGP_KEY_ARG:
		if (state->arg_num > max_arguments){
			argp_usage(state);
		}
		arguments->args[state->arg_num-1] = arg;
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;

}

static struct argp args_parser = {options, parse_options};

//TODO: remove this shit
int print_callback(Queue_t *in){
	sensor_dissected_t *packet = queue_pop(in);
	printf("%s\n", packet->content);
	fflush(stdout);
	free(packet->content);
	free(packet->payload);
	free(packet);
	return 0;
}

sensor_t sensor;
void break_loop() {
	sensor_breakloop(&sensor);
}
//---------------------

int main(int argc, char** argv) {
	struct arguments arguments;

	arguments.promiscuous = false;
	arguments.verbose = false;
	arguments.interface = "eth0";
	arguments.db_host = "127.0.0.1";
	arguments.db_schema = "sniffer";
	arguments.db_table = "sniffer";
	arguments.persist_period = 2;
	arguments.db_username = 0;
	arguments.db_password = 0;
	argp_parse(&args_parser, argc, argv, 0, 0, &arguments);

	sensor = sensor_init();
	sensor_set_options(&sensor, arguments.interface, arguments.promiscuous, 65536, 5);
	sensor_set_dissection_simple(&sensor);
	sensor_loop(&sensor, print_callback);


	signal(SIGINT, break_loop);
	signal(SIGQUIT, break_loop);



	return (EXIT_SUCCESS);
}


