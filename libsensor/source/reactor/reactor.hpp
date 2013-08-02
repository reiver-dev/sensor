#ifndef REACTOR_HPP_
#define REACTOR_HPP_

#include <ev.h>

namespace net {

class Reactor {
public:

	typedef ev_io handle_t;

	Reactor() {
		m_eventLoop = ev_loop_new(0);
	}

	void set_io_events(handle_t *h, int events) {
		bool is_active = ev_is_active(h);
		if (is_active)
			ev_io_stop(m_eventLoop, h);
			ev_io_set(h, h->fd, events);
		if (is_active)
			ev_io_start(m_eventLoop, h);
	}

	void start() {
		ev_run(m_eventLoop, 0);
	}

	void stop() {
		ev_break(m_eventLoop);
	}

	struct ev_loop *get_loop() {
		return m_eventLoop;
	}

private:
	struct ev_loop *m_eventLoop;
};

}

#endif /* REACTOR_HPP_ */

