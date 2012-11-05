#ifndef FUTURE_HPP_
#define FUTURE_HPP_

#include <atomic>
#include "waitable.hpp"

template<typename Result>
struct Future {
private:
	Waitable *wt;
	std::atomic_flag ready = ATOMIC_FLAG_INIT;
	Result res;

public:

	Future(Waitable &wt) : wt(&wt){
		//
	}

	Future(Waitable *wt) : wt(wt){
		//
	}

	Result&& get() {
		if (!ready.test_and_set()) {
			wt->wait();
		}
		return res;
	}
};

template<>
struct Future<void> {
private:
	Waitable *wt;
	std::atomic_flag ready = ATOMIC_FLAG_INIT;

public:
	void get() {
		if (ready.test_and_set(std::memory_order_relaxed)) {
			wt->wait();
		}
	}
};

#endif /* FUTURE_HPP_ */
