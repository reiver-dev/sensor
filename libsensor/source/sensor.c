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

#include <netpacket/packet.h>

#include "debug.h"                 // local
#include "sensor.h"
#include "dissect.h"
#include "debug.h"
#include "redirect.h"

#define SENSOR_DEFAULT_READ_BUFFER_SIZE 65536
#define SENSOR_DEFAULT_TIMEOUT 1
#define SENSOR_DEFAULT_PROMISC false


//------------PRIVATE---------------------
//---------socket-related---------------

int create_socket() {
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
		return SENSOR_IFACE_GET_FLAGS;
	}

	if (state) {
		interface.ifr_flags |= IFF_PROMISC;
	} else {
		interface.ifr_flags &= ~IFF_PROMISC;
	}

	//setting flags
	if (ioctl(sock, SIOCSIFFLAGS, &interface) == -1) {
		return SENSOR_IFACE_SET_FLAGS;
	}

	return SENSOR_SUCCESS;
}

int bind_socket_to_interface(int sock, char *interfaceName){

	struct ifreq interface;
	struct sockaddr_ll address;

	strcpy(interface.ifr_name, interfaceName);

	// Get interface index
	if (ioctl(sock, SIOCGIFINDEX, &interface) == -1) {
		return SENSOR_IFACE_GET_INDEX;
	}

	// Bind raw socket to interface
	address.sll_family   = AF_PACKET;
	address.sll_ifindex  = interface.ifr_ifindex;
	address.sll_protocol = htons(ETH_P_ALL);

	if (bind(sock, (struct sockaddr *)&address, sizeof(address)) == -1) {
		DEBUG_PRINTF("Can't bind socket");
		return SENSOR_BIND_SOCKET;
	}

	return SENSOR_SUCCESS;
}


int set_socket_timeout(int sock, int seconds) {
	struct timeval timeout;
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	int res = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	DEBUG_PRINTF("Set socket timeout, result: %d\n", res);
	return res;
}


//-----------sensor-related
int sensor_empty(){
	return 0;
}

int empty_persist(Queue_t in){
	sensor_dissected_t *packet = queue_pop(in);
	free(packet->content);
	free(packet->payload);
	free(packet);
	return 0;
}



int commit_config(sensor_t *config){
	config->captured = queue_init();
	config->dissected = queue_init();

	config->sock = create_socket();

	if (!config->sock) {
		return SENSOR_CREATE_SOCKET;
	}

	if (config->opt.promiscuous) {
		int res = set_iface_promiscuous(config->sock, config->opt.device_name, true);
		if (res)
			return res;
	}

	bind_socket_to_interface(config->sock, config->opt.device_name);

	if (config->opt.timeout_capture) {
		int res = set_socket_timeout(config->sock, config->opt.timeout_capture);
		if (res)
			return res;
	}

	get_current_mac_r(config->sock, config->opt.device_name, config->hwaddr);
	config->ip4addr = get_current_address(config->sock, config->opt.device_name);

	return 0;
}

int sensor_destroy(sensor_t *config){
	DEBUG_PRINTF("Destroying sensor\n");
	close_socket(config->sock);
	queue_destroy(config->captured);
	queue_destroy(config->dissected);
	if (config->opt.promiscuous){
		int res;
		if (!(res = set_iface_promiscuous(config->sock, config->opt.device_name, false)))
			DEBUG_PRINTF("Disabling promiscuous: %d\n", res);
			return res;
	}
	return 0;
}
//------------------------------------------------

/*
 * Rewrites packet mac from packet ip address if
 * it is not equal to sensor ip
 *
 * returns true if rewrite occurred, false otherwise
 */
bool prepare_redirect(sensor_t *sensor, uint8_t* buffer, int captured) {
	if (captured < (sizeof(struct ether_header) + sizeof(struct iphdr))) {
		return false;
	}

	int position = sizeof(struct ether_header);
	struct ether_header *ethernet = (struct ether_header*) (buffer);

	if (ethernet->ether_type == ETHERTYPE_IP) {
		struct iphdr *ipheader = (struct iphdr*) (buffer + position);
		if (ipheader->daddr != sensor->ip4addr) {
			read_arp_ip_to_mac_r(sensor->sock, sensor->opt.device_name, ipheader->daddr, ethernet->ether_dhost);
			return true;
		}
	}

	return false;
}


//-----------------interfaces---------------------
sensor_t sensor_init(){
	sensor_options_t options = {"",SENSOR_DEFAULT_PROMISC,SENSOR_DEFAULT_READ_BUFFER_SIZE, SENSOR_DEFAULT_TIMEOUT};
	struct sensor result;
	result.activated = false;
	result.sock = 0;
	result.opt = options;
	result.dissect_function = sensor_empty;
	result.persist_function = sensor_empty;
	return result;
}

int sensor_set_options(sensor_t *config, sensor_options_t options){
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->opt = options;
	return SENSOR_SUCCESS;
}

int sensor_set_dissection_default(sensor_t *config){
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->dissect_function = sensor_dissect_simple;
	return SENSOR_SUCCESS;
}

int sensor_set_dissection(sensor_t *config, sensor_dissect_f callback){
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->dissect_function = callback;
	return SENSOR_SUCCESS;
}

int sensor_set_persist_callback(sensor_t *config, sensor_persist_f callback){
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

sensor_captured_t* init_captured(uint8_t *buffer, int len){
	assert(buffer);
	sensor_captured_t *captured = malloc(sizeof(sensor_captured_t));
	captured->timestamp = time(0);
	captured->length = len;
	captured->buffer = malloc(len);
	memcpy(captured->buffer, buffer, len);
	return captured;
}

void destroy_captured(sensor_captured_t *captured){
	assert(captured);
	free(captured->buffer);
	free(captured);
}

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
void sensor_breakloop(sensor_t *config){
	config->activated = false;
}

int sensor_loop(sensor_t *config){

	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->activated = true;

	int res = commit_config(config);
	if (res)
		return res;



	//queue intervals
	time_t iteration_time=0;
	time_t dissect_time=0;
	time_t persist_time=0;

	//buffer length
	int buflength = config->opt.buffersize;
	uint8_t *buffer = malloc(buflength);

	//Main loop
	DEBUG_PRINTF("Starting capture\n");
	while(config->activated || queue_length(config->captured) || queue_length(config->dissected)){
		iteration_time = time(0);

		// complete queue if we broke the loop
		if(config->activated){

			// wait for packet for given timeout and then read it
			int read_len = recv(config->sock, buffer, buflength, 0);
			DEBUG_PRINTF("Captured: %d bytes\n", read_len);

			if(read_len > 0){

				if (config->opt.enable_redirect && prepare_redirect(config, buffer, read_len)) {
					send(config->sock, buffer, read_len, 0);
				}

				sensor_captured_t *captured = init_captured(buffer, read_len);

				queue_push(config->captured, captured);
			}

		}

		DEBUG_PRINTF("QUEUE CAP:%i=>ITER:%i=>DIS:%i\n", queue_length(config->captured), (uint32_t)iteration_time, (uint32_t)dissect_time);

		if ((queue_length(config->captured)
			&& (iteration_time - dissect_time) > config->opt.timeout_dissect)
			|| !config->activated)
		{
			DEBUG_PRINTF("Dissecting: %d packets\n", queue_length(config->captured));
			while(queue_length(config->captured) !=0){
				DEBUG_PRINTF("dissecting\n");
				config->dissect_function(config->captured, config->dissected);
				dissect_time = time(0);
			}
		}

		DEBUG_PRINTF("QUEUE DIS:%i=>ITER:%i=>PER:%i\n", queue_length(config->dissected), (uint32_t)iteration_time, (uint32_t)persist_time);

		if ((queue_length(config->dissected)
			&& (iteration_time - persist_time) > config->opt.timeout_persist)
			|| !config->activated)
		{

			DEBUG_PRINTF("Persisting: %d packets\n", queue_length(config->dissected));
			while(queue_length(config->dissected) !=0){
				config->persist_function(config->dissected);
				persist_time = time(0);
			}
		}

	} /* while */
	DEBUG_PRINTF("Capture ended\n");
	sensor_destroy(config);
	return SENSOR_SUCCESS;
}




