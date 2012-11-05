#ifndef DELEGATE_HPP_
#define DELEGATE_HPP_

template<typename ResultT>
struct Delegate {

	std::function<ResultT()> callback;
	ResultT *result;
	Waitable *wt;

	Delegate(std::function<ResultT()> cb, ResultT *ret, Waitable *w)
	: callback(cb), result(ret), wt(w) {
		//
	}

	void operator()() {
		*result = std::move(callback());
		wt->notify();
	}
};

template<>
struct Delegate<void> {

	std::function<void ()> callback;
	Waitable *wt;

	Delegate(std::function<void ()> cb, Waitable *w)
	: callback(cb), wt(w) {
		//
	}

	void operator()() {
		callback();
		wt->notify();
	}

};

#endif /* DELEGATE_HPP_ */
