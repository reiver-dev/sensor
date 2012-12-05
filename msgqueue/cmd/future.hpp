#ifndef FUTURE_HPP_
#define FUTURE_HPP_

#include <atomic>
#include <type_traits>
#include <memory>
#include <mutex>


#include "thread_util.hpp"

#define DISABLE_COPY(TYPE) \
	TYPE(const TYPE&) = delete; \
	TYPE& operator=(const TYPE&) = delete;

namespace mq {

struct ResultBase {

	ResultBase() {
		//
	}

	virtual ~ResultBase() {
		//
	}

	DISABLE_COPY(ResultBase)

};

template<typename R>
class Result : public ResultBase {
public:

	R& value() {
		return *(R*)&storage;
	}

	Result() : initialized() {
		//
	}

	~Result() {
		if (initialized) {
			value().~R();
		}
	}

	void set(const R& value) {
		new (address()) R(value);
		initialized = true;
	}

	void set(R&& value) {
		new (address()) R(value);
		initialized = true;
	}

private:
	typedef typename std::aligned_storage<sizeof(R), std::alignment_of<R>::value>::type storage_t;

	storage_t storage;
	bool initialized;

	void* address() {
		return static_cast<void*>(&storage);
	}
};

template<>
class Result<void> : public ResultBase {


};

class State {
private:

	detail::CondVar cond;
	detail::Mutex   mutex;
	std::unique_ptr<ResultBase> result;

public:
	State() noexcept : result() {
		//
	}

	ResultBase& wait() {
		mutex.lock();
		while (!isReady()) {
			cond.wait(mutex);
		}
		mutex.unlock();
		return *result;
	}

	bool wait_for(int sec, int nanosec) {
		bool success = true;
		mutex.lock();
		if (!isReady()) {
			cond.wait_for(mutex, sec, nanosec);
			if (!isReady()) {
				success = false;
			}
		}
		mutex.unlock();
		return success;
	}

	void set_result(ResultBase *r) {
		std::unique_ptr<ResultBase> res(r);
		{
			std::lock_guard<detail::Mutex> lock(mutex);
			result.swap(res);
		}
		cond.notify_all();
	}

	bool isReady() {
		return static_cast<bool>(result);
	}
};

class FutureBase {
public:

	FutureBase(FutureBase&& f) : state(std::move(f.state)) {
		//
	}

	FutureBase& operator=(FutureBase&& f) {
		FutureBase tmp(std::move(f));
		tmp.swap(*this);
		return *this;
	}

	~FutureBase() {
		//
	}

	DISABLE_COPY(FutureBase)

	void wait() {
		state->wait();
	}

	bool wait_for(int sec, int nanosec) {
		return state->wait_for(sec, nanosec);
	}



protected:

	FutureBase(std::shared_ptr<State> s) : state(s) {
		//
	}

	void swap(FutureBase &that) {
		state.swap(that.state);
	}

	std::shared_ptr<State> state;
};


//-------------------------------------------------

template<typename R>
class future : FutureBase {
protected:


	Result<R>& get_result() {
		ResultBase& res = state->wait();
		return static_cast<Result<R>&>(res);
	}


public :

	future(std::shared_ptr<State> s) : FutureBase(s) {
		//
	}

	future(future&& f) : FutureBase(std::move(f)) {
		//
	}

	future& operator=(future&& f) {
		future tmp(std::move(f));
		tmp.swap(*this);
		return *this;
	}

	DISABLE_COPY(future)

	R get() {
		return std::move(get_result().value());
	}
};

template<>
class future<void> : FutureBase {
protected:


	ResultBase& get_result() {
		ResultBase& res = state->wait();
		return static_cast<Result<void>&>(res);
	}


public :

	future(std::shared_ptr<State> s) : FutureBase(s) {
		//
	}

	future(future&& f) : FutureBase(std::move(f)) {
		//
	}

	future& operator=(future&& f) {
		future tmp(std::move(f));
		tmp.swap(*this);
		return *this;
	}

	DISABLE_COPY(future)

	void get() {
		get_result();
	}
};


template<typename R>
class promise {
public:

	promise() :
		state(std::make_shared<State>()), storage(new Result<R>()) {
		//
	}

	future<R> get_future() {
		return future<R>(state);
	}

	void set_value(R& result) {
		storage->set(std::move(result));
		Result<R> *tmp = storage.release();
		state->set_result(tmp);
	}

	void set_value(R&& result) {
		storage->set(std::move(result));
		Result<R> *tmp = storage.release();
		state->set_result(tmp);
	}

private	:

	std::shared_ptr<State> state;
	std::unique_ptr<Result<R>> storage;

};


template<>
class promise<void> {
public:

	promise() :
		state(std::make_shared<State>()), storage(new Result<void>()) {
		//
	}

	future<void> get_future() {
		return future<void>(state);
	}

	void set_value() {
		Result<void> *tmp = storage.release();
		state->set_result(tmp);
	}

private	:

	std::shared_ptr<State> state;
	std::unique_ptr<Result<void>> storage;

};


}

#endif /* FUTURE_HPP_ */
