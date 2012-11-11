#ifndef MSGQUEUE_H_
#define MSGQUEUE_H_

#include "int_mpsc_pipe.hpp"
#include "command.hpp"

namespace mq {

class MessageQueue {
protected:

	typedef IntMpscPipe::Node Node;
	typedef AbstractCommand<Node> ACommand;

	ACommand *getcommand(Node *node) {
		return static_cast<ACommand *>(node->self);
	}

	bool running;
	IntMpscPipe *pipe;

public:

	static void *start(void *arg) {
		MessageQueue *mq = (MessageQueue *) arg;
		mq->run();
		return NULL;
	}

	MessageQueue()
	: running(false), pipe(new IntMpscPipe()) {

	};

	~MessageQueue() {
		delete pipe;
	}

	template<typename RESULT, typename ...ARG>
	void send(RESULT (*func)(ARG...), ARG&&... arg) {
		auto mes =
			new WaitlessCommand<Node, RESULT, ARG...>(func, std::forward<ARG>(arg)...);
		mes->node.self = mes;
		pipe->send(&mes->node);
	}

	template<typename RESULT, typename ...ARG>
	std::future<RESULT> request(RESULT (*func)(ARG...), ARG&&... arg) {
		auto mes =
			new Command<Node, RESULT, ARG...>(func, std::forward<ARG>(arg)...);
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


#endif /* MSGQUEUE_H_ */
