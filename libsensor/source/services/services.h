#ifndef SERVICES_H_
#define SERVICES_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint32_t serviceID;
} Service_request;

typedef void (*service_request_f)(int sock, void *request);
typedef void (*service_response_f)(int sock, uint32_t ip4_to);

typedef struct {
	uint32_t Name;
	service_request_f Request;
	service_response_f Response;
	bool broadcast_allowed;
} Service;

void service_invoke(int sock, uint32_t serviceID, void *request);

#endif /* SERVICES_H_ */
