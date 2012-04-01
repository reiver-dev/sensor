#ifndef SERVICES_H_
#define SERVICES_H_

#include <stdbool.h>
#include <stdint.h>
#include "../nodes.h"

#define SERVICE_INFO 0
#define SERVICE_NODE 1

typedef struct Service *Service;

void Services_Init();
void Services_Destroy();
void Services_Invoke(int sock, uint32_t serviceID, struct Node *to, void *request);

#endif /* SERVICES_H_ */
