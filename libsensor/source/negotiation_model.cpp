#include "negotiation_model.hpp"
#include "debug.h"

NegotiationModel::NegotiationModel() {


}

bool NegotiationModel::check_session(InternetAddress address) {
	return get_session(address) != nullptr;
}

SessionContext* NegotiationModel::get_session(InternetAddress address) {
	SessionContext* context = nullptr;
	try {
		context = sessions[address];
	} catch (...) {
		//
	}
	return context;
}

SessionContext* NegotiationModel::create_session(InternetAddress address) {
	SessionContext *session = new SessionContext(address);
	sessions[address] = session;
	return session;
}
