#include <stdlib.h>                // Standard Libraries
#include <stdio.h>

#include <stdint.h>                // Additional libraries
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

#include <sys/select.h>            // For package capture

#include <sys/ioctl.h>             // For interface flags change

#include <netinet/in.h>            // Basic address related functions and types
#include <netinet/ether.h>
#include <net/if.h>

#include <arpa/inet.h>

#include <netpacket/packet.h>

#include "sensor_private.h"
#include "debug.h"

#include "dissect.h"

#include "nodes.h"
#include "services/services.h"
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
//---------socket-related---------------

int create_raw_socket() {
	/*
	 * Packet family
	 * Linux specific way of getting packets at the dev level
	 * Capturing every packet
	 */
	int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	return sock;
}


int close_socket(int socket) {
	shutdown(socket, 2);
	return close(socket);
}


int set_iface_promiscuous(int sock, const char* interfaceName, bool state) {
	struct ifreq interface;

	strcpy(interface.ifr_name, interfaceName);

	//reading flags
	if (ioctl(sock, SIOCGIFFLAGS, &interface) == -1) {
		DERROR("%s\n", "get interface flags failed");
		return SENSOR_IFACE_GET_FLAGS;
	}

	if (state) {
		interface.ifr_flags |= IFF_PROMISC;
	} else {
		interface.ifr_flags &= ~IFF_PROMISC;
	}

	//setting flags
	if (ioctl(sock, SIOCSIFFLAGS, &interface) == -1) {
		DERROR("%s\n", "set interface flags failed");
		return SENSOR_IFACE_SET_FLAGS;
	}

	return SENSOR_SUCCESS;
}

int set_socket_timeout(int sock, int seconds) {
	struct timeval timeout;
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	int res = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	DINFO("Set socket timeout, result: %d\n", res);
	return res;
}


//-----------sensor-related
int sensor_empty(){
	return 0;
}

int empty_persist(Queue_t in){
	sensor_dissected_t *packet = queue_pop(in);
	free(packet);
	return 0;
}



int commit_config(sensor_t config){
	config->captured = queue_init();
	config->dissected = queue_init();

	config->sock = create_raw_socket();

	if (!config->sock) {
		return SENSOR_CREATE_SOCKET;
	}

	if (config->opt.capture.promiscuous) {
		int res = set_iface_promiscuous(config->sock, config->opt.device_name, true);
		if (res)
			return res;
	}

	bind_socket_to_interface(config->sock, config->opt.device_name);

	if (config->opt.capture.timeout) {
		int res = set_socket_timeout(config->sock, config->opt.capture.timeout);
		if (res)
			return res;
	}

	get_current_mac_r(config->sock, config->opt.device_name, config->current.hwaddr);
	config->current.ip4addr = get_current_address(config->sock, config->opt.device_name);
	config->current.netmask = get_current_netmask(config->sock, config->opt.device_name);
	config->current.gateway = read_default_gateway(config->opt.device_name);

	DNOTIFY("Current MAC: %s\n",     EtherToStr(config->current.hwaddr));
	DNOTIFY("Current IP4: %s\n",     Ip4ToStr(config->current.ip4addr));
	DNOTIFY("Current NETMASK: %s\n", Ip4ToStr(config->current.netmask));
	DNOTIFY("Current GATEWAY: %s\n", Ip4ToStr(config->current.gateway));


	return 0;
}

int sensor_clean(sensor_t config){
	assert(config);
	DNOTIFY("%s\n", "Destroying sensor");
	queue_destroy(config->captured);
	queue_destroy(config->dissected);
	if (config->opt.capture.promiscuous) {
		int res = set_iface_promiscuous(config->sock, config->opt.device_name, false);
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

		if (ipheader->daddr != sensor->current.ip4addr
			&& ipheader->saddr != sensor->current.ip4addr
			&& !memcmp(ethernet->ether_dhost, sensor->current.hwaddr, ETH_ALEN)
			&& memcmp(ethernet->ether_shost, sensor->current.hwaddr, ETH_ALEN)) {

			struct Node *dest = node_get_destination(ipheader->daddr);
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
			&& memcmp(ethernet->ether_dhost, sensor->current.hwaddr, ETH_ALEN)
			&& !memcmp(ethernet->ether_shost, sensor->current.hwaddr, ETH_ALEN)) {

			struct Node *source = node_get_destination(ipheader->saddr);
			memcpy(ethernet->ether_shost, source->hwaddr, ETH_ALEN);
			return true;
		}
	}

	return false;
}

//-----------------interfaces---------------------
sensor_t sensor_init(){
	sensor_options_t options;
	sensor_t result = malloc(sizeof(*result));
	result->activated = false;
	result->sock = 0;
	result->opt = options;
	result->dissect_function = sensor_dissect_simple;
	result->persist_function = sensor_empty;
	return result;
}

void sensor_destroy(sensor_t config){
	assert(config);
	free(config);
}

int sensor_set_options(sensor_t config, sensor_options_t options){
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->opt = options;
	return SENSOR_SUCCESS;
}

int sensor_set_dissection_default(sensor_t config){
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->dissect_function = sensor_dissect_simple;
	return SENSOR_SUCCESS;
}

int sensor_set_dissection(sensor_t config, sensor_dissect_f callback){
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->dissect_function = callback;
	return SENSOR_SUCCESS;
}

int sensor_set_persist_callback(sensor_t config, sensor_persist_f callback){
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

sensor_captured_t *init_captured(uint8_t *buffer, int len){
	assert(buffer);

	uint8_t *begin = malloc(sizeof(sensor_captured_t) + len);
	uint8_t *content = begin + sizeof(sensor_captured_t);

	sensor_captured_t *captured = (sensor_captured_t *)begin;
	captured->timestamp = time(0);
	captured->length = len;
	captured->buffer = content;
	memcpy(captured->buffer, buffer, len);
	return captured;
}

sensor_dissected_t *init_dissected(int content_length, int payload_length) {
	// 2 is spacer
	uint8_t *begin = malloc(sizeof(sensor_dissected_t) + content_length + payload_length );
	uint8_t *content = begin + sizeof(sensor_dissected_t);
	uint8_t *payload = content + content_length;

	sensor_dissected_t *dissected = (sensor_dissected_t *)begin;
	dissected->content = (char *)content;
	dissected->payload = payload;

	dissected->content_length = content_length;
	dissected->payload_length = payload_length;

	return dissected;
}

/* DEPRACHATED */
void destroy_captured(sensor_captured_t *captured){
	assert(captured);
	assert(captured->buffer);
	free(captured->buffer);
	free(captured);
}
/* DEPRACHATED */
void destroy_dissected(sensor_dissected_t *dissected){
	assert(dissected);
	if (dissected->content) {
		free(dissected->content);
	}
	if (dissected->payload) {
		free(dissected->payload);
	}
	free(dissected);
}


/* Main sensor loop */
void sensor_breakloop(sensor_t config) {
	config->activated = false;
}

int sensor_loop(sensor_t config) {

	sensor_captured_t *captured;
	sensor_dissected_t *dissected;
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

	balancer = balancing_init(config);
	nodes_init(&config->current);

	/* Initial */
	iteration_time = time(0);
	survey_perform_survey(&config->current, config->sock);
	while ((time(0) - iteration_time) < 5) {
		int read_len = recv(config->sock, buffer, buflength, 0);
		if (read_len > 0) {
			survey_process_response(&config->current, buffer, read_len);
		}
	}

	iteration_time = time(0);
	balancing_process(balancer);
	while ((time(0) - iteration_time) < 5) {
		balancing_receive_service(balancer);
		usleep(500);
	}

	balancing_process(balancer);

	if (config->opt.balancing.enable_modify) {
		ArrayList owned = balancing_get_owned(balancer);
		Spoof_nodes(config->sock, owned, &config->current);
		ArrayList_destroy(owned);
	}

	iteration_time = time(0);
	struct timer dissect_timer   = {iteration_time, config->opt.dissect.timeout};
	struct timer persist_timer   = {iteration_time, config->opt.persist.timeout};
	struct timer survey_timer    = {iteration_time, config->opt.balancing.survey_timeout};
	struct timer balancing_timer = {iteration_time, config->opt.balancing.timeout};
	struct timer spoof_timer     = {iteration_time, config->opt.balancing.spoof_timeout};

	uint32_t load_interval = 2;
	uint32_t load_count = 5;

	/* Main loop */
	DNOTIFY("%s\n", "Starting capture");
	while(config->activated || queue_length(config->captured) || queue_length(config->dissected)){
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
					ArrayList_destroy(owned);
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
					&& !balancing_filter_response(balancer, buffer, read_len)) {

					/* perform redirect if enabled and packet addresses replacement */
					if (config->opt.balancing.enable_redirect
						&& prepare_redirect(config, buffer, read_len))
					{
						send(config->sock, buffer, read_len, 0);
						restore_redirect(config, buffer, read_len);
					}

					balancing_add_load(balancer, buffer, read_len);
					balancing_count_load(balancer, load_interval, load_count);

					/* put captured packet in queue for dissection */
					captured = init_captured(buffer, read_len);
					queue_push(config->captured, captured);
				}
			}

		}

		/* Dissection */
		if ((queue_length(config->captured)	&& timer_check(&dissect_timer, iteration_time)) || !config->activated) {
			DINFO("Dissecting: %d packets\n", queue_length(config->captured));
			while(queue_length(config->captured)) {
				captured = queue_pop(config->captured);
				dissected = config->dissect_function(captured);
				free(captured);
				queue_push(config->dissected, dissected);
			}
			timer_ping(&dissect_timer);
		}

		/* Persistance */
		if ((queue_length(config->dissected) && timer_check(&persist_timer, iteration_time)) || !config->activated)	{
			while(queue_length(config->dissected) != 0) {
				config->persist_function(config->dissected);
			}
			timer_ping(&persist_timer);
		}

	} /* while */
	DNOTIFY("%s\n","Capture ended");
	balancing_destroy(balancer);
	nodes_destroy();
	free(buffer);
	sensor_clean(config);
	return SENSOR_SUCCESS;
}
