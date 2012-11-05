#ifndef MSGQUEUE_H_
#define MSGQUEUE_H_

#include <cstddef>
#include <pthread.h>
#include <functional>
#include "mpsc_pipe.hpp"
#include "waitable.hpp"
#include "delegate.hpp"

namespace mq {

class MessageQueue {
public:

	static void *start(void *arg) {
		MessageQueue *mq = (MessageQueue *) arg;
		mq->run();
		return NULL;
	}

	MessageQueue()
	: running(false), pipe(new MpscPipe<Message *>) {

	};

	~MessageQueue() {
		delete pipe;
	}

	void request(void func(void)) {
		Message *mes = new Message;
		mes->func = func;
		pipe->send(mes);
	}

	void request(void func(void), Waitable &w) {
		Message *mes = new Message;
		mes->func = Delegate<void>(func, &w);
		pipe->send(mes);
		w.wait();
	}

	template<typename... ArgT>
	void request(void func(ArgT...), ArgT... req) {
		Message *mes = new Message;
		mes->func = std::bind(func, req...);
		pipe->send(mes);
	}

	template<typename... ArgT>
	void request(void func(ArgT...), Waitable &w, ArgT... args) {
		Message *mes = new Message;
		mes->func = Delegate<void>(std::bind(func, args...), &w);
		pipe->send(mes);
		w.wait();
	}

	template<typename... ArgT, typename ResultT>
	ResultT request(ResultT func(ArgT...), Waitable &w, ArgT... arg) {
		ResultT result;
		Message *mes = new Message;
		mes->func = Delegate<ResultT>(std::bind(func, arg...), &result);
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
		std::function<void(void)> func;
	};

	bool running;
	MpscPipe<Message *> *pipe;

};

}


#endif /* MSGQUEUE_H_ */
