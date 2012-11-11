#ifndef MEMBER_MSG_QUEUE_HPP_
#define MEMBER_MSG_QUEUE_HPP_

#include <future>
#include "int_mpsc_pipe.hpp"
#include "member_command.hpp"

namespace mq {

template<class ThreadClass>
class MemberMessageQueue {
protected:

	typedef IntMpscPipe::Node Node;
	typedef AMemberCommand<ThreadClass, Node> ACommand;

	ACommand *getcommand(Node *node) {
		return static_cast<ACommand *>(node->self);
	}

	bool running;
	IntMpscPipe *pipe;
	ThreadClass *worker;

public:

	static void *start(void *arg) {
		MemberMessageQueue *mq = (MemberMessageQueue *) arg;
		mq->run();
		return NULL;
	}

	MemberMessageQueue(ThreadClass *worker)
	: running(false), pipe(new IntMpscPipe()), worker(worker) {

	};

	~MemberMessageQueue() {
		delete pipe;
	}

	template<typename RESULT, typename ...ARG>
	void send(RESULT (ThreadClass::*func)(ARG...), ARG&&... arg) {
		auto mes =
			new MemberCommand<ThreadClass, Node, RESULT, ARG...>(worker, func, std::forward<ARG>(arg)...);
		mes->node.self = mes;
		auto future = mes->prms.get_future();
		pipe->send(&mes->node);
		return future;
	}

	template<typename RESULT, typename ...ARG>
	std::future<RESULT> request(RESULT (ThreadClass::*func)(ARG...), ARG&&... arg) {
		auto mes =
			new MemberCommand<ThreadClass, Node, RESULT, ARG...>(worker, func, std::forward<ARG>(arg)...);
		mes->node.self = mes;
		auto future = mes->prms.get_future();
		pipe->send(&mes->node);
		return future;
	}

	bool receive() {
		bool result = false;
		Node *node = pipe->recv();
		if (node) {
			ACommand *cmd = getcommand(node);
			if (cmd) {
				cmd->call();
				result = true;
				delete cmd;
			} else {
				delete node;
			}
		}
		return result;
	}

	void stop() {
		running = false;
		Node *nullmsg = new Node();
		nullmsg->self = nullptr;
		pipe->send(nullmsg);
	}


	void run() {
		if (running) {
			return;
		} else {
			running = true;
		}
		while (running) {
			Node *node = pipe->recv();
			if (node) {

				ACommand *cmd = getcommand(node);
				if (cmd) {
					cmd->call();
					delete cmd;
				} else {
					delete node;
					running = false;
				}

			} else {
				running = false;
			}
		}
	}


};

}

#endif /* MEMBER_MSG_QUEUE_HPP_ */
