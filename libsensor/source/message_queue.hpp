#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_

#include <cstddef>
#include <pthread.h>
#include <functional>
#include "mpsc_pipe.hpp"


class Waitable {

	pthread_mutex_t mtx;
	pthread_cond_t cond;

public:

	Waitable() {
		pthread_mutex_init(&mtx, 0);
		pthread_cond_init(&cond, 0);
	}

	~Waitable() {
		pthread_mutex_destroy(&mtx);
		pthread_cond_destroy(&cond);
	}

	void wait() {
		pthread_mutex_lock(&mtx);
		pthread_cond_wait(&cond, &mtx);
	}

	void notify() {
		pthread_cond_signal(&cond);
	}

};

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


	void request(void func(void)) {
		Message *mes = new Message;
		mes->func = func;
		pipe->send(mes);
	}

	template<typename ArgT>
	void request(void func(ArgT), ArgT req) {
		Message *mes = new Message;
		mes->func = std::bind(func, req);
		pipe->send(mes);
	}

	template<typename ResultT, typename ArgT>
	ResultT request(ResultT func(ArgT), ArgT arg, Waitable &w) {
		Message *mes = new Message;
		ResultT result;
		mes->func = Delegate<ResultT>(std::bind(func, arg), &result);
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

	template<typename ResultT>
	struct Delegate {

		std::function<ResultT()> callback;
		ResultT *result;
		Waitable *wt;

		Delegate(std::function<ResultT()> cb, ResultT *ret, Waitable &w)
		: callback(cb), result(ret), wt(&w) {
			//
		}

		void operator()() {
			*result = callback();
			wt->notify();
		}
	};

	struct Message {
		std::function<void(void)> func;
	};

	bool running;
	MpscPipe<Message *> *pipe;

};


template<typename ThreadClass>
class MemberMessageQueue : public MessageQueue {
public:

	MemberMessageQueue(ThreadClass *obj) : worker(obj) {
		//
	};

	void request(void (ThreadClass::*func)()) {
		Message *mes = new Message;
		mes->func = func;
		pipe->send(mes);
	}

	template<typename ArgT>
	void request(void (ThreadClass::*func)(ArgT), ArgT arg) {
		Message *mes = new Message;
		mes->func = std::bind(func, worker, arg);
		pipe->send(mes);
	}

	template<typename ResultT, typename ArgT>
	ResultT request(ResultT (ThreadClass::*func)(ArgT), ArgT arg, Waitable &w) {
		Message *mes = new Message;
		ResultT result;
		mes->func = Delegate<ResultT>(std::bind(func, arg), &result);
		pipe->send(mes);
		w.wait();
	}

private:
	ThreadClass *worker;
};



#endif /* MESSAGE_QUEUE_H_ */
