#ifndef NODE_H_
#define NODE_H_

#include "../array.h"
#include "services.h"

#define NODE_TYPE_GIVE 0
#define NODE_TYPE_TAKE  1

typedef struct {
	int type;
	Array ip4array;
} NodeRequest;



Service NodeService_Get();


#endif /* NODE_H_ */
