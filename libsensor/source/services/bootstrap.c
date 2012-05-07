#include <stdlib.h>
#include "services.h"
#include "services_private.h"
#include "bootstrap.h"
#include "info.h"

#include "../debug.h"
#include "../util.h"


#define BOOTSTRAPID 3



static struct RequestData bootstrap_request(ServicesData servicesData, void *request);
static void bootstrap_response(ServicesData servicesData, struct Node *from, struct RequestData *data);

static struct Service bootstrapService = {
	.Request  = bootstrap_request,
	.Response = bootstrap_response,
	.ID       = BOOTSTRAPID,
	.Name     = "Bootstrap Service",
	.broadcast_allowed = true
};

Service BootstrapService_Get() {
	return &bootstrapService;
}


static struct RequestData bootstrap_request(ServicesData servicesData, void *request) {
	BootstrapRequest *req = request;

	uint8_t *buf = malloc(1);

	switch (req->type) {
	case BOOTSTRAP_TYPE_CONNECT:
		*buf = BOOTSTRAP_TYPE_CONNECT;
		break;
	case BOOTSTRAP_TYPE_DISCONNECT:
		*buf = BOOTSTRAP_TYPE_DISCONNECT;
		break;
	case BOOTSTRAP_TYPE_CONNECT_ACK:
		*buf = BOOTSTRAP_TYPE_DISCONNECT;
		break;
	default:
		free(buf);
		struct RequestData data = {0, NULL};
		return data;
	}

	struct RequestData data = {1, buf};
	return data;
}

static void bootstrap_response(ServicesData servicesData, struct Node *from, struct RequestData *data) {
	uint8_t *ptr = data->buffer;
	int type  = GetFromBuffer8(&ptr);

	if (type == BOOTSTRAP_TYPE_CONNECT) {
		node_set_sensor(from);

		BootstrapRequest bootreq = {BOOTSTRAP_TYPE_CONNECT_ACK};
		Services_Request(servicesData, BootstrapService_Get(), 0, &bootreq);

	} else if (BOOTSTRAP_TYPE_DISCONNECT) {
		node_set_client(from);

	} else if (BOOTSTRAP_TYPE_CONNECT_ACK) {

		if (from == NULL) {
			DERROR("%s", "Got connect ack broadcasted");
			return;
		}

		int State = balancing_get_state(servicesData->balancer);
		if (State != STATE_WAIT_SENSORS) {
			DERROR("Got connect ack while not in STATE_WAIT_SENSORS from (%s)", Ip4ToStr(from->ip4addr));
			return;
		}

		node_set_sensor(from);

		InfoRequest inforeq = {INFO_TYPE_PUSH};
		Services_Request(servicesData, InfoService_Get(), 0, &inforeq);

	}

}

