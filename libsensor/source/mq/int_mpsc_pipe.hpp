#ifndef INT_MPSC_PIPE_HPP_
#define INT_MPSC_PIPE_HPP_

#include <atomic>
#include <semaphore.h>
#include "int_mpsc_queue.hpp"

class IntMpscPipe {
private:

	IntMpscQueue *queue;
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

	typedef IntMpscQueue::Node Node;

	IntMpscPipe() : queue(new IntMpscQueue), waiting(false) {
		sem_init(&semaphore, 0, 0);
	}

	~IntMpscPipe() {
		sem_destroy(&semaphore);
		delete queue;
	}

	void send(Node *node) {
		queue->push(node);
		notify();
	}

	Node *recv() {
		Node *result = queue->pop();

		while (!result) {
			prepare_wait();
			result = queue->pop();
			if (result) {
				cancel_wait();
				break;
			}
			commit_wait();
			result = queue->pop();
		}

		return result;
	}

	Node *try_recv() {
		return queue->pop();
	}

};

#endif /* INT_MPSC_PIPE_HPP_ */
