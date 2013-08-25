#ifndef PGM_TRANSPORT_HPP_
#define PGM_TRANSPORT_HPP_

#include <unordered_map>

#include <assert.h>

#include "net/pgm_socket_utils.h"
#include "reactor/reactor.hpp"
#include "base/internet_address.hpp"

#define INIT_EV_LISTENER(lst, cb, fd, ev) { ev_init(&lst, cb); ev_io_set(&lst, fd, ev); lst.data = this; }

class PgmSender {
public:

	~PgmSender() {
		if (m_socket)
			pgm_close(m_socket, true);
	}

	void initialize(char *deviceName, char *multicastAddr, uint16_t port) {
		m_socket = create_pgm_socket(deviceName, multicastAddr, port, false);
		set_pgm_sender(m_socket);

		pgm_send_fds fds;

		get_pgm_send_fd(m_socket, &fds);

		INIT_EV_LISTENER(m_send, send_event, fds.send, EV_WRITE);
		INIT_EV_LISTENER(m_send_complete, recv_event, fds.receive, EV_READ);
		INIT_EV_LISTENER(m_send_repair, recv_event, fds.repair, EV_READ);
		INIT_EV_LISTENER(m_send_pending, recv_event, fds.pending, EV_READ);

		ev_init(&m_recv_timeout, timer_event);
		m_recv_timeout.data = this;

		ev_init(&m_send_timeout, timer_event);
		m_send_timeout.data = this;

	}

	void start(net::EventLoop *loop) {
		assert(loop);
		reactor = loop;

		ev_io_start(reactor->get_loop(), &m_send);
		ev_io_start(reactor->get_loop(), &m_send_complete);
		ev_io_start(reactor->get_loop(), &m_send_repair);
		ev_io_start(reactor->get_loop(), &m_send_pending);
	}

	void stop() {
		ev_io_stop(reactor->get_loop(), &m_send);
		ev_io_stop(reactor->get_loop(), &m_send_complete);
		ev_io_stop(reactor->get_loop(), &m_send_repair);
		ev_io_stop(reactor->get_loop(), &m_send_pending);

		ev_timer_stop(reactor->get_loop(), &m_send_timeout);
		ev_timer_stop(reactor->get_loop(), &m_recv_timeout);
	}

private:

	static void send_event(EV_P_ ev_io *handler, int events) {
		PgmSender *self = static_cast<PgmSender*>(handler->data);

		if (ev_is_active(&self->m_send_timeout)) {
			ev_timer_stop(EV_A_ &self->m_send_timeout);
		}

		if (!self->m_buffer.pending()) {
			ev_io_stop(EV_A_&self->m_send);
			return;
		}

		char *buf = self->m_buffer.readPtr();
		size_t bufLen = self->m_buffer.pending();
		size_t written = 0;

		int status = pgm_send(self->m_socket, buf, bufLen, &written);

		if (written == bufLen) {
			assert(status == PGM_IO_STATUS_NORMAL);
			self->m_buffer.reset();
		} else {
			if (status == PGM_IO_STATUS_RATE_LIMITED) {
				uint64_t timeout = get_pgm_rate_timeout(self->m_socket);
				ev_timer_set(&self->m_send_timeout, timeout, 0);
			} else {
				assert(status == PGM_IO_STATUS_WOULD_BLOCK);
			}
		}
	}

	static void recv_event(EV_P_ ev_io *handler, int events) {
		PgmSender *self = static_cast<PgmSender*>(handler->data);

		if (ev_is_active(&self->m_recv_timeout)) {
			ev_timer_stop(EV_A_ &self->m_recv_timeout);
		}

		uint64_t timeout = pgm_sender_push_fsm(self->m_socket);

		if (timeout > 0) {
			ev_timer_set(&self->m_recv_timeout, timeout, 0);
			ev_timer_start(EV_A_ &self->m_recv_timeout);
		}
	}

	static void timer_event(EV_P_ ev_timer *handler, int events) {
		PgmSender *self = static_cast<PgmSender*>(handler->data);

		if (ev_is_active(&self->m_recv_timeout)) {
			recv_event(EV_A_ &self->m_send_repair, events);
		}

		if (ev_is_active(&self->m_send_timeout)) {
			send_event(EV_A_ &self->m_send, events);
		}

	}

	ev_io m_send;
	ev_io m_send_repair;
	ev_io m_send_complete;
	ev_io m_send_pending;

	ev_timer m_recv_timeout;
	ev_timer m_send_timeout;

	pgm_sock_t *m_socket;

	net::Buffer m_buffer;
	net::EventLoop *reactor;

};


class PgmReceiver {
public:

	const size_t in_batch_size = 1024;

	void initialize(char *deviceName, char *multicastAddr, uint16_t port) {
		m_receiver = create_pgm_socket(deviceName, multicastAddr, port, false);
		set_pgm_receiver(m_receiver);

		pgm_recv_fds fds;

		get_pgm_recv_fd(m_receiver, &fds);

		INIT_EV_LISTENER(m_recv, recv_event, fds.receive, EV_READ);
		INIT_EV_LISTENER(m_recv_pending, recv_event, fds.pending, EV_READ);

		ev_init(&m_recv_timeout, timeout_event);
		m_recv_timeout.data = this;

		size_t max_tsdu_size = pgm_get_max_tsdu(m_receiver);
		pgm_msgv_len = (int) in_batch_size / max_tsdu_size;
		if ((int) in_batch_size % max_tsdu_size) {
			pgm_msgv_len++;
		}

		pgm_msgv = (pgm_msgv_t*) malloc(sizeof(pgm_msgv_t) * pgm_msgv_len);
	}

	void start(net::EventLoop *loop) {
		assert(loop);
		reactor = loop;

		ev_io_start(reactor->get_loop(), &m_recv);
		ev_io_start(reactor->get_loop(), &m_recv_pending);

	}

	void stop() {
		ev_io_stop(reactor->get_loop(), &m_recv);
		ev_io_stop(reactor->get_loop(), &m_recv_pending);
		ev_timer_stop(reactor->get_loop(), &m_recv_timeout);
	}

	~PgmReceiver() {
		if (m_receiver)
			pgm_close(m_receiver, false);
	}

private:

	bool read_msg(void **data, size_t *received, pgm_tsi_t **tsi) {

		if (bytes_processed == bytes_received && bytes_received > 0) {
			bytes_received = 0;
			bytes_processed = 0;
			messages_processed = 0;
			return 0;
		}

		if (bytes_processed == bytes_received) {

			pgm_error_t *pgm_error;
			const int status = pgm_recvmsgv(m_receiver, pgm_msgv, pgm_msgv_len, MSG_ERRQUEUE, &bytes_received, &pgm_error);

			if (status == PGM_IO_STATUS_TIMER_PENDING) {
				uint64_t timeout = get_pgm_timeout(m_receiver);
				ev_timer_set(&m_recv_timeout, timeout, 0);
				bytes_received = 0;
				return true;
			}

			if (status == PGM_IO_STATUS_RATE_LIMITED) {
				uint64_t timeout = get_pgm_rate_timeout(m_receiver);
				ev_timer_set(&m_recv_timeout, timeout, 0);
				bytes_received = 0;
				return true;
			}

			if (status == PGM_IO_STATUS_RESET) {
				struct pgm_sk_buff_t* skb = pgm_msgv[0].msgv_skb[0];
				*tsi = &skb->tsi;
				pgm_free_skb(skb);
				return false;
			}

		}

		assert(bytes_received > 0);
		assert(pgm_msgv[messages_processed].msgv_len == 1);

		struct pgm_sk_buff_t *skb = pgm_msgv[messages_processed].msgv_skb[0];

		*data = &skb->data;
		*received = skb->len;
		*tsi = &skb->tsi;

		messages_processed++;
		bytes_processed += skb->len;

		return true;
	}

	static void recv_event(EV_P_ ev_io *handler, int events) {
		PgmReceiver *self = static_cast<PgmReceiver*>(handler->data);

		if (ev_is_active(&self->m_recv_timeout)) {
			ev_timer_stop(self->reactor->get_loop(), &self->m_recv_timeout);
		}

		while (true) {

			char *data;
			size_t received;
			pgm_tsi_t *tsi;

			bool rc = self->read_msg((void**)&data, &received, &tsi);

			// timeout
			if (rc && received == 0) {
				break;
			}

			// peer disconnect
			if (!rc) {
				//TODO Data loss callback
				break;
			}

			break;
		}
	}

	static void timeout_event(EV_P_ ev_timer *handler, int events) {
		PgmReceiver *self = static_cast<PgmReceiver*>(handler->data);

		if (ev_is_active(&self->m_recv_timeout)) {
			recv_event(EV_A_ &self->m_recv, events);
		}
	}

	ev_io m_recv;
	ev_io m_recv_pending;
	ev_timer m_recv_timeout;

	pgm_sock_t *m_receiver;

	pgm_msgv_t *pgm_msgv;
	size_t pgm_msgv_len;

	size_t bytes_received;
	size_t bytes_processed;
	size_t messages_processed;

	struct get_tsi {
		std::size_t operator()(const pgm_tsi_t& tsi) const {
			std::size_t hash;
			memcpy(&hash, &tsi, sizeof(tsi));
			return hash;
		}
	};

	struct peer_info_t {
		bool active;
		InternetAddress addr;
	};

	typedef std::unordered_map<pgm_tsi_t, peer_info_t, get_tsi> peers_t;
	peers_t peers;

	net::Buffer m_buffer;
	net::EventLoop *reactor;
};


#endif /* PGM_TRANSPORT_HPP_ */
