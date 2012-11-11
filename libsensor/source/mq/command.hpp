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

template<typename NODE, typename RESULT, typename ...ARG>
struct Command : public AbstractCommand<NODE> {
	using AbstractCommand<NODE>::node;

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
	using AbstractCommand<NODE>::node;
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
	using AbstractCommand<NODE>::node;

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
	using AbstractCommand<NODE>::node;

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



#endif /* COMMAND_HPP_ */
