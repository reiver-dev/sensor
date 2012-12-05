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

	RESULT operator()(ARG&& ...arg) {
		return (worker->*func)(std::forward<ARG>(arg)...);
	}
};

template<typename RESULT>
struct PromiseApplier {

	template<typename FUNC, typename ...ARG>
	static void apply(mq::promise<RESULT> &p, FUNC f, std::tuple<ARG...>&& args) {
		p.set_value(
			functor_expand<RESULT>::forward(f, std::forward<decltype(args)>(args))
		);
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
		functor_expand<void>::forward(f, std::forward<decltype(args)>(args));
		p.set_value();
	}

	template<typename FUNC>
	static void apply(mq::promise<void> &p, FUNC f) {
		f();
		p.set_value();
	}
};


#define USE_NODE using AbstractCommand<NODE>::node

template<typename NODE, typename FUNCTOR, typename RESULT, typename ...ARG>
struct Command : public AbstractCommand<NODE> {
	USE_NODE;

	FUNCTOR caller;
	std::tuple<ARG...> argument;

	Command(FUNCTOR c, ARG&& ...arg)
		: caller(c), argument(std::move(arg)...) {
		//
	}

	virtual void call() {
		functor_expand<RESULT>
		::forward(caller, std::forward<std::tuple<ARG...>>(argument));
	}

	virtual ~Command() {
		//
	}
};


template<typename NODE, typename FUNCTOR, typename RESULT, typename ...ARG>
struct RequestCommand : public AbstractCommand<NODE> {
	USE_NODE;

	FUNCTOR caller;
	std::tuple<ARG...> argument;
	mq::promise<RESULT> prms;

	RequestCommand(FUNCTOR c, ARG&& ...arg)
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

	virtual ~RequestCommand() {
		//
	}
};








#endif /* COMMAND_HPP_ */
