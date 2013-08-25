#ifndef LOOKUP_SERVICE_HPP_
#define LOOKUP_SERVICE_HPP_

#include <stdint.h>

#include "net/pgm_socket_utils.h"


class LookupService {
public:
	LookupService(char *deviceName, char *multicastAddr, uint16_t port);

private:

};


#endif /* LOOKUP_SERVICE_HPP_ */
