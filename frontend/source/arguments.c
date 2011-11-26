#include <stdlib.h>
#include <argp.h>

#include "arguments.h"

#define OPT_DISABLE_PERSIST 1
#define OPT_DISABLE_REDIRECT 2
#define OPT_DISABLE_FORK 3

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
		{"disable-fork", OPT_DISABLE_FORK, 0, 0, "Disable redirect (for debug purposes)", 3},
		// misc
		{"verbose", 'v', 0, 0, "Verbose mode", 4},
		{"background", 'g', 0, 0, "Detach from console", 4},

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

	case OPT_DISABLE_FORK:
		arguments->enable_fork = false;
		break;

	// misc
	case 'v':
		arguments->verbose = true;
		break;
	case 'g':
		arguments->background = true;
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;

}

static struct argp args_parser = {options, parse_options};

struct arguments args_get_default() {

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
	arguments.enable_fork = true;
	// misc
	arguments.verbose = false;
	arguments.background = false;

	return arguments;
}

error_t args_parse(int argc, char** argv, struct arguments *arguments) {
	return argp_parse(&args_parser, argc, argv, 0, 0, arguments);
}

