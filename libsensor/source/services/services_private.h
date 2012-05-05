#ifndef SERVICES_PRIVATE_H
#define SERVICES_PRIVATE_H

#include "../balancing.h"
#include "services.h"

struct RequestData {
	int len;
	uint8_t *buffer;
};

struct Services {
	int udp_sock;
	ArrayList services;
	Balancer balancer;
};

typedef struct RequestData (*service_request_f) (ServicesData servicesData, void *request);
typedef void (*service_response_f)(ServicesData servicesData, struct Node *from, struct RequestData *request);

struct Service {
	char *Name;
	uint32_t ID;
	service_request_f Request;
	service_response_f Response;
	bool broadcast_allowed;
};

#endif /* SERVICES_PRIVATE_H */
