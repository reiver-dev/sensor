#ifndef MSGQUEUE_H_
#define MSGQUEUE_H_

#include "intqueue/int_mpsc_pipe.hpp"
#include "cmd/command.hpp"

namespace mq {

class MessageQueue {
protected:

	typedef IntMpscPipe::Node Node;
	typedef AbstractCommand<Node> ACommand;

	ACommand *getcommand(Node *node) {
		return static_cast<ACommand *>(node->self);
	}

	IntMpscPipe *pipe;

public:

	static void *start(void *arg) {
		MessageQueue *mq = (MessageQueue *) arg;
		mq->run();
		return NULL;
	}

	MessageQueue()
	: pipe(new IntMpscPipe()) {

	};

	~MessageQueue() {
		delete pipe;
	}

	template<typename RESULT, typename ...ARG>
	void send(RESULT (*func)(ARG...), ARG&&... arg) {
		auto mes =
			new WaitlessCommand<Node, void, RESULT, ARG...>({func}, std::forward<ARG>(arg)...);
		mes->node.self = mes;
		pipe->send(&mes->node);
	}

	template<typename RESULT, typename ...ARG>
	mq::future<RESULT> request(RESULT (*func)(ARG...), ARG&&... arg) {
		auto mes =
			new Command<Node, void, RESULT, ARG...>({func}, std::forward<ARG>(arg)...);
		mes->node.self = mes;
		auto future = mes->prms.get_future();
		pipe->send(&mes->node);
		return future;
	}

	template<typename T, typename RESULT, typename ...ARG>
	void send(T *obj, RESULT (T::*func)(ARG...), ARG&&... arg) {
		auto mes =
			new WaitlessCommand<Node, T, RESULT, ARG...>({obj, func}, std::forward<ARG>(arg)...);
		mes->node.self = mes;
		pipe->send(&mes->node);
	}

	template<typename T, typename RESULT, typename ...ARG>
	mq::future<RESULT> request(T *obj, RESULT (T::*func)(ARG...), ARG&&... arg) {
		auto mes =
			new Command<Node, T, RESULT, ARG...>({obj, func}, std::forward<ARG>(arg)...);
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

	bool try_receive() {
		bool result = false;
		Node *node = pipe->try_recv();
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

	void nullmsg() {
		Node *nullmsg = new Node();
		nullmsg->self = nullptr;
		pipe->send(nullmsg);
	}


	void run() {
		while (receive());
	}

	void dry_run() {
		while (try_receive());
	}

};

}


#endif /* MSGQUEUE_H_ */
