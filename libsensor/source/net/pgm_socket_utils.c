#include "pgm_socket_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "base/random.h"
#include "base/debug.h"

static inline void process_pgm_error(pgm_error_t *error) {
	if (!error) {
		DERROR("PGM error: %s", "invalid arguments");
	} else  {
		DERROR("PGM error: %s", error->message);
	}
}

pgm_sock_t *create_pgm_socket(char *interface, char *multicastaddr, uint16_t port, bool encap) {

	struct {
		bool error, sock_init;
	} state = { true, false };

	pgm_error_t *pgm_error = NULL;
	struct pgm_addrinfo_t *addrinfo = NULL;

	char network[256] = { 0 };
	snprintf(network, sizeof(network),"%s;%s", interface, multicastaddr);

	pgm_sock_t *sock = NULL;
	int proto = encap ? IPPROTO_UDP : IPPROTO_PGM;

	for (;;) {
		if (!pgm_getaddrinfo(network, NULL, &addrinfo, &pgm_error)) {
			process_pgm_error(pgm_error);
			break;
		}

		sa_family_t family = addrinfo->ai_send_addrs->gsr_group.ss_family;
		if (!pgm_socket(&sock, family, SOCK_SEQPACKET, proto, &pgm_error)) {
			process_pgm_error(pgm_error);
			break;
		}
		state.sock_init = true;

		if (encap) {
			const int _p = port;
			if (!pgm_setsockopt(sock, IPPROTO_PGM, PGM_UDP_ENCAP_UCAST_PORT, &_p, sizeof(_p))
			    || !pgm_setsockopt(sock, IPPROTO_PGM, PGM_UDP_ENCAP_MCAST_PORT, &_p, sizeof(_p))) {
				DERROR("PGM error: can't set UDP port (%i)", port);
				break;
			}
		}

		// generate global session id
		struct pgm_sockaddr_t addr = { 0 };
		addr.sa_port = port;
		addr.sa_addr.sport = DEFAULT_DATA_SOURCE_PORT;
		uint64_t rnd = random_get64();
		if (!pgm_gsi_create_from_data(&addr.sa_addr.gsi, (uint8_t *)&rnd, sizeof(rnd))) {
			DERROR("PGM error: can't set GSI (%i)", rnd);
			break;
		}

		struct pgm_interface_req_t if_req;
		memset (&if_req, 0, sizeof(if_req));
		if_req.ir_interface = addrinfo->ai_recv_addrs[0].gsr_interface;
		if_req.ir_scope_id  = 0;
		if (AF_INET6 == family) {
			struct sockaddr_in6 sa6;
			memcpy (&sa6, &addrinfo->ai_recv_addrs[0].gsr_group, sizeof(sa6));
			if_req.ir_scope_id = sa6.sin6_scope_id;
		}
		if (!pgm_bind3(sock, &addr, sizeof(addr), &if_req, sizeof(if_req), &if_req, sizeof(if_req), &pgm_error)) {
			process_pgm_error(pgm_error);
			break;
		}

		//  Join IP multicast groups.
		for (unsigned i = 0; i < addrinfo->ai_recv_addrs_len; i++) {
			if (!pgm_setsockopt(sock, IPPROTO_PGM, PGM_JOIN_GROUP, &addrinfo->ai_recv_addrs[i], sizeof(struct group_req))) {
				DERROR("PGM error: join group failed (%i)", i);
				break;
			}
		}
		if (!pgm_setsockopt(sock, IPPROTO_PGM, PGM_SEND_GROUP, &addrinfo->ai_send_addrs[0], sizeof(struct group_req))) {
			DERROR("%s", "PGM error: send group failed");
			break;
		}

		/* ip level parameters */
		const int blocking = 0;
		const int multicast_loop = 0;
		const int multicast_hops = 16;
		// Expedited Forwarding PHB for network elements, no ECN.
		const int dscp = 0x2e << 2;

		pgm_setsockopt (sock, IPPROTO_PGM, PGM_MULTICAST_LOOP, &multicast_loop, sizeof(multicast_loop));
		pgm_setsockopt (sock, IPPROTO_PGM, PGM_MULTICAST_HOPS, &multicast_hops, sizeof(multicast_hops));
		if (AF_INET6 != family)
			pgm_setsockopt (sock, IPPROTO_PGM, PGM_TOS, &dscp, sizeof(dscp));
		pgm_setsockopt (sock, IPPROTO_PGM, PGM_NOBLOCK, &blocking, sizeof(blocking));

		if (!pgm_connect(sock, &pgm_error)) {
			process_pgm_error(pgm_error);
			break;
		}

		state.error = false;
		break;
	}

	if (pgm_error) {
		pgm_error_free(pgm_error);
	}

	if (addrinfo) {
		pgm_freeaddrinfo(addrinfo);
	}

	if (state.error) {
		if (state.sock_init) {
			pgm_close(sock, true);
			sock = NULL;
		}
	}

	return sock;
}

bool set_pgm_sender(pgm_sock_t *sock) {
	const int send_only = 1;
	const int max_tdpu = 1500; // maximum transport data unit size
	const int sqns = 100; // send window size
	const int max_rte = 400*1000; // maximum transmit rate in bytes/second
	const int hops = 32; // number of network hops to cross (TTL)
	const int ambient_spm = pgm_secs (30); // interval of background SPM packets
	const int heartbeat_spm[] = { // intervals of data flushing SPM packets
		  pgm_msecs (100)
		, pgm_msecs (100)
		, pgm_msecs (100)
		, pgm_msecs (100)
		, pgm_msecs (1300)
		, pgm_secs  (7)
		, pgm_secs  (16)
		, pgm_secs  (25)
		, pgm_secs  (30)
	};

	bool ok = false;
	if (
	   pgm_setsockopt (sock, IPPROTO_PGM, PGM_SEND_ONLY, &send_only, sizeof(send_only))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_MTU, &max_tdpu, sizeof(max_tdpu))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_TXW_SQNS, &sqns, sizeof(sqns))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_TXW_MAX_RTE, &max_rte, sizeof(max_rte))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_MULTICAST_HOPS, &hops, sizeof(hops))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_AMBIENT_SPM, &ambient_spm, sizeof(ambient_spm))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_HEARTBEAT_SPM, &heartbeat_spm, sizeof(heartbeat_spm))
	) {
		ok = true;
	}

	return ok;
}

bool set_pgm_receiver(pgm_sock_t *sock) {
	const int recv_only = 1;
	const int passive = 0;
	const int max_tdpu = 1500; // maximum transport data unit size
	const int sqns = 100; // receive window size
	const int peer_expiry = pgm_secs (300); // timeout for removing a dead peer
	const int spmr_expiry = pgm_msecs (250); // expiration time of SPM Requests
	const int nak_bo_ivl = pgm_msecs (50); // NAK transmit back-off interval
	const int nak_rpt_ivl = pgm_secs (2); // timeout before repeating NAK
	const int nak_rdata_ivl = pgm_secs (2); // timeout for receiving RDATA
	const int nak_data_retries = 50; // retries for DATA packets after NAK
	const int nak_ncf_retries = 50; // retries for DATA after NCF

	bool ok = false;
	if (
	   pgm_setsockopt (sock, IPPROTO_PGM, PGM_RECV_ONLY, &recv_only, sizeof(recv_only))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_PASSIVE, &passive, sizeof(passive))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_MTU, &max_tdpu, sizeof(max_tdpu))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_RXW_SQNS, &sqns, sizeof(sqns))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_PEER_EXPIRY, &peer_expiry, sizeof(peer_expiry))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_SPMR_EXPIRY, &spmr_expiry, sizeof(spmr_expiry))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_NAK_BO_IVL, &nak_bo_ivl, sizeof(nak_bo_ivl))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_NAK_RPT_IVL, &nak_rpt_ivl, sizeof(nak_rpt_ivl))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_NAK_RDATA_IVL, &nak_rdata_ivl, sizeof(nak_rdata_ivl))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_NAK_DATA_RETRIES, &nak_data_retries, sizeof(nak_data_retries))
	&& pgm_setsockopt (sock, IPPROTO_PGM, PGM_NAK_NCF_RETRIES, &nak_ncf_retries, sizeof(nak_ncf_retries))
	) {
		ok = true;
	}
	return ok;
}

static inline uint64_t get_pgm_time(pgm_sock_t *sock, int option) {
	struct timeval tv = { 0 };
	socklen_t len = sizeof(tv);
	bool ok = pgm_getsockopt (sock, IPPROTO_PGM, option, &tv, &len);
	assert(ok);
	uint64_t result = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return result;
}

uint64_t get_pgm_rate_timeout(pgm_sock_t *sock) {
	assert(sock);
	return get_pgm_time(sock, PGM_RATE_REMAIN);
}

uint64_t get_pgm_timeout(pgm_sock_t *sock) {
	assert(sock);
	return get_pgm_time(sock, PGM_TIME_REMAIN);
}

bool get_pgm_send_fd(pgm_sock_t *sock, struct pgm_send_fds *send_fds) {
	assert(send_fds);
	struct pgm_send_fds fds = {0};
	socklen_t len = sizeof(fds.send);
	bool ok = true;
	ok = ok && pgm_getsockopt (sock, IPPROTO_PGM, PGM_SEND_SOCK, &fds.send, &len);
	ok = ok && pgm_getsockopt (sock, IPPROTO_PGM, PGM_RECV_SOCK, &fds.receive, &len);
	ok = ok && pgm_getsockopt (sock, IPPROTO_PGM, PGM_REPAIR_SOCK, &fds.repair, &len);
	ok = ok && pgm_getsockopt (sock, IPPROTO_PGM, PGM_PENDING_SOCK, &fds.pending, &len);
	if (ok) {
		*send_fds = fds;
	}
	return ok;
}

bool get_pgm_recv_fd(pgm_sock_t *sock, struct pgm_recv_fds *recv_fds) {
	assert(recv_fds);
	struct pgm_recv_fds fds = {0};
	socklen_t len = sizeof(fds.receive);
	bool ok = true;
	ok = ok && pgm_getsockopt (sock, IPPROTO_PGM, PGM_RECV_SOCK, &fds.receive, &len);
	ok = ok && pgm_getsockopt (sock, IPPROTO_PGM, PGM_PENDING_SOCK, &fds.pending, &len);
	if (ok) {
		*recv_fds = fds;
	}
	return ok;
}


uint64_t pgm_sender_push_fsm(pgm_sock_t *sock) {
	struct pgm_msgv_t dummy_msg;

	size_t length = 0;
	pgm_error_t *pgm_error = NULL;

	const int status = pgm_recvmsgv(sock, &dummy_msg, 1, MSG_ERRQUEUE, &length, &pgm_error);

	assert(status != PGM_IO_STATUS_ERROR);
	assert(length == 0);

	uint64_t timeout;
	switch(status) {
	case PGM_IO_STATUS_TIMER_PENDING:
		timeout = get_pgm_timeout(sock);
		DINFO("PGM - send pending for (%i)", timeout);
		break;
	case PGM_IO_STATUS_RATE_LIMITED:
		timeout = get_pgm_rate_timeout(sock);
		DINFO("PGM - rate limited for (%i)", timeout);
		break;
	default:
		timeout = 0;
		break;
	}

	return true;
}
