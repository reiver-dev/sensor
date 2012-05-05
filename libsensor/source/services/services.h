#ifndef SERVICES_H_
#define SERVICES_H_

#include <stdbool.h>
#include <stdint.h>
#include "../nodes.h"
#include "../balancing.h"

typedef struct Service *Service;
typedef struct Services *ServicesData;

ServicesData Services_Init(Balancer balancer, char *deviceName);
void Services_Destroy(ServicesData);
void Services_Invoke(ServicesData self, uint32_t serviceID, struct Node *to, void *request);
void Services_Request(ServicesData self, Service service, struct Node *to, void *request);
void Services_Receive(ServicesData self);
bool Services_isResponse(uint8_t *buffer, int len);


#endif /* SERVICES_H_ */
