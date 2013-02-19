#ifndef TUPLE_INDEXER_HPP_
#define TUPLE_INDEXER_HPP_


/**
 * Indices holder
 */
template <size_t...>
struct indices {
	//
};

/**
 * Add N-th index and take one argument in recursion
 */
template<size_t N, typename Indices, typename... Types>
struct make_indices_impl;

template<size_t N, size_t... Indices, typename Type, typename ... Types>
struct make_indices_impl<N, indices<Indices...>, Type, Types...> {
	typedef typename make_indices_impl<N + 1, indices<Indices..., N>, Types...>::type type;
};

/**
 * Add last index, no types left
 */
template<size_t N, size_t ... Indices>
struct make_indices_impl<N, indices<Indices...>> {
	typedef indices<Indices...> type;
};

/**
 * Add first index, container is empty
 */
template<typename... Types>
struct make_indices : make_indices_impl<0, indices<>, Types...> {
	//
};

/**
 * Unpack arguments for function pointer call
 */
template<typename RESULT, typename ...ARG, size_t ... Indices>
RESULT forward_impl(RESULT (*func)(ARG...), std::tuple<ARG...>&& tuple, indices<Indices...>) {
	return func(std::forward<ARG>(std::get<Indices>(tuple))...);
}

template<typename RESULT, typename ...ARG>
RESULT forward(RESULT (*func)(ARG...), std::tuple<ARG...>&& tuple) {
	typedef typename make_indices<ARG...>::type Indices;
	return forward_impl(func, std::forward<decltype(tuple)>(tuple), Indices());
}

/**
 * Unpack arguments for method call with corresponding object
 */
template<typename T, typename RESULT, typename ...ARG, size_t ... Indices>
RESULT forward_member_impl(T *worker, RESULT (T::*func)(ARG...), std::tuple<ARG...>&& tuple, indices<Indices...>) {
	return (worker->*func)(std::forward<ARG>(std::get<Indices>(tuple))... );
}

template<typename T, typename RESULT, typename ...ARG>
RESULT forward(T *worker, RESULT (T::*func)(ARG...), std::tuple<ARG...>&& tuple) {
	typedef typename make_indices<ARG...>::type Indices;
	return forward_member_impl(worker, func, std::forward<decltype(tuple)>(tuple), Indices());
}

/**
 * Unpack arguments for function object call
 */
template<typename RESULT>
class functor_expand {
private:

	template<typename FUNCTOR, typename ...ARG, size_t ...Indices>
	static RESULT forward_functor_impl(FUNCTOR func, std::tuple<ARG...>&& tuple, indices<Indices...>) {
		return func(std::forward<ARG>(std::get<Indices>(tuple))...);
	}

public:

	template<typename FUNCTOR, typename ...ARG>
	static RESULT forward(FUNCTOR func, std::tuple<ARG...>&& tuple) {
		typedef typename make_indices<ARG...>::type Indices;
		return forward_functor_impl(func, std::forward<decltype(tuple)>(tuple), Indices());
	}

};

#endif /* TUPLE_INDEXER_HPP_ */
