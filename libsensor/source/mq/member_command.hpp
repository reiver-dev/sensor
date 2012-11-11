#ifndef MEMBER_COMMAND_HPP_
#define MEMBER_COMMAND_HPP_

#include <future>
#include "command.hpp"
#include "tuple_indexer.hpp"

template<typename CLASS, typename NODE>
struct AMemberCommand {
	NODE node;
	CLASS *worker;

	AMemberCommand(CLASS *w) : worker(w) {
		//
	}

	virtual void call() = 0;
	virtual ~AMemberCommand() {
		//
	}
};

template<typename T, typename NODE, typename RESULT, typename ...ARG>
struct MemberCommand : public AMemberCommand<T, NODE> {
	typedef AMemberCommand<T, NODE> super;
	using super::worker;

	RESULT (T::*func)(ARG...);
	std::tuple<ARG...> argument;
	std::promise<RESULT> prms;

	MemberCommand(T *wrk, RESULT (T::*f)(ARG...), ARG&& ...arg)
	: super(wrk), func(f), argument(std::move(arg)...) {

	}

	virtual void call() {
		prms.set_value(
			forward(worker, func, std::forward<std::tuple<ARG...> >(argument))
		);
	}

	virtual ~MemberCommand() {
		//
	}
};

template<typename T, typename NODE, typename RESULT>
struct MemberCommand<T, NODE, RESULT, void> : public AMemberCommand<T, NODE> {
	typedef AMemberCommand<T, NODE> super;
	using super::worker;

	RESULT (T::*func)();
	std::promise<RESULT> prms;

	MemberCommand(T *wrk, RESULT (T::*f)(void)) : super(wrk), func(f) {
		//
	}

	virtual void call() {
		prms.set_value((worker->*func)());
	}

	virtual ~MemberCommand() {
		//
	}
};

template<typename T, typename NODE, typename ...ARG>
struct MemberCommand<T, NODE, void, ARG...> : public AMemberCommand<T, NODE> {
	typedef AMemberCommand<T, NODE> super;
	using super::worker;

	void (T::*func)(ARG...);
	std::tuple<ARG...> argument;
	std::promise<void> prms;

	MemberCommand(T *wrk, void (T::*f)(ARG...), ARG&& ...arg)
		: super(wrk), func(f), argument(std::move(arg)...) {

	}

	virtual void call() {
		forward(worker, func, std::forward<std::tuple<ARG...> >(argument));
		prms.set_value();
	}

	virtual ~MemberCommand() {
		//
	}
};

template<typename T, typename NODE, typename RESULT, typename ...ARG>
struct WaitlessMemberCommand : public AMemberCommand<T, NODE> {
	typedef AMemberCommand<T, NODE> super;
	using super::worker;

	void (T::*func)(ARG...);
	std::tuple<ARG...> argument;
	std::promise<void> prms;

	WaitlessMemberCommand(T *wrk, void (T::*f)(ARG...), ARG&& ...arg)
		: super(wrk), func(f), argument(std::move(arg)...) {

	}

	virtual void call() {
		forward(worker, func, std::forward<std::tuple<ARG...> >(argument));
	}

	virtual ~WaitlessMemberCommand() {
		//
	}
};

template<typename T, typename NODE, typename RESULT>
struct WaitlessMemberCommand<T, NODE, RESULT, void> : public AMemberCommand<T, NODE> {
	typedef AMemberCommand<T, NODE> super;
	using super::worker;

	RESULT (T::*func)();

	WaitlessMemberCommand(T *wrk, RESULT (T::*f)()) : super(wrk), func(f) {
		//
	}

	virtual void call() {
		(worker->*func)();
	}

	virtual ~WaitlessMemberCommand() {
		//
	}
};

#endif /* MEMBER_COMMAND_HPP_ */
