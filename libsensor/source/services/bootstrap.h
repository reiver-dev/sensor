#ifndef BOOTSTRAP_H_
#define BOOTSTRAP_H_

#include "services.h"

#define BOOTSTRAP_TYPE_CONNECT 1
#define BOOTSTRAP_TYPE_CONNECT_ACK 2
#define BOOTSTRAP_TYPE_DISCONNECT  0

typedef struct {
	int type;
} BootstrapRequest;

Service BootstrapService_Get();


#endif /* BOOTSTRAP_H_ */
