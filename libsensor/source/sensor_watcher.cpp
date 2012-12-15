/*
 * sensor_watcher.cpp
 *
 *  Created on: 11.12.2012
 *      Author: reiver
 */

#include "sensor_watcher.hpp"

SensorWatcher::SensorWatcher(SessionContext *sc, int sock) : context(sc), fd(sock) {
	set_nonblocking(fd);
	net_watcher.set<SensorWatcher, &SensorWatcher::callback>(this);
	net_watcher.start(fd, ev::READ | ev::WRITE);
}



SensorWatcher::~SensorWatcher() {
	// TODO Auto-generated destructor stub
}


void SensorWatcher::write_callback(ev::io &watcher, int events) {

}


void SensorWatcher::read_callback(ev::io &watcher, int events) {

}

void SensorWatcher::callback(ev::io &watcher, int events) {
	if (EV_ERROR & events) {
		return;
	}

	if (events & EV_READ)
		read_callback(watcher, events);

	if (events & EV_WRITE)
		write_callback(watcher, events);

}
