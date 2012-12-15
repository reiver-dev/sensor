#ifndef SENSOR_WATCHER_HPP_
#define SENSOR_WATCHER_HPP_

#include <ev++.h>

#include "session_context.hpp"

class SensorWatcher {
public:

	SensorWatcher(SessionContext *context, int sock);
	~SensorWatcher();

private:

	void connect_callback(ev::io &watcher, int events);
	void read_callback(ev::io &watcher, int events);
	void write_callback(ev::io &watcher, int events);
	void callback(ev::io &watcher, int events);


	SessionContext *context;

	int fd;
	ev::io net_watcher;

};

#endif /* SENSOR_WATCHER_HPP_ */
