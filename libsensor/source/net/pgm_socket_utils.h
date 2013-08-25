#ifndef PGM_SOCKET_UTILS_HPP_
#define PGM_SOCKET_UTILS_HPP_

#include <stdbool.h>
#include <pgm/pgm.h>


#ifdef __cplusplus
extern "C" {
#endif

struct pgm_send_fds {
	int send;
	int receive;
	int repair;
	int pending;
};

struct pgm_recv_fds {
	int receive;
	int pending;
};

pgm_sock_t *create_pgm_socket(char *interface, char *multicastaddr, uint16_t port, bool encap);
bool set_pgm_sender(pgm_sock_t *sock);
bool set_pgm_receiver(pgm_sock_t *sock);
uint64_t get_pgm_rate_timeout(pgm_sock_t *sock);
uint64_t get_pgm_timeout(pgm_sock_t *sock);
bool get_pgm_send_fd(pgm_sock_t *sock, struct pgm_send_fds *send_fds);
bool get_pgm_recv_fd(pgm_sock_t *sock, struct pgm_recv_fds *recv_fds);
uint64_t pgm_sender_push_fsm(pgm_sock_t *sock);
size_t pgm_get_max_tsdu(pgm_sock_t *sock);

#ifdef __cplusplus
}
#endif


#endif /* PGM_SOCKET_UTILS_HPP_ */
