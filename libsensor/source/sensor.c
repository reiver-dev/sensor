#define READ_BUFFER_SIZE 4096
//Standart Libraries
#include <stdlib.h>
#include <stdio.h>
//Additional libs
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
//For packate capture
#include <sys/select.h>
//For interface flags change
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <linux/if.h>

#include "debug.h"
#include "sensor.h"


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
		uint8_t* buffer = malloc(READ_BUFFER_SIZE);
		int count = read(sock, buffer, READ_BUFFER_SIZE);
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




