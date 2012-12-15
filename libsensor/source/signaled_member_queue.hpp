#ifndef SIGNALED_MEMBER_QUEUE_HPP_
#define SIGNALED_MEMBER_QUEUE_HPP_

#include "signaled_queue.hpp"

template<class ThreadClass>
class SignaledMemberQueue {
protected:

	mq::SignaledQueue queue;
	ThreadClass *worker;

public:

	SignaledMemberQueue(ThreadClass *worker) : worker(worker) {
		//
	};

	~SignaledMemberQueue() {
		//
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


#endif /* SIGNALED_MEMBER_QUEUE_HPP_ */
