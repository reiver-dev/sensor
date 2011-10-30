// Standard Libraries
#include <stdlib.h>
#include <stdio.h>
// Additional libraries
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
// For package capture
#include <sys/select.h>
// For interface flags change
#include <sys/ioctl.h>
// Basic address related functions and types
#include <netinet/in.h>
#include <netinet/ether.h>
// replace for net/if.h
#include <net/if.h>
// local
#include "debug.h"
#include "sensor.h"
#include "dissect.h"

#define SENSOR_DEFAULT_READ_BUFFER_SIZE 65536
#define SENSOR_DEFAULT_TIMEOUT 1
#define SENSOR_DEFAULT_PROMISC false

//-------------------------------

typedef int (*sensor_dissect_f)(Queue_t *in, Queue_t *out);

typedef struct {
	char *device_name;
	bool promiscuous;
	uint32_t buffersize;
	uint8_t timeout;
} sensor_options_t;

struct sensor{
	bool activated;
	int sock;
	sensor_options_t opt;
	Queue_t captured;
	Queue_t dissected;
	sensor_dissect_f dissect_function;
	sensor_persist_f persist_function;
};
//----------------------------------

int sensor_empty(){
	return 0;
}

int create_socket() {
	/*
	 * Packet family
	 * Linux specific way of getting packets at the dev level
	 * Capturing every packet
	 */
	int sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
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
	if(ioctl(sock, SIOCGIFFLAGS, &interface) == -1){
		return SENSOR_IFACE_GET_FLAGS;
	}

	if(state){
		interface.ifr_flags |= IFF_PROMISC;
	}
	else{
		interface.ifr_flags &= ~IFF_PROMISC;
	}

	//setting flags
	if(ioctl(sock, SIOCSIFFLAGS, &interface) == -1){
		return SENSOR_IFACE_SET_FLAGS;
	}

	return SENSOR_SUCCESS;
}

int BindRawSocketToInterface(int sock, char *interfaceName){
	if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, interfaceName, strlen(interfaceName))) {
		return SENSOR_BIND_SOCKET;
	}
	return SENSOR_SUCCESS;

}



int sensor_prepare_loop(sensor_t *config){
	config->sock = create_socket();
	if (!config->sock) {
		return SENSOR_CREATE_SOCKET;
	}

	if (config->opt.promiscuous){
		int res;
		if (!(res = set_iface_promiscuous(config->sock, config->opt.device_name, true)))
			return res;
	}

	return 0;
}

int sensor_destroy(sensor_t *config){
	close_socket(config->sock);
	queue_destroy(&config->captured);
	queue_destroy(&config->dissected);
	if (config->opt.promiscuous){
		int res;
		if (!(res = set_iface_promiscuous(config->sock, config->opt.device_name, true)))
			return res;
	}
	return 0;
}

//-----------------interfaces---------------------
sensor_t sensor_init(){
	sensor_options_t options = {0,SENSOR_DEFAULT_PROMISC,SENSOR_DEFAULT_READ_BUFFER_SIZE, SENSOR_DEFAULT_TIMEOUT};
	struct sensor result;
	result.activated = false;
	result.sock = 0;
	result.opt = options;
	result.dissect_function = sensor_empty;
	result.persist_function = sensor_empty;
	return result;
}

int sensor_set_options(sensor_t *config, char *device, bool is_promisc, uint32_t buffersize, uint8_t capture_timeout){
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->opt.device_name = device;
	config->opt.promiscuous = is_promisc;
	config->opt.buffersize = buffersize;
	config->opt.timeout = capture_timeout;
	return 0;
}

int sensor_set_dissection_simple(sensor_t *config){
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->dissect_function = sensor_dissect_simple;
	return 0;
}

int sensor_loop(sensor_t *config, sensor_persist_f callback){
	if (config->activated) {
		return SENSOR_ALREADY_ACTIVATED;
	}
	config->activated = true;
	int res;
	if ((res = sensor_prepare_loop(config))) {
		return res;
	}

	//--------------------------
	struct timeval timeout;
	timeout.tv_sec = config->opt.timeout;
	timeout.tv_usec = 0;

	fd_set readset;
	FD_ZERO(&readset);
	FD_SET(config->sock, &readset);

	int buflength = config->opt.buffersize;
	uint8_t *buffer = malloc(buflength);

	while(config->activated && !config->captured.length && !config->dissected.length){
		// complete queue if we broke the loop
		if(config->activated){
			// wait for packet for given timeout and then read it
			select(FD_SETSIZE, &readset, NULL, NULL, &timeout);
			if(FD_ISSET(config->sock, &readset)){
				int read_len = read(config->sock, buffer, buflength);
				if(read_len > 0){
					queue_push(&config->captured, buffer, read_len);
				}
			}
		}
		config->dissect_function(&config->captured, &config->dissected);
		config->persist_function(&config->dissected);
	} /* while */
	sensor_destroy(config);
	return SENSOR_SUCCESS;
}

void sensor_breakloop(sensor_t *config){
	config->activated = false;
}

