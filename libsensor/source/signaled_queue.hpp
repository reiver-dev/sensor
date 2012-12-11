#ifndef SIGNALED_QUEUE_HPP_
#define SIGNALED_QUEUE_HPP_

#include <sys/eventfd.h>
#include <assert.h>

#include <intqueue/int_mpsc_queue.hpp>
#include <cmd/command.hpp>

#include "socket_utils.hpp"

namespace mq {

class SignaledQueue {
public:
	typedef int fd_t;

	SignaledQueue() : descriptor(create_fd) {
		//
	}

	~SignaledQueue() {
		close(fd);
	}

	fd_t get_fd() {
		return descriptor;
	}

	template<typename FUNC, typename RESULT, typename ...ARG>
	void send(FUNC func, ARG&&... arg) {
		auto mes =
			new Command<Node, FUNC, RESULT, ARG...>(func, std::forward<ARG>(arg)...);
		mes->get_node()->self = mes;
		queue.push(mes->get_node());
		send(descriptor);
	}

	template<typename FUNC, typename RESULT, typename ...ARG>
	mq::future<RESULT> request(FUNC func, ARG&&... arg) {
		auto mes =
			new RequestCommand<Node, FUNC, RESULT, ARG...>(func, std::forward<ARG>(arg)...);
		mes->get_node()->self = mes;
		auto future = mes->get_future();
		queue.push(mes->get_node());
		send(descriptor);
		return future;
	}

	int receive() {
		int times = recv(descriptor);
		int actual_times = times;
		for (int i = 0; i < times; ++i) {
			for (;;) {
				Node *node = queue.pop();
				if (!node) {
					continue;
				}
				ACommand *cmd = getcommand(node);
				if (cmd) {
					cmd->call();
					delete cmd;
					actual_times++;
				} else {
					delete node;
				}
				break;
			}
		}
		return times;
	}

private:

	fd_t descriptor;

	IntMpscQueue queue;

	typedef IntMpscQueue::Node Node;
	typedef AbstractCommand<Node> ACommand;

	ACommand *getcommand(Node *node) {
		return static_cast<ACommand *>(node->self);
	}

	static void send(fd_t descr) {
		const uint64_t inc = 1;
		ssize_t sz = write(descr, &inc, sizeof(inc));
	}

	static int recv(fd_t descr) {
		uint64_t ret;
		ssize_t sz = read(descr, &ret, sizeof (ret));
		assert(sz == sizeof(ret));
		return ret;
	}

	static int create_fd() {
		fd_t fd = eventfd(0, 0);
		set_nonblocking(fd);
		return fd;
	}
};

} /* namespace mq */
#endif /* SIGNALED_QUEUE_HPP_ */
