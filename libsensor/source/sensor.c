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

	get_current_mac_r(config->sock, config->opt.capture.device_name,
			config->current.hwaddr);
	config->current.ip4addr = get_current_address(config->sock,
			config->opt.capture.device_name);
	config->current.netmask = get_current_netmask(config->sock,
			config->opt.capture.device_name);
	config->current.gateway = read_default_gateway(
			config->opt.capture.device_name);

	DNOTIFY("Current MAC: %s\n", EtherToStr(config->current.hwaddr));DNOTIFY("Current IP4: %s\n", Ip4ToStr(config->current.ip4addr));DNOTIFY("Current NETMASK: %s\n", Ip4ToStr(config->current.netmask));DNOTIFY("Current GATEWAY: %s\n", Ip4ToStr(config->current.gateway));

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
		if (ipheader->daddr != sensor->current.ip4addr
				&& ipheader->saddr != sensor->current.ip4addr
				&& !memcmp(ethernet->ether_dhost, sensor->current.hwaddr,
						ETH_ALEN)
				&& memcmp(ethernet->ether_shost, sensor->current.hwaddr,
						ETH_ALEN)
				&& (dest = nodes_get_destination(ipheader->daddr))) {

			memcpy(ethernet->ether_dhost, dest->hwaddr, ETH_ALEN);
			memcpy(ethernet->ether_shost, sensor->current.hwaddr, ETH_ALEN);
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

		if (ipheader->daddr != sensor->current.ip4addr
				&& ipheader->saddr != sensor->current.ip4addr
				&& memcmp(ethernet->ether_dhost, sensor->current.hwaddr,
						ETH_ALEN)
				&& !memcmp(ethernet->ether_shost, sensor->current.hwaddr,
						ETH_ALEN)) {

			struct Node *source = nodes_get_destination(ipheader->saddr);
			if (!source)
				return false;

			memcpy(ethernet->ether_shost, source->hwaddr, ETH_ALEN);
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

int sensor_loop(sensor_t config) {

	sensor_captured_t *captured;
	int buflength;
	uint8_t *buffer;
	Balancer balancer;
	time_t iteration_time;

	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->activated = true;

	int res = commit_config(config);
	if (res)
		return res;

	// buffer length
	buflength = config->opt.capture.buffersize;
	buffer = malloc(buflength);

	nodes_init(&config->current);
	balancer = balancing_init(config);

	/* Initial */
	iteration_time = time(0);
	survey_perform_survey(&config->current, config->sock);
	while ((time(0) - iteration_time) < config->opt.survey.initial_timeout) {
		int read_len = recv(config->sock, buffer, buflength, 0);
		if (read_len > 0) {
			survey_process_response(&config->current, buffer, read_len);
		}
	}

	iteration_time = time(0);
	balancing_process(balancer);
	while ((time(0) - iteration_time) < config->opt.balancing.initial_timeout) {
		balancing_receive_service(balancer);
		usleep(500);
	}

	balancing_process(balancer);

	if (config->opt.balancing.enable_modify) {
		ArrayList owned = balancing_get_owned(balancer);
		Spoof_nodes(config->sock, owned, &config->current);
	}

	iteration_time = time(0);
	struct timer survey_timer = { iteration_time, config->opt.survey.timeout };
	struct timer balancing_timer = { iteration_time,
			config->opt.balancing.timeout };
	struct timer spoof_timer = { iteration_time,
			config->opt.balancing.modify_timeout };
	struct timer persist_timer = { iteration_time, config->opt.persist.timeout };

	Queue_t Qcaptured = queue_init();

	/* Main loop */
	DNOTIFY("%s\n", "Starting capture");
	while (config->activated || queue_length(Qcaptured)) {
		iteration_time = time(0);

		/* complete only queue if we broke the loop */
		if (config->activated) {

			if (timer_check(&survey_timer, iteration_time)) {
				survey_perform_survey(&config->current, config->sock);
				timer_ping(&survey_timer);
			}

			if (config->opt.balancing.enable_modify)
				if (timer_check(&spoof_timer, iteration_time)) {
					ArrayList owned = balancing_get_owned(balancer);
					Spoof_nodes(config->sock, owned, &config->current);
					timer_ping(&spoof_timer);
				}

			if (timer_check(&balancing_timer, iteration_time)) {
				balancing_process(balancer);
				timer_ping(&balancing_timer);
			}

			/* wait for packet for given timeout and then read it */
			int read_len = recv(config->sock, buffer, buflength, 0);
			//DINFO("Captured: %d bytes\n", read_len);

			/* process the packet */
			if (read_len > 0) {

				balancing_receive_service(balancer);
				/* if not survey or balancing packet */
				if (!survey_process_response(&config->current, buffer, read_len)
						&& !balancing_filter_response(balancer, buffer,
								read_len)) {

					/* perform redirect if enabled and packet addresses replacement */
					if (config->opt.balancing.enable_redirect
							&& prepare_redirect(config, buffer, read_len)) {
						send(config->sock, buffer, read_len, 0);
						restore_redirect(config, buffer, read_len);
					}

					balancing_add_load(balancer, buffer, read_len);
					balancing_count_load(balancer);

					/* put captured packet in queue for dissection */
					captured = init_captured(buffer, read_len);
					queue_push(Qcaptured, captured);
				}
			}

		}

		/* Persistance */
		if ((queue_length(Qcaptured) && timer_check(&persist_timer, iteration_time)) || !config->activated) {
			while (queue_length(Qcaptured)) {
				captured = queue_pop(Qcaptured);
				config->persist_function(captured);
				free(captured);
			}
			timer_ping(&persist_timer);
		}

		if (!config->activated) {
			balancing_disconnect(balancer);
		}

	} /* while */

	DNOTIFY("%s\n","Capture ended");
	balancing_destroy(balancer);
	nodes_destroy();

	free(buffer);
	queue_destroy(Qcaptured);

	sensor_clean(config);
	return SENSOR_SUCCESS;
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
