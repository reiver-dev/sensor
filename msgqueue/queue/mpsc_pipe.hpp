#ifndef MPSC_PIPE_HPP_
#define MPSC_PIPE_HPP_

#include <atomic>
#include <semaphore.h>

#include "eventcount.hpp"
#include "mpsc_queue.hpp"

template<typename DataT>
class MpscPipe {
public:

	void send(DataT &data) {
		queue->push(data);
		eventcount.notify();
	}

	bool recv(DataT &data) {
		return eventcount.out([&]{return queue.pop(data);});
	}

	bool try_recv(DataT &data) {
		return queue->pop(data);
	}

private:
	MpscQueue<DataT> queue;
	Eventcount eventcount;
};

#endif /* MPSC_PIPE_HPP_ */
