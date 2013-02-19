#ifndef EVENTCOUNT_HPP_
#define EVENTCOUNT_HPP_

#include <semaphore.h>
#include <atomic>


class Eventcount {
public:

	Eventcount() :  waiting(false) {
		sem_init(&semaphore, 0, 0);
	}

	~Eventcount() {
		sem_destroy(&semaphore);
	}

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

	template<typename FUNC>
	auto out(FUNC func) -> decltype(func()) {
		decltype(func()) result = func();
		while (!result) {
			prepare_wait();
			result = func();
			if (result) {
				cancel_wait();
				break;
			}
			commit_wait();
			result = func();
		}
		return result;
	}


private:

	std::atomic<bool> waiting;
	sem_t semaphore;

};


#endif /* EVENTCOUNT_HPP_ */
