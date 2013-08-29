#ifndef ASYNCMQUEUE_HPP_
#define ASYNCMQUEUE_HPP_

#include "reactor/reactor.hpp"
#include "../msgqueue/cmd/command.hpp"
#include "../msgqueue/queue/int_mpsc_queue.hpp"


class AsyncQueue {
public:
	typedef int fd_t;

	AsyncQueue() : loop(nullptr) {
		ev_async_init(&signaler, asyncCallback);
		signaler.data = this;
	}

	void start(net::EventLoop *loop) {
		this->loop = loop;
		ev_async_start(loop->get_loop(), &signaler);
	}

	void stop() {
		ev_async_stop(loop->get_loop(), &signaler);
	}

	template<typename FUNC, typename RESULT, typename ...ARG>
	void send(FUNC func, ARG&&... arg) {
		auto mes =
			new mq::Command<Node, FUNC, ARG...>(func, std::forward<ARG>(arg)...);
		mes->get_node()->self = mes;
		queue.push(mes->get_node());
		ev_async_send(loop->get_loop(), &signaler);
	}

	template<typename FUNC, typename RESULT, typename ...ARG>
	mq::future<RESULT> request(FUNC func, ARG&&... arg) {
		auto mes =
			new mq::RequestCommand<Node, FUNC, ARG...>(func, std::forward<ARG>(arg)...);
		mes->get_node()->self = mes;
		auto future = mes->get_future();
		queue.push(mes->get_node());
		ev_async_send(loop->get_loop(), &signaler);
		return future;
	}

	bool receive() {
		bool result = false;
		Node *node = queue.pop();
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
		queue.push(nullmsg);
	}

private:

	typedef IntMpscQueue::Node Node;
	typedef mq::AbstractCommand<Node> ACommand;

	net::EventLoop *loop;
	ev_async signaler;

	IntMpscQueue queue;

	static void asyncCallback(EV_P_ ev_async *handler, int events) {
		if (!static_cast<AsyncQueue*>(handler->data)->receive()) {
			static_cast<AsyncQueue*>(handler->data)->stop();
		}
	}

	ACommand *getcommand(Node *node) {
		return static_cast<ACommand *>(node->self);
	}

};

template<class ThreadClass>
class MemberAsyncQueue {
protected:

	AsyncQueue queue;
	ThreadClass *worker;

public:

	MemberAsyncQueue(ThreadClass *worker) : worker(worker) {
		//
	};

	~MemberAsyncQueue() {
		//
	}

	void start(net::EventLoop *loop) {
		queue.start(loop);
	}

	void stop() {
		queue.stop();
	}

	template<typename RESULT, typename ...ARG>
	void send(RESULT (ThreadClass::*func)(ARG...), ARG&&... arg) {
		queue.send(worker, func, std::forward<ARG>(arg)...);
	}

	bool receive() {
		return queue.receive();
	}

	void nullmsg() {
		queue.nullmsg();
	}

	void setWorker(ThreadClass *worker) {
		this->worker = worker;
	}

};

#endif /* ASYNCMQUEUE_HPP_ */
