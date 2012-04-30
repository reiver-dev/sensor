#ifndef SERVICES_H_
#define SERVICES_H_

#include <stdbool.h>
#include <stdint.h>
#include "../nodes.h"

#define SERVICE_INFO 0
#define SERVICE_NODE 1

typedef struct Service *Service;

void Services_Init(char *deviceName);
void Services_Destroy();
void Services_Invoke(uint32_t serviceID, struct Node *to, void *request);
void Services_Request(Service service, struct Node *to, void *request);
void Services_Receive();

bool Services_isResponse(uint8_t *buffer, int len);


#endif /* SERVICES_H_ */
