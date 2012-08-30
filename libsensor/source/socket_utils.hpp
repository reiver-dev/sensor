#ifndef SOCKET_UTILS_H_
#define SOCKET_UTILS_H_

#include <stdbool.h>

int create_raw_socket();
int close_socket(int socket);
int set_iface_promiscuous(int sock, const char* interfaceName, bool state);
int set_socket_timeout(int sock, int seconds);
int bind_socket_to_interface(int sock, char *interfaceName);
int setNonblocking(int fd);

#endif /* SOCKET_UTILS_H_ */
