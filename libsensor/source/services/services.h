#ifndef SERVICES_H_
#define SERVICES_H_

#include <stdbool.h>
#include <stdint.h>
#include "../nodes.h"

typedef struct {
	uint32_t serviceID;
} Service_request;

typedef int (*service_request_f) (void *request, uint8_t *buffer);
typedef void (*service_response_f)(int sock, struct Node *from, void *request);

typedef struct {
	uint32_t Name;
	service_request_f Request;
	service_response_f Response;
	bool broadcast_allowed;
} Service;

void service_invoke(int sock, uint32_t serviceID, struct Node *to, void *request);

void service_request(int sock, Service *service, struct Node *to, void *request);

void send_service(int sock, int serviceID, uint8_t *data, int len, struct Node *to);

#endif /* SERVICES_H_ */
