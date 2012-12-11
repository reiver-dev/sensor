#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include "future.hpp"
#include "tuple_indexer.hpp"

namespace mq {

template<typename NODE>
class AbstractCommand {
private:
	NODE node;

public:

	NODE *get_node() {
		return &node;
	}

	virtual void call() = 0;
	virtual ~AbstractCommand() {
		//
	}
};

/**
 * Unpacks tuple of arguments to variadic template list
 * and puts result into promise
 */
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

/**
 * Specialization for void return value
 * when promise is simply called
 */
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

/**
 * Applies stored arguments to function object
 */
template<typename NODE, typename FUNCTOR, typename RESULT, typename ...ARG>
class Command : public AbstractCommand<NODE> {
private:
	FUNCTOR caller;
	std::tuple<ARG...> argument;

public:

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

/**
 * Applies stored arguments to function object
 * and returns result via future-promise
 */
template<typename NODE, typename FUNCTOR, typename RESULT, typename ...ARG>
class RequestCommand : public AbstractCommand<NODE> {
private:
	FUNCTOR caller;
	std::tuple<ARG...> argument;
	mq::promise<RESULT> prms;

public:

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

	mq::future<RESULT> get_future() {
		return prms.get_future();
	}
};

}

#endif /* COMMAND_HPP_ */
