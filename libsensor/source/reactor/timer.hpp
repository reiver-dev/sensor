#ifndef TIMER_HPP_
#define TIMER_HPP_

#include "event_loop.hpp"
#include "callback.hpp"

namespace net {

class Timer {
public:

	void setCallback(CB::Callback<void ()> callback) {
		cb = callback;
	}

	void shedule(EventLoop *loop, double delay, double repeat) {
		ev_timer_init(&ev, timerCallback, delay, repeat);
		ev_timer_start(loop->get_loop(), &ev);
	}

	void stop(EventLoop *loop) {
		ev_timer_stop(loop->get_loop(), &ev);
	}

private:

	static void timerCallback(EV_P_ ev_timer *handler, int event);

	ev_timer ev;
	CB::Callback<void ()> cb;
};

}

#endif /* TIMER_HPP_ */
