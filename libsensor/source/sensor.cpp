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

#include "sensor_private.hpp"
#include "socket_utils.hpp"

/* thread modules */
#include "traffic_capture.hpp"
#include "poluter.hpp"

#include "debug.hpp"
#include "nodes.hpp"
#include "netinfo.hpp"
#include "util.hpp"

#define SENSOR_DEFAULT_READ_BUFFER_SIZE 65536
#define SENSOR_DEFAULT_TIMEOUT 1
#define SENSOR_DEFAULT_PROMISC false

enum {
	MQCAPTURE,
	MQPOLUTER,
	MQNEGOTIATOR
};

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



	return 0;
}

//------------------------------------------------

/*
 * Rewrites packet mac from packet ip address if
 * it is not equal to sensor ip
 *
 * returns true if rewrite occurred, false otherwise
 */
/*
bool prepare_redirect(sensor_t sensor, uint8_t* buffer, size_t captured) {
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
}*/
/*
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
}*/

//-----------------interfaces---------------------
sensor_t sensor_init() {
	sensor_options_t options;
	sensor_t result = (sensor_t)malloc(sizeof(*result));
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



static pcap_t *create_pcap_handle(sensor_t context) {
	char errbuf[PCAP_ERRBUF_SIZE] = {0};
	pcap_t *pcapHandle = NULL;
	pcap_t *result = NULL;

	bool success = true;
	pcapHandle = pcap_create(context->opt.capture.device_name, errbuf);
	DINFO("Pcap created %s\n", errbuf);

	success = !pcap_set_promisc(pcapHandle, context->opt.capture.promiscuous);
	success = success && !pcap_set_buffer_size(pcapHandle, context->opt.capture.buffersize);

	if (!pcap_setdirection(pcapHandle, PCAP_D_OUT)) {
		DWARNING("%s\n", "can't set PCAP_D_IN");
	}

	success = success && !pcap_activate(pcapHandle);

	if (success) {
		result = pcapHandle;
		DINFO("Pcap initialized successfully\n", errbuf);
	} else {
		pcap_close(pcapHandle);
		DERROR("Pcap crash: %s\n", pcap_geterr(pcapHandle));
	}

	return result;
}











//----------------------------------------------------------
sensor_captured_t *init_captured(uint8_t *buffer, int len) {
	assert(buffer);

	uint8_t *begin = (uint8_t *)malloc(sizeof(sensor_captured_t) + len);
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

int sensor_main(sensor_t config) {
	struct TrafficCapture captureContext;
	struct Poluter poluterContext;

	pthread_t captureThread, poluterThread;

	MessageQueueContext mqContext;
	MessageQueue trafficEx, trafficCore, negotiatorEx, negotiatorCore, poluterEx, poluterCore;

	pcap_t *handle;

	config->current = read_interface_info(config->opt.capture.device_name);
	DNOTIFY("Current MAC: %s\n", EtherToStr(config->current.addr.hw));
	DNOTIFY("Current IP4: %s\n", Ip4ToStr(config->current.addr.in));
	DNOTIFY("Current NETMASK: %s\n", Ip4ToStr(config->current.netmask));
	DNOTIFY("Current GATEWAY: %s\n", Ip4ToStr(config->current.gateway));

	DNOTIFY("%s\n", "Creating pcap");
	handle = create_pcap_handle(config);

	mqContext = MessageQueue_context();
	DINFO("%s\n", "Message queue context created");

	MessageQueue_getPair(mqContext, MQCAPTURE, &trafficEx, &trafficCore);
	MessageQueue_getPair(mqContext, MQNEGOTIATOR, &negotiatorEx, &negotiatorCore);
	MessageQueue_getPair(mqContext, MQPOLUTER, &poluterEx, &poluterCore);

	captureContext.queueToCore = trafficEx;
	TrafficCapture_prepare(&captureContext, config, handle);

	poluterContext.queueToCore = poluterEx;
	Poluter_prepare(&poluterContext, config, handle);

	time_t iteration_time = time(0);
	struct timer survey_timer = {iteration_time, config->opt.survey.timeout};
	//struct timer balancing_timer = {iteration_time, config->opt.balancing.timeout};
	//struct timer spoof_timer = {iteration_time, config->opt.balancing.modify_timeout};

	config->activated = true;

	DNOTIFY("%s\n", "Starting");

	pthread_create(&captureThread, NULL, (void *(*)(void*))TrafficCapture_start, &captureContext);
	pthread_create(&poluterThread, NULL, (void *(*)(void*))Poluter_start, &poluterContext);

	DNOTIFY("%s\n", "Threads Started");

	int type;
	char data[256];
	size_t of_data;

	while (config->activated) {
		iteration_time = time(0);
		MessageQueue_recv(trafficCore, &type, data, &of_data);
		if (timer_check(&survey_timer, iteration_time)) {
			MessageQueue_send(poluterCore, POLUTER_MSG_SURVEY, data, 0);
			timer_ping(&survey_timer);
		}
	}
	DNOTIFY("%s\n", "Stopping threads");

	Poluter_stop(&poluterContext);
	TrafficCapture_stop(&captureContext);

	DNOTIFY("%s\n", "Cleaning up MQ");

	MessageQueue_destroy(trafficCore);
	MessageQueue_destroy(trafficEx);
	MessageQueue_destroy(negotiatorCore);
	MessageQueue_destroy(negotiatorEx);
	MessageQueue_destroy(poluterCore);
	MessageQueue_destroy(poluterEx);

	MessageQueue_contextDestroy(mqContext);

	DNOTIFY("%s\n", "Waiting for threads exit");

	pthread_join(poluterThread, NULL);
	pthread_join(captureThread, NULL);

	DNOTIFY("%s\n", "Closing pcap");

	pcap_close(handle);

	return 0;
}
