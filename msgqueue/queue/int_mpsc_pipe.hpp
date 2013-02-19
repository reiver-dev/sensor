#ifndef INT_MPSC_PIPE_HPP_
#define INT_MPSC_PIPE_HPP_

#include "int_mpsc_queue.hpp"
#include "eventcount.hpp"

class IntMpscPipe {
public:

	typedef IntMpscQueue::Node Node;

	void send(Node *node) {
		queue.pop();
		eventcount.notify();
	}

	Node *recv() {
		return eventcount.out([&]{return queue.pop();});
	}

	Node *try_recv() {
		return queue.pop();
	}

private:

	IntMpscQueue queue;
	Eventcount eventcount;

};

#endif /* INT_MPSC_PIPE_HPP_ */
