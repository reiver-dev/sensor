#ifndef SESSION_CONTEXT_HPP_
#define SESSION_CONTEXT_HPP_

#include <vector>

#include "node.hpp"

class SessionContext {
public:

	SessionContext(InternetAddress a);
	~SessionContext();

	void setSocket(int fd);

private:

	InternetAddress address;
	std::vector<NodeAddress> owned;

};

#endif /* SESSION_CONTEXT_HPP_ */
