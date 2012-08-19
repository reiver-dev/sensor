#include <stdlib.h>                // Standard Libraries
#include <stdint.h>                // Additional libraries
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

#include <netinet/ether.h>
#include <netinet/ip.h>

#include "sensor_private.h"
#include "socket_utils.h"

#include "traffic_capture.h"
#include "queue.h"
#include "debug.h"
#include "nodes.h"
#include "balancing.h"
#include "survey.h"
#include "spoof.h"
#include "netinfo.h"
#include "util.h"

#define SENSOR_DEFAULT_READ_BUFFER_SIZE 65536
#define SENSOR_DEFAULT_TIMEOUT 1
#define SENSOR_DEFAULT_PROMISC false

struct timer {
	time_t last;
	time_t period;
};

bool timer_check(struct timer *timer, time_t now) {
	return (now - timer->last) > timer->period;
}

void timer_ping_r(struct timer *timer, time_t now) {
	timer->last = now;
}

void timer_ping(struct timer *timer) {
	timer_ping_r(timer, time(0));
}

//------------PRIVATE---------------------
//-----------sensor-related

int empty_persist(sensor_captured_t *packet) {
	return 0;
}

int commit_config(sensor_t config) {

	config->sock = create_raw_socket();

	if (!config->sock) {
		return SENSOR_CREATE_SOCKET;
	}

	if (config->opt.capture.promiscuous) {
		int res = set_iface_promiscuous(config->sock,
				config->opt.capture.device_name, true);
		if (res)
			return res;
	}

	bind_socket_to_interface(config->sock, config->opt.capture.device_name);

	if (config->opt.capture.timeout) {
		int res = set_socket_timeout(config->sock, config->opt.capture.timeout);
		if (res)
			return res;
	}

	config->current = read_interface_info(config->opt.capture.device_name);

	DNOTIFY("Current MAC: %s\n", EtherToStr(config->current.addr.hw));
	DNOTIFY("Current IP4: %s\n", Ip4ToStr(config->current.addr.in));
	DNOTIFY("Current NETMASK: %s\n", Ip4ToStr(config->current.netmask));
	DNOTIFY("Current GATEWAY: %s\n", Ip4ToStr(config->current.gateway));

	return 0;
}

int sensor_clean(sensor_t config) {
	assert(config);
	DNOTIFY("%s\n", "Destroying sensor");
	if (config->opt.capture.promiscuous) {
		int res = set_iface_promiscuous(config->sock,
				config->opt.capture.device_name, false);
		if (res)
			return res;
	}
	close_socket(config->sock);
	return 0;
}

//------------------------------------------------

/*
 * Rewrites packet mac from packet ip address if
 * it is not equal to sensor ip
 *
 * returns true if rewrite occurred, false otherwise
 */

bool prepare_redirect(sensor_t sensor, uint8_t* buffer, int captured) {
	if (captured < (sizeof(struct ether_header) + sizeof(struct iphdr))) {
		return false;
	}

	int position = sizeof(struct ether_header);
	struct ether_header *ethernet = (struct ether_header*) (buffer);

	uint16_t ethernetType = htons(ethernet->ether_type);
	if (ethernetType == ETH_P_IP) {
		struct iphdr *ipheader = (struct iphdr*) (buffer + position);

		struct Node *dest;
		if (ipheader->daddr != sensor->current.addr.in
				&& ipheader->saddr != sensor->current.addr.in
				&& !memcmp(ethernet->ether_dhost, sensor->current.addr.hw,
						ETH_ALEN)
				&& memcmp(ethernet->ether_shost, sensor->current.addr.hw,
						ETH_ALEN)
				&& (dest = nodes_get_destination(ipheader->daddr))) {

			memcpy(ethernet->ether_dhost, dest->addr.hw, ETH_ALEN);
			memcpy(ethernet->ether_shost, sensor->current.addr.hw, ETH_ALEN);
			return true;

		}
	}

	return false;
}

bool restore_redirect(sensor_t sensor, uint8_t* buffer, int captured) {
	if (captured < (sizeof(struct ether_header) + sizeof(struct iphdr))) {
		return false;
	}

	int position = sizeof(struct ether_header);
	struct ether_header *ethernet = (struct ether_header*) (buffer);

	uint16_t ethernetType = htons(ethernet->ether_type);
	if (ethernetType == ETH_P_IP) {
		struct iphdr *ipheader = (struct iphdr*) (buffer + position);

		if (ipheader->daddr != sensor->current.addr.in
				&& ipheader->saddr != sensor->current.addr.in
				&& memcmp(ethernet->ether_dhost, sensor->current.addr.hw,
						ETH_ALEN)
				&& !memcmp(ethernet->ether_shost, sensor->current.addr.hw,
						ETH_ALEN)) {

			struct Node *source = nodes_get_destination(ipheader->saddr);
			if (!source)
				return false;

			memcpy(ethernet->ether_shost, source->addr.hw, ETH_ALEN);
			return true;
		}
	}

	return false;
}

//-----------------interfaces---------------------
sensor_t sensor_init() {
	sensor_options_t options;
	sensor_t result = malloc(sizeof(*result));
	result->activated = false;
	result->sock = 0;
	result->opt = options;
	result->persist_function = empty_persist;
	return result;
}

void sensor_destroy(sensor_t config) {
	assert(config);
	free(config);
}

int sensor_set_options(sensor_t config, sensor_options_t options) {
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->opt = options;
	return SENSOR_SUCCESS;
}

int sensor_set_persist_callback(sensor_t config, sensor_persist_f callback) {
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	if (callback) {
		config->persist_function = callback;
	} else {
		config->persist_function = empty_persist;
	}
	return SENSOR_SUCCESS;
}

//----------------------------------------------------------

sensor_captured_t *init_captured(uint8_t *buffer, int len) {
	assert(buffer);

	uint8_t *begin = malloc(sizeof(sensor_captured_t) + len);
	uint8_t *content = begin + sizeof(sensor_captured_t);

	sensor_captured_t *captured = (sensor_captured_t *) begin;
	gettimeofday(&captured->header.ts, NULL);
	captured->header.caplen = len;
	captured->header.len = len;
	captured->buffer = content;
	memcpy(captured->buffer, buffer, len);
	return captured;
}

/* Main sensor loop */
void sensor_breakloop(sensor_t config) {
	config->activated = false;
}

#define MQCORE 1
#define MQPERSIST 2
int sensor_main(sensor_t config) {
	MessageQueueContext mqContext = MessageQueue_context();
	DINFO("%s\n", "Message queue context created");

	MessageQueue coreMQ = MessageQueue_getReceiver(mqContext, MQCORE);

	DERROR("%s\n", "Core receiver MQ created");

	struct TrafficCapture captureContext;
	captureContext.queueToCore = MessageQueue_getSender(mqContext, MQCORE);
	captureContext.queueToPersist = 0;


	TrafficCapture_prepare(&captureContext, config);

	config->activated = true;

	DNOTIFY("%s\n", "Starting");

	pthread_t captureThread;
	pthread_create(&captureThread, NULL, (void *(*)(void*))TrafficCapture_start, &captureContext);

	while (config->activated) {
		void *data = NULL;
		size_t size;
		MessageQueue_recv(coreMQ, &data, &size);
		if (size && data) {
			DINFO("received %i bytes\n", size);
			free(data);
		}
	}
	DINFO("%s\n", "Closing threads");

	TrafficCapture_stop(&captureContext);

	pthread_join(captureThread, NULL);

	DNOTIFY("%s\n", "Cleaning up MQ");

	MessageQueue_destroy(coreMQ);
	MessageQueue_destroy(captureContext.queueToCore);
	MessageQueue_contextDestroy(mqContext);

	return 0;
}
