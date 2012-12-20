/*
 * sensor_watcher.cpp
 *
 *  Created on: 11.12.2012
 *      Author: reiver
 */

#include "sensor_watcher.hpp"


SensorWatcher* SensorWatcher::createInstance(SessionContext *sc, int sock) {
	SensorWatcher *watcher = new SensorWatcher();

	watcher->context = sc;
	watcher->fd = sock;

	watcher->inbuf = new SockBuffer(4096);
	watcher->outbuf = new SockBuffer(4096);

	watcher->net_watcher.set(watcher->fd);
	watcher->net_watcher.set<SensorWatcher, &SensorWatcher::callback>(watcher);
	watcher->net_watcher.set(EV_READ);

	return watcher;

}

SensorWatcher::SensorWatcher() : context(), fd(), inbuf(), outbuf() {
	//
}

SensorWatcher::~SensorWatcher() {
	delete inbuf;
	delete outbuf;
}

void SensorWatcher::start() {
	net_watcher.start();
}

void SensorWatcher::stop() {
	net_watcher.stop();
}

void SensorWatcher::write_callback(ev::io &watcher, int events) {

}


void SensorWatcher::read_callback(ev::io &watcher, int events) {

}

void SensorWatcher::callback(ev::io &watcher, int events) {
	if (EV_ERROR & events) {
		delete this;
	}

	if (events & EV_READ) {
		read_callback(watcher, events);
	}

	if (events & EV_WRITE) {
		write_callback(watcher, events);
	}
}
