#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "socket_utils.hpp"
#include "debug.hpp"

/*
 * Packet family
 * Linux specific way of getting packets at the dev level
 * Capturing every packet
 */
int create_raw_socket() {
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
		return 1;
	}

	if (state) {
		interface.ifr_flags |= IFF_PROMISC;
	} else {
		interface.ifr_flags &= ~IFF_PROMISC;
	}

	//setting flags
	if (ioctl(sock, SIOCSIFFLAGS, &interface) == -1) {
		DERROR("%s\n", "set interface flags failed");
		return 1;
	}

	return 0;
}

int set_socket_timeout(int sock, int seconds) {
	struct timeval timeout;
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	int res = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	DINFO("Set socket timeout, result: %d\n", res);
	return res;
}

int bind_raw_socket_to_interface(int sock, char *interfaceName) {

	struct ifreq interface;
	struct sockaddr_ll address = {0};

	strcpy(interface.ifr_name, interfaceName);

	// Get interface index
	if (ioctl(sock, SIOCGIFINDEX, &interface) == -1) {
		DERROR("%s\n", "get interface flags failed");
		return 1;
	}

	// Bind raw socket to interface

	address.sll_family   = AF_PACKET;
	address.sll_ifindex  = interface.ifr_ifindex;
	address.sll_protocol = htons(ETH_P_ALL);

	if (bind(sock, (struct sockaddr *)&address, sizeof(address)) == -1) {
		DERROR("Bind socket to interface failed: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

int set_nonblocking(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		flags = 0;

	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int create_tcp_server(const char *address, const char *port) {
	struct addrinfo hints;
	struct addrinfo *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int rc = getaddrinfo(address, port, &hints, &result);
	if (rc == 0) {
		DERROR("%s\n", gai_strerror(rc));
	}

	int sock = socket(result->ai_family, result->ai_socktype, 0);

	if (sock != -1)
		bind(sock, result->ai_addr, result->ai_addrlen);
	else
		DERROR("%s\n", strerror(errno));

	freeaddrinfo(result);
	return sock;
}

int create_tcp_connect(const char *address, const char *port) {
	struct addrinfo hints;
	struct addrinfo *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int rc = getaddrinfo(address, port, &hints, &result);
	if (rc == 0) {
		DERROR("%s\n", gai_strerror(rc));
	}

	int sock = socket(result->ai_family, result->ai_socktype, 0);

	if (sock != -1)
		connect(sock, result->ai_addr, result->ai_addrlen);
	else
		DERROR("%s\n", strerror(errno));

	freeaddrinfo(result);
	return sock;
}


