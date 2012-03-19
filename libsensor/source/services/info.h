#ifndef INFO_H_
#define INFO_H_

#define SERVICE_INFO 0


#include "services.h"


typedef struct {
	struct Node *node;
} InfoRequest;

Service get_info_service();

#endif /* INFO_H_ */
