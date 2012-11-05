#ifndef MEMBER_MSG_QUEUE_HPP_
#define MEMBER_MSG_QUEUE_HPP_

#include "msgqueue.hpp"

namespace mq {

template<class ThreadClass>
class MemberMessageQueue {
public:

	static void *start(void *arg) {
		MessageQueue *mq = (MessageQueue *) arg;
		mq->run();
		return NULL;
	}

	MemberMessageQueue(ThreadClass *worker)
	: running(false), pipe(new MpscPipe<Message *>), worker(worker) {

	};

	~MemberMessageQueue() {
		delete pipe;
	}

	void request(void (ThreadClass::*func)(void)) {
		Message *mes = new Message;
		mes->func = func;
		pipe->send(mes);
	}

	void request(void (ThreadClass::*func)(void), Waitable &w) {
		Message *mes = new Message;
		mes->func = Delegate<void>(func, &w);
		pipe->send(mes);
		w.wait();
	}

	template<typename... ArgT>
	void request(void (ThreadClass::*func)(ArgT...), ArgT... req) {
		Message *mes = new Message;
		mes->func = std::bind(func, worker, req...);
		pipe->send(mes);
	}

	template<typename... ArgT>
	void request(void (ThreadClass::*func)(ArgT...), Waitable &w, ArgT... args) {
		Message *mes = new Message;
		mes->func = Delegate<void>(std::bind(func, worker, args...), &w);
		pipe->send(mes);
		w.wait();
	}

	template<typename... ArgT, typename ResultT>
	ResultT request(ResultT (ThreadClass::*func)(ArgT...), Waitable &w, ArgT... arg) {
		ResultT result;
		Message *mes = new Message;
		mes->func = Delegate<ResultT>(std::bind(func, worker, arg...), &result);
		pipe->send(mes);
		w.wait();
	}

	void receive() {
		Message *req = NULL;
		pipe->recv(req);
		req->func();
		delete req;
	}

	void stop() {
		running = false;
		Message *nullmsg = NULL;
		pipe->send(nullmsg);
	}


	void run() {
		if (running)
			return;
		else
			running = true;

		Message *req;
		while (running) {
			req = NULL;
			pipe->recv(req);
			if (!req) {
				running = false;
				break;
			}
			req->func();
			delete req;
		}
	}

protected:

	struct Message {
		std::function<void ()> func;
	};

	bool running;
	MpscPipe<Message *> *pipe;

	ThreadClass *worker;
};

}

#endif /* MEMBER_MSG_QUEUE_HPP_ */
