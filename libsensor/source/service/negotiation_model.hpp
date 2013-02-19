#ifndef NEGOTIATION_MODEL_HPP_
#define NEGOTIATION_MODEL_HPP_

#include <unordered_map>

#include "service/session_context.hpp"


class NegotiationModel {
public:
	NegotiationModel();

	bool check_session(InternetAddress address);
	SessionContext *get_session(InternetAddress address);
	SessionContext *create_session(InternetAddress address);

private:
	std::unordered_map<InternetAddress, SessionContext *> sessions;
	std::unordered_map<InternetAddress, Node> nodes;

};

#endif /* NEGOTIATION_MODEL_HPP_ */
