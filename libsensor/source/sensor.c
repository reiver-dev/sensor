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
// protocol header defines
#include <netinet/ether.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
// replace for net/if.h
#include <linux/if.h>
// local
#include "debug.h"
#include "protocols.h"
#include "sensor.h"

#define SENSOR_DEFAULT_READ_BUFFER_SIZE 65536
#define SENSOR_DEFAULT_TIMEOUT 1
#define SENSOR_DEFAULT_PROMISC false
//-------------------------------

typedef int (*sensor_dissect_f)(uint8_t* packet, int length, Queue_t in, Queue_t out);

typedef struct {
	char *device_name;
	bool promiscuous;
	uint32_t buffersize;
	uint8_t timeout;
} sensor_options_t;

struct sensor{
	bool activated;
	bool loop;
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

sensor_t sensor_init(){
	sensor_options_t options = {0,SENSOR_DEFAULT_PROMISC,SENSOR_DEFAULT_READ_BUFFER_SIZE, SENSOR_DEFAULT_TIMEOUT};
	struct sensor result = {false, false, options, queue_init(), queue_init(), *sensor_empty, *sensor_empty};
	return result;
}

int sensor_set_options(sensor_t config, char *device, bool is_promisc, uint8_t capture_timeout);
int sensor_set_dissection_simple(sensor_t config);
int sensor_loop(sensor_t config, sensor_persist_f callback);
int sensor_breakloop(sensor_t config);
int sensor_destroy(sensor_t config);













int create_socket() {
	/*
	 * Packet family
	 * Linux specific way of getting packets at the dev level
	 * Capturing every packet
	 */
	int sock = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ALL));
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
		return 1;
	}

	if(state){
		interface.ifr_flags |= IFF_PROMISC;
	}
	else{
		interface.ifr_flags &= ~IFF_PROMISC;
	}

	//setting flags
	if(ioctl(sock, SIOCSIFFLAGS, &interface) == -1){
		return 1;
	}

	return 0;
}


void PrintPacketInHex(unsigned char *packet, int len) {
	unsigned char *p = packet;

	printf("\n\n--------Packet---Starts-----\n\n");

	while(len--) {
		printf("%.2x ", *p);
		p++;
	}

	printf("\n\n--------Packet---Ends-----\n\n");

}



int get_next_packet(int sock, int seconds){
	fd_set readset;
	struct timeval timeout;

	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;

	FD_ZERO(&readset);
	FD_SET(sock, &readset);

	select(FD_SETSIZE, &readset, NULL, NULL, &timeout);
	if(FD_ISSET(sock, &readset)){
		uint8_t* buffer = malloc(SENSOR_DEFAULT_READ_BUFFER_SIZE);
		int count = read(sock, buffer, SENSOR_DEFAULT_READ_BUFFER_SIZE);
		if(count > 0){
			dissect(buffer, count);
		}
		free(buffer);
	}

	return 0;
}


void macToStr(uint8_t mac[6]){
	int i;
	for(i=0; i<6;i++)
		printf("%02X:", mac[i]);
}


//--------------actual dissection-----------------
uint8_t dissect(uint8_t* packet, int length){
	DEBUG_PRINT("\n---------------\n");
	struct EthernetHeader *ethernet = (struct EthernetHeader*) (packet);

	int size_ethernet = sizeof(struct EthernetHeader);
	macToStr(ethernet->sourceHost);
	DEBUG_PRINT("\nETHERNET-TYPE: %04X\n", ethernet->type);
	switch (ethernet->type){
		case 8:
			DEBUG_PRINT("%s\n","IP");
			struct Ip4Header *ipheader = (struct Ip4Header*) (packet + size_ethernet);
			int size_ip = IP4_HEADER(ipheader->version_headerLen) * 4;
			DEBUG_PRINT("Source ip:%d\nDest ip:%d\n", ipheader->sourceAddress.s_addr, ipheader->destAddress.s_addr);
			DEBUG_PRINT("PROTOCOL:%d\n", ipheader->protocol);
			switch (ipheader->protocol){
				case IPPROTO_TCP:
					DEBUG_PRINT("%s\n","Protocol TCP");
					struct TcpHeader *tcpheader = (struct TcpHeader*) (packet + size_ethernet + size_ip);
					DEBUG_PRINT("Source port:%d\nDest port:%d\n", tcpheader->sourcePort, tcpheader->destPort);
					break;
				case IPPROTO_UDP:
					DEBUG_PRINT("%s\n","Protocol UDP");
					struct UdpHeader *udpheader = (struct UdpHeader*) (packet + size_ethernet + size_ip);
					DEBUG_PRINT("Source port:%d\nDest port:%d\n", udpheader->sourcePort, udpheader->destPort);
					break;
				case IPPROTO_ICMP:
					DEBUG_PRINT("%s\n","Protocol ICMP");
					struct Icmp *icmp = (struct Icmp*) (packet + size_ethernet + size_ip);
					DEBUG_PRINT("Type:%d\nCode:%d\n", icmp->type, icmp->code);
					break;
			}
			break;
	}


	return 0;
}




