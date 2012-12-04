#ifndef MEMBER_MSG_QUEUE_HPP_
#define MEMBER_MSG_QUEUE_HPP_

#include "msgqueue.hpp"

namespace mq {

template<class ThreadClass>
class MemberMessageQueue {
protected:

	MessageQueue queue;
	ThreadClass *worker;

public:

	static void *start(void *arg) {
		MemberMessageQueue *mq = (MemberMessageQueue *) arg;
		mq->run();
		return NULL;
	}

	MemberMessageQueue(ThreadClass *worker) : worker(worker) {
		//
	};

	~MemberMessageQueue() {
		//
	}

	template<typename RESULT, typename ...ARG>
	void send(RESULT (ThreadClass::*func)(ARG...), ARG&&... arg) {
		queue.send(worker, func, std::forward<ARG>(arg)...);
	}

	template<typename RESULT, typename ...ARG>
	mq::future<RESULT> request(RESULT (ThreadClass::*func)(ARG...), ARG&&... arg) {
		return queue.request(worker, func, std::forward<ARG>(arg)...);
	}

	bool receive() {
		return queue.receive();
	}

	void nullmsg() {
		queue.nullmsg();
	}

	void run() {
		queue.run();
	}

	void dry_run() {
		queue.dry_run();
	}
};

}

#endif /* MEMBER_MSG_QUEUE_HPP_ */
