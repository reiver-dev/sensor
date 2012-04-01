#ifndef INFO_H_
#define INFO_H_

#include "services.h"

#define INFO_TYPE_PUSH 0
#define INFO_TYPE_POP  1

typedef struct {
	int type;
} InfoRequest;

Service InfoService_Get();

#endif /* INFO_H_ */
