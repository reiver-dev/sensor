#ifndef CALLER_HPP_
#define CALLER_HPP_

namespace mq {

/**
 * Used to pack pointer to method with object
 */
template<typename WORKER, typename RESULT, typename ...ARG>
struct Caller {
	WORKER *worker;
	RESULT (WORKER::*func)(ARG...);

	RESULT operator()(ARG&& ...arg) {
		return (worker->*func)(std::forward<ARG>(arg)...);
	}
};

}

#endif /* CALLER_HPP_ */
