#define READ_BUFFER_SIZE 4096

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>

#include <netinet/in.h>
#include <linux/if_ether.h>

#include <fcntl.h>

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
			PrintPacketInHex(buffer, count);
		}
		free(buffer);
	}

	return 0;
}






