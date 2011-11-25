#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <argp.h>
#include <signal.h>

#include <sensor.h>

#include "sensord.h"
#include "queue.h"
#include "dbrelated.h"

#define OPT_DISABLE_PERSIST 1
#define OPT_DISABLE_REDIRECT 2

const char * argp_program_version = "Sensor v0.0.1";
const char * argp_program_bug_address = "<Bushev A.S.>";


const struct argp_option options[] = {
		// main
		{"interface", 'i', "IFACE", 0, "Specify interface to listen (DEFAULT eth0)"},
		{"promisc", 'p', 0, 0, "Specify promiscuous mode for interface (DEFAULT false)"},
		{"buffer", 'b', "BYTES", 0, "Specify capture buffer size (DEFAULT 65536)"},

		// persistance
		{"host", 'H', "HOSTNAME", 0, "MySQL database connection host address", 1},
		{"port", 'P', "UNAME", 0, "MySQL database connection port", 1},
		{"username", 'U', "UNAME", 0, "MySQL database username", 1},
		{"password", 'W', "PASS", 0, "MySQL database password", 1},
		{"schema", 'S', "SCHEMA", 0, "MySQL database schema (DEFAULT sniffer)", 1},
		{"tablename", 'T', "TABLENAME", 0, "MySQL database tablename (DEFAULT sniffer)", 1},

		// period
		{"capture-timeout", 't', "SECONDS", 0, "Specify capture timeout (DEFAULT 5)", 2},
		{"dissection-timeout", 'd', "SECONDS", 0, "Specify dissection period (DEFAULT 5)", 2},
		{"writeout-timeout", 'w', "SECONDS", 0, "Specify DB persistance period (DEFAULT 10)", 2},

		// disable
		{"disable-persistance", OPT_DISABLE_PERSIST, 0, 0, "Disable persistance (for debug purposes)", 3},
		{"disable-redirect", OPT_DISABLE_REDIRECT, 0, 0, "Disable redirect (for debug purposes)", 3},

		// misc
		{"verbose", 'v', 0, 0, "Verbose mode", 4},

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
	case 'b':
		arguments->buffersize = atoi(arg);
		break;

	// for database
	case 'H':
		arguments->db_host = arg;
		break;
	case 'P':
		arguments->db_port = atoi(arg);
		break;
	case 'U':
		arguments->db_username = arg;
		break;
	case 'W':
		arguments->db_password = arg;
		break;
	case 'S':
		arguments->db_schema = arg;
		break;
	case 'T':
		arguments->db_table = arg;
		break;


	case 't':
		arguments->capture_timeout = atoi(arg);
		break;

	case 'd':
		arguments->dissection_period = atoi(arg);
		break;

	case 'w':
		arguments->persist_period = atoi(arg);
		break;


	case OPT_DISABLE_PERSIST:
		arguments->enable_persistance = false;
		break;

	case OPT_DISABLE_REDIRECT:
		arguments->enable_redirect = false;
		break;

	// misc
	case 'v':
		arguments->verbose = true;
		break;
/*
	case ARGP_KEY_ARG:
		arguments->args[state->arg_num-1] = arg;
		break;*/

	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;

}

static struct argp args_parser = {options, parse_options};

//-----------------------------------------------------

sensor_t sensor;
sensor_options_t opts;


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

int persist_callback(Queue_t *in){
	sensor_dissected_t *packet = queue_pop(in);
	db_execute_statement(
			packet->timestamp,
			packet->mac_source,
			packet->mac_dest,
			packet->content,
			packet->payload,
			packet->payload_length
			);

	free(packet->content);
	free(packet->payload);
	free(packet);
	return 0;
}

void break_loop() {
	sensor_breakloop(&sensor);
}
//---------------------

int main(int argc, char** argv) {
	struct arguments arguments;

// main
	arguments.interface = "eth0";
	arguments.promiscuous = false;
	arguments.buffersize = 65536;
// mysql
	arguments.db_host = "localhost";
	arguments.db_username = 0;
	arguments.db_password = 0;
	arguments.db_schema = "sniffer";
	arguments.db_table = "sniffer";
	arguments.db_port = 0;
// periods
	arguments.capture_timeout = 5;
	arguments.dissection_period = 5;
	arguments.persist_period = 10;
// debug
	arguments.enable_persistance = true;
	arguments.enable_redirect = true;
// misc
	arguments.verbose = false;

	argp_parse(&args_parser, argc, argv, 0, 0, &arguments);
//-----------

	signal(SIGINT, break_loop);
	signal(SIGQUIT, break_loop);

	sensor = sensor_init();

	if (arguments.enable_persistance) {
		db_init(&arguments);
		db_connect();
		db_prepare_statement();
		sensor_set_persist_callback(&sensor, persist_callback);
	} else {
		sensor_set_persist_callback(&sensor, 0);
	}


	opts.promiscuous = arguments.promiscuous;
	opts.device_name = arguments.interface;
	opts.buffersize = arguments.buffersize;

	opts.capture_timeout = arguments.capture_timeout;
	opts.persist_timeout = arguments.persist_period;
	opts.dissect_timeout = arguments.dissection_period;
	opts.enable_redirect = arguments.enable_redirect;

	sensor_set_options(&sensor, opts);
	sensor_set_dissection_simple(&sensor);
	sensor_loop(&sensor);

	if (arguments.enable_persistance) {
		db_close_statement();
		db_disconnect();
	}


	return (EXIT_SUCCESS);
}


