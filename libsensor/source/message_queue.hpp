#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_

#include <cstddef>
#include <pthread.h>
#include <functional>
#include "mpsc_pipe.hpp"


class MessageQueue {
public:

	typedef void (*callback)(void *);

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

	void request(callback func, void *req) {
		Message *mes = new Message;
		mes->func = func;
		mes->request = req;
		pipe->send(mes);
	}

	void receive() {
		Message *req = NULL;
		pipe->recv(req);
		req->func(req->request);
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
			req->func(req->request);
			delete req;
		}
	}

protected:

	struct Message {
		std::function<void(void *)> func;
		void *request;
	};

	bool running;
	MpscPipe<Message *> *pipe;

};


template<typename ThreadClass>
class MemberMessageQueue : public MessageQueue {
public:

	typedef void (ThreadClass::*callback)(void *);

	MemberMessageQueue(ThreadClass *obj) : worker(obj) {
		//
	};

	template<typename ArgT>
	void request(void (ThreadClass::*func)(ArgT *), ArgT *req) {
		Message *mes = new Message;
		auto temp = std::bind((callback)func, worker, std::placeholders::_1);
		mes->func = temp;
		mes->request = req;
		pipe->send(mes);
	}

private:
	typedef MemberMessageQueue<ThreadClass> this_type;
	ThreadClass *worker;
};



#endif /* MESSAGE_QUEUE_H_ */
