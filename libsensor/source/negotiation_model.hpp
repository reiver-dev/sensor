#ifndef NEGOTIATION_MODEL_HPP_
#define NEGOTIATION_MODEL_HPP_

#include <unordered_map>

#include "session_context.hpp"


class NegotiationModel {
public:
	NegotiationModel();

	bool create_session();

private:
	std::unordered_map<InternetAddress, SessionContext> sessions;
	std::unordered_map<InternetAddress, Node> nodes;

};

#endif /* NEGOTIATION_MODEL_HPP_ */
