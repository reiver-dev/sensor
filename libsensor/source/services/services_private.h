#ifndef SERVICES_PRIVATE_H
#define SERVICES_PRIVATE_H

#include <stdbool.h>

struct RequestData {
	int len;
	uint8_t *buffer;
};

typedef struct RequestData (*service_request_f) (void *request);
typedef void (*service_response_f)(int sock, struct Node *from, struct RequestData request);

struct Service {
	char *Name;
	uint32_t ID;
	service_request_f Request;
	service_response_f Response;
	bool broadcast_allowed;
};

void Service_Request(int sock, Service service, struct Node *to, void *request);

#endif /* SERVICES_PRIVATE_H */
