#ifndef SOCKET_UTILS_H_
#define SOCKET_UTILS_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int create_raw_socket();
int bind_raw_socket_to_interface(int sock, char *interfaceName);
int set_iface_promiscuous(int sock, const char* interfaceName, bool state);
int close_socket(int socket);
int set_socket_timeout(int sock, int seconds);
int set_nonblocking(int fd);
int create_socket_server(int socket_type, const char *address, const char *port);
int create_tcp_connect(const char *address, const char *port);


#ifdef __cplusplus
}
#endif

#endif /* SOCKET_UTILS_H_ */
