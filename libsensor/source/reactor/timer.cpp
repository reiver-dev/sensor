#include "timer.hpp"


void net::Timer::timerCallback(EV_P_ ev_timer *handler, int event) {
	static_cast<net::Timer*>(handler->data)->cb();
}
