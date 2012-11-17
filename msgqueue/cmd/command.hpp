#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include <future>
#include "tuple_indexer.hpp"

template<typename NODE>
struct AbstractCommand {
	NODE node;

	virtual void call() = 0;
	virtual ~AbstractCommand() {
		//
	}

};

#define USE_NODE using AbstractCommand<NODE>::node

template<typename NODE, typename RESULT, typename ...ARG>
struct Command : public AbstractCommand<NODE> {
	USE_NODE;

	RESULT (*func)(ARG...);
	std::tuple<ARG...> argument;
	std::promise<RESULT> prms;

	Command(RESULT (*f)(ARG...), ARG&& ...arg)
	: func(f), argument(std::move(arg)...) {

	}

	virtual void call() {
		prms.set_value(
			forward(func, std::forward<std::tuple<ARG...> >(argument))
		);
	}

	virtual ~Command() {
		//
	}
};

template<typename NODE, typename RESULT>
struct Command<NODE, RESULT, void> : public AbstractCommand<NODE> {
	USE_NODE;

	RESULT (*func)();
	std::promise<RESULT> prms;

	Command(RESULT (*f)(void)) : func(f) {
		//
	}

	virtual void call() {
		prms.set_value(func());
	}

	virtual ~Command() {
		//
	}
};

template<typename NODE, typename ...ARG>
struct Command<NODE, void, ARG...> : public AbstractCommand<NODE> {
	USE_NODE;

	void (*func)(ARG...);
	std::tuple<ARG...> argument;
	std::promise<void> prms;

	Command(void (*f)(ARG...), ARG&& ...arg)
	: func(f), argument(std::move(arg)...) {

	}

	virtual void call() {
		forward(func, std::forward<std::tuple<ARG...> >(argument));
		prms.set_value();
	}

	virtual ~Command() {
		//
	}
};


template<typename NODE, typename RESULT, typename ...ARG>
struct WaitlessCommand : public AbstractCommand<NODE> {
	USE_NODE;

	RESULT (*func)(ARG...);
	std::tuple<ARG...> argument;

	WaitlessCommand(RESULT (*f)(ARG...), ARG&& ...arg)
	: func(f), argument(std::move(arg)...) {

	}

	virtual void call() {
		forward(func, std::forward<std::tuple<ARG...> >(argument));
	}

	virtual ~WaitlessCommand() {
		//
	}
};

// ---------------------------------------------------------

template<typename T, typename NODE, typename RESULT, typename ...ARG>
struct MemberCommand : public AbstractCommand<NODE> {
	USE_NODE;

	T *worker;
	RESULT (T::*func)(ARG...);
	std::tuple<ARG...> argument;
	std::promise<RESULT> prms;

	MemberCommand(T *wrk, RESULT (T::*f)(ARG...), ARG&& ...arg)
	: worker(wrk), func(f), argument(std::move(arg)...) {

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
struct MemberCommand<T, NODE, RESULT, void> : public AbstractCommand<NODE> {
	USE_NODE;

	T *worker;
	RESULT (T::*func)();
	std::promise<RESULT> prms;

	MemberCommand(T *wrk, RESULT (T::*f)(void)) : worker(wrk), func(f) {
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
struct MemberCommand<T, NODE, void, ARG...> : public AbstractCommand<NODE> {
	USE_NODE;

	T *worker;
	void (T::*func)(ARG...);
	std::tuple<ARG...> argument;
	std::promise<void> prms;

	MemberCommand(T *wrk, void (T::*f)(ARG...), ARG&& ...arg)
		: worker(wrk), func(f), argument(std::move(arg)...) {

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
struct WaitlessMemberCommand : public AbstractCommand<NODE> {
	USE_NODE;

	T *worker;
	void (T::*func)(ARG...);
	std::tuple<ARG...> argument;
	std::promise<void> prms;

	WaitlessMemberCommand(T *wrk, void (T::*f)(ARG...), ARG&& ...arg)
		: worker(wrk), func(f), argument(std::move(arg)...) {

	}

	virtual void call() {
		forward(worker, func, std::forward<std::tuple<ARG...> >(argument));
	}

	virtual ~WaitlessMemberCommand() {
		//
	}
};

template<typename T, typename NODE, typename RESULT>
struct WaitlessMemberCommand<T, NODE, RESULT, void> : AbstractCommand<NODE> {
	USE_NODE;

	T *worker;
	RESULT (T::*func)();

	WaitlessMemberCommand(T *wrk, RESULT (T::*f)()) : worker(wrk), func(f) {
		//
	}

	virtual void call() {
		(worker->*func)();
	}

	virtual ~WaitlessMemberCommand() {
		//
	}
};


#endif /* COMMAND_HPP_ */
