#ifndef SESSION_CONTEXT_HPP_
#define SESSION_CONTEXT_HPP_

#include <vector>

#include "address.hpp"
#include "node.hpp"

class SessionContext {
public:
	SessionContext();
	virtual ~SessionContext();


private:
	Address address;
	std::vector<NodeAddress> owned;
};

#endif /* SESSION_CONTEXT_HPP_ */
