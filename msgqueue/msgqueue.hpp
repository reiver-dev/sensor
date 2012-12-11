#ifndef MSGQUEUE_H_
#define MSGQUEUE_H_

#include "intqueue/int_mpsc_pipe.hpp"
#include "cmd/command.hpp"
#include "cmd/caller.hpp"

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


	template<typename FUNC, typename RESULT, typename ...ARG>
	void send(FUNC func, ARG&&... arg) {
		auto mes =
			new Command<Node, FUNC, RESULT, ARG...>(func, std::forward<ARG>(arg)...);
		mes->get_node()->self = mes;
		pipe->send(mes->get_node());
	}

	template<typename FUNC, typename RESULT, typename ...ARG>
	mq::future<RESULT> request(FUNC func, ARG&&... arg) {
		auto mes =
			new RequestCommand<Node, FUNC, RESULT, ARG...>(func, std::forward<ARG>(arg)...);
		mes->get_node()->self = mes;
		auto future = mes->get_future();
		pipe->send(mes->get_node());
		return future;
	}

	template<typename RESULT, typename ...ARG>
	void send(RESULT (*func)(ARG...), ARG&&... arg) {
		send<decltype(func), RESULT, ARG...>(func, std::forward<ARG>(arg)...);
	}

	template<typename RESULT, typename ...ARG>
	mq::future<RESULT> request(RESULT (*func)(ARG...), ARG&&... arg) {
		return request<decltype(func), RESULT, ARG...>(func, std::forward<ARG>(arg)...);
	}

	template<typename T, typename RESULT, typename ...ARG>
	void send(T *obj, RESULT (T::*func)(ARG...), ARG&&... arg) {
		Caller<T, RESULT, ARG...> caller = {obj, func};
		send<decltype(caller), RESULT, ARG...>(caller, std::forward<ARG>(arg)...);

	}

	template<typename T, typename RESULT, typename ...ARG>
	mq::future<RESULT> request(T *obj, RESULT (T::*func)(ARG...), ARG&&... arg) {
		Caller<T, RESULT, ARG...> caller = {obj, func};
		return request<decltype(caller), RESULT, ARG...>(caller, std::forward<ARG>(arg)...);
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
