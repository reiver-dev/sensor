#include <stdlib.h>
#include <argp.h>

#include "arguments.h"

#define OPT_DISABLE_PERSIST 1
#define OPT_DISABLE_REDIRECT 2
#define OPT_DISABLE_FORK 3

const char * argp_program_version = "Sensor v0.0.1";
const char * argp_program_bug_address = "<Bushev A.S.>";

const struct argp_option options[] = {

		{"config", 'c', "FILE", 0, "Config file to use"},

		// main
		{"interface", 'i', "IFACE", 0, "Specify interface to listen (DEFAULT eth0)"},
		{"promisc", 'p', 0, 0, "Specify promiscuous mode for interface (DEFAULT false)"},
		{"buffer", 'b', "BYTES", 0, "Specify capture buffer size (DEFAULT 65536)"},

		// persistance
		{"host", 'H', "HOSTNAME", 0, "Head connection host address", 1},
		{"port", 'P', "PORT", 0, "Head connection util port", 1},
		{"dump", 'D', "PORT", 0, "Head connection dump port", 1},
		{"headkey", 'K', "KEY", 0, "SHA2 key for head", 1},

		// period
		{"capture-timeout", 't', "SECONDS", 0, "Specify capture timeout (DEFAULT 5)", 2},
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

	case 'c':
		arguments->config = arg;
		break;

	// main
	case 'i':
		arguments->capture_interface = arg;
		break;
	case 'p':
		arguments->capture_promiscuous = true;
		break;
	case 'b':
		arguments->capture_buffersize = atoi(arg);
		break;

	// for database
	case 'H':
		arguments->head_host = arg;
		break;
	case 'P':
		arguments->head_util_port = atoi(arg);
		break;
	case 'D':
		arguments->head_dump_port = atoi(arg);
		break;
	case 'K':
		arguments->head_key = arg;
		break;


	case 't':
		arguments->capture_timeout = atoi(arg);
		break;

	case 'w':
		arguments->persist_period = atoi(arg);
		break;


	case OPT_DISABLE_PERSIST:
		arguments->enable_persistance = false;
		break;

	case OPT_DISABLE_REDIRECT:
		arguments->nodes_enable_redirect = false;
		break;

	case OPT_DISABLE_FORK:
		arguments->enable_fork = false;
		break;

	// misc
	case 'v':
		arguments->verbose = true;
		break;
	case 'g':
		arguments->enable_background = true;
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;

}

static struct argp args_parser = {options, parse_options};

struct arguments args_get_default() {

	struct arguments arguments;
	arguments.config = 0;
	// main
	arguments.capture_interface = "eth0";
	arguments.capture_promiscuous = false;
	arguments.capture_buffersize = 65536;
	// mysql
	arguments.head_host = "localhost";
	arguments.head_util_port = 0;
	arguments.head_dump_port = 0;
	arguments.head_key = "dummy";
	// periods
	arguments.capture_timeout = 5;
	arguments.persist_period = 10;
	// debug
	arguments.enable_persistance = true;
	arguments.nodes_enable_redirect = true;
	arguments.enable_fork = true;
	// misc
	arguments.verbose = false;
	arguments.enable_background = false;

	return arguments;
}

error_t args_parse(int argc, char** argv, struct arguments *arguments) {
	return argp_parse(&args_parser, argc, argv, 0, 0, arguments);
}

