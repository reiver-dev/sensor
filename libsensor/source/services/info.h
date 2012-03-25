#ifndef INFO_H_
#define INFO_H_

#define SERVICE_INFO 0
#include "services.h"

#define INFO_TYPE_PUSH 0
#define INFO_TYPE_POP  1

typedef struct {
	int type;
} InfoRequest;

Service get_info_service();

#endif /* INFO_H_ */
