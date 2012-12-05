#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include "future.hpp"
#include "tuple_indexer.hpp"

template<typename NODE>
struct AbstractCommand {
	NODE node;

	virtual void call() = 0;
	virtual ~AbstractCommand() {
		//
	}
};

template<typename WORKER, typename RESULT, typename ...ARG>
struct Caller {
	WORKER *worker;
	RESULT (WORKER::*func)(ARG...);

	RESULT operator()(std::tuple<ARG...>&& arg) {
		return forward(worker, func, std::forward<decltype(arg)>(arg));
	}
};

template<typename RESULT, typename ...ARG>
struct Caller<void, RESULT, ARG...> {
	RESULT (*func)(ARG...);

	RESULT operator()(std::tuple<ARG...>&& arg) {
		return forward(func, std::forward<decltype(arg)>(arg));
	}
};

template<typename WORKER, typename RESULT>
struct Caller<WORKER, RESULT, void> {
	WORKER *worker;
	RESULT (WORKER::*func)();

	RESULT operator()() {
		return (worker->*func)();
	}
};

template<typename RESULT>
struct Caller<void, RESULT, void> {
	RESULT (*func)();

	RESULT operator()() {
		return func();
	}
};

template<typename RESULT>
struct PromiseApplier {

	template<typename FUNC, typename ...ARG>
	static void apply(mq::promise<RESULT> &p, FUNC f, std::tuple<ARG...>&& args) {
		p.set_value(f(std::forward<decltype(args)>(args)));
	}

	template<typename FUNC>
	static void apply(mq::promise<RESULT> &p, FUNC f) {
		p.set_value(f());
	}
};

template<>
struct PromiseApplier<void> {

	template<typename FUNC, typename ...ARG>
	static void apply(mq::promise<void> &p, FUNC f, std::tuple<ARG...>&& args) {
		f(std::forward<decltype(args)>(args));
		p.set_value();
	}

	template<typename FUNC>
	static void apply(mq::promise<void> &p, FUNC f) {
		f();
		p.set_value();
	}
};




#define USE_NODE using AbstractCommand<NODE>::node

template<typename NODE, typename T, typename RESULT, typename ...ARG>
struct Command : public AbstractCommand<NODE> {
	USE_NODE;

	typedef Caller<T, RESULT, ARG...> caller_type;

	caller_type caller;
	std::tuple<ARG...> argument;
	mq::promise<RESULT> prms;

	Command(Caller<T, RESULT, ARG...> c, ARG&& ...arg)
		: caller(c), argument(std::move(arg)...) {
		//
	}

	virtual void call() {
		PromiseApplier<RESULT>::apply(
			prms,
			caller,
			std::forward<decltype(argument)>(argument)
		);
	}

	virtual ~Command() {
		//
	}
};

template<typename NODE, typename T, typename RESULT, typename ...ARG>
struct WaitlessCommand : public AbstractCommand<NODE> {
	USE_NODE;

	typedef Caller<T, RESULT, ARG...> caller_type;

	caller_type caller;
	std::tuple<ARG...> argument;

	WaitlessCommand(Caller<T, RESULT, ARG...> c, ARG&& ...arg)
		: caller(c), argument(std::move(arg)...) {
		//
	}

	virtual void call() {
		caller(std::forward<decltype(argument)>(argument));
	}

	virtual ~WaitlessCommand() {
		//
	}
};




#endif /* COMMAND_HPP_ */
