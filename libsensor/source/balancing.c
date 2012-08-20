#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_arp.h>

#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>

#include <arpa/inet.h>

#include "balancing.h"
#include "load_count.h"
#include "debug.h"
#include "util.h"
#include "nodes.h"
#include "packet_extract.h"
#include "hashmap.h"
#include "arraylist.h"

#include "bestfit.h"

char *state_text[] = {
	"STATE_BEGIN",
	"STATE_WAIT_SENSORS",
	"STATE_ALONE",
	"STATE_COUPLE"
};


struct SensorSession {
	time_t created;
	time_t last_info;
	struct in_addr address;
	ArrayList owned;
};

struct balancer {
	struct NetAddress current;
	ArrayList owned;

	HashMap clientMomentLoads;
	HashMap sensorSessions;
};




