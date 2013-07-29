#ifndef SENSOR_WATCHER_HPP_
#define SENSOR_WATCHER_HPP_

#include <ev++.h>
#include <memory>

#include "net/socket_buffer.hpp"
#include "session_context.hpp"

class SensorWatcher {
public:

	static SensorWatcher* createInstance(SessionContext *context, int sock);
	~SensorWatcher();

	void start();
	void stop();

private:
	SensorWatcher();

	void connect_callback(ev::io &watcher, int events);
	void read_callback(ev::io &watcher, int events);
	void write_callback(ev::io &watcher, int events);
	void callback(ev::io &watcher, int events);
	void prepare_receive();

	SessionContext *context;

	int fd;
	ev::io net_watcher;

	CircularBuffer *inbuf;
	CircularBuffer *outbuf;

};

#endif /* SENSOR_WATCHER_HPP_ */
