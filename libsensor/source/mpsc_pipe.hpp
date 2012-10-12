#ifndef MPSC_PIPE_HPP_
#define MPSC_PIPE_HPP_

#include <atomic>
#include <semaphore.h>
#include "mpsc_queue.hpp"

template<typename DataT>
class MpscPipe {
private:
	MpscQueue<DataT> *queue;
	std::atomic<bool> waiting;
	sem_t semaphore;

	void prepare_wait() {
		waiting.store(true, std::memory_order_seq_cst);
	}

	void cancel_wait() {
		waiting.store(false, std::memory_order_release);
	}

	void commit_wait() {
		sem_wait(&semaphore);
	}

	void notify() {
		if (waiting.load(std::memory_order_acquire)) {
			waiting.store(false, std::memory_order_release);
			sem_post(&semaphore);
		}
	}

public:

	MpscPipe() : queue(new MpscQueue<DataT>), waiting(false) {
		sem_init(&semaphore, 0, 0);
	}

	~MpscPipe() {
		sem_destroy(&semaphore);
		delete queue;
	}

	void send(DataT &data) {
		queue->push(data);
		notify();
	}

	bool recv(DataT &data) {
		bool result;
		result = queue->pop(data);

		while (!result) {
			prepare_wait();
			result = queue->pop(data);
			if (result) {
				cancel_wait();
				break;
			}
			commit_wait();
			result = queue->pop(data);
		}

		return result;
	}

	bool try_recv(DataT &data) {
		return queue->pop(data);
	}

};

#endif /* MPSC_PIPE_HPP_ */
