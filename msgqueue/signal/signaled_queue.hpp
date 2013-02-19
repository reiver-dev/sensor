#ifndef SIGNALED_QUEUE_HPP_
#define SIGNALED_QUEUE_HPP_


#include <assert.h>

#include "queue/int_mpsc_queue.hpp"
#include "cmd/command.hpp"
#include "signaler.hpp"


namespace mq {

class SignaledQueue {
public:
	typedef int fd_t;

	SignaledQueue() {
		//
	}

	const Signaler& signal() const {
		return signaler;
	}

	template<typename FUNC, typename RESULT, typename ...ARG>
	void send(FUNC func, ARG&&... arg) {
		auto mes =
			new Command<Node, FUNC, ARG...>(func, std::forward<ARG>(arg)...);
		mes->get_node()->self = mes;
		queue.push(mes->get_node());
		signaler.send();
	}

	template<typename FUNC, typename RESULT, typename ...ARG>
	mq::future<RESULT> request(FUNC func, ARG&&... arg) {
		auto mes =
			new RequestCommand<Node, FUNC, ARG...>(func, std::forward<ARG>(arg)...);
		mes->get_node()->self = mes;
		auto future = mes->get_future();
		queue.push(mes->get_node());
		signaler.send();
		return future;
	}

	int receive() {
		int times = signaler.recv();
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

	void nullmsg() {
		Node *nullmsg = new Node();
		nullmsg->self = nullptr;
		queue.push(nullmsg);
	}

private:

	typedef IntMpscQueue::Node Node;
	typedef AbstractCommand<Node> ACommand;

	Signaler signaler;
	IntMpscQueue queue;

	ACommand *getcommand(Node *node) {
		return static_cast<ACommand *>(node->self);
	}

};

} /* namespace mq */
#endif /* SIGNALED_QUEUE_HPP_ */
