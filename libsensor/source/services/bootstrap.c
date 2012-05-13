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

	uint8_t *buf = malloc(5);
	uint8_t *ptr = buf;

	size_t len = 0;
	switch (req->type) {
	case BOOTSTRAP_TYPE_CONNECT:
		AddToBuffer8(&ptr, BOOTSTRAP_TYPE_CONNECT);
		AddToBuffer32(&ptr, balancing_get_created(servicesData->balancer));
		len=5;
		break;
	case BOOTSTRAP_TYPE_DISCONNECT:
		AddToBuffer8(&ptr, BOOTSTRAP_TYPE_DISCONNECT);
		len=1;
		break;
	case BOOTSTRAP_TYPE_CONNECT_ACK:
		AddToBuffer8(&ptr, BOOTSTRAP_TYPE_CONNECT_ACK);
		AddToBuffer32(&ptr, balancing_get_created(servicesData->balancer));
		len=5;
		break;
	default:
		free(buf);
		struct RequestData data = {0, NULL};
		return data;
	}

	struct RequestData data = {len, buf};
	return data;
}

static void bootstrap_response(ServicesData servicesData, struct Node *from, struct RequestData *data) {
	uint8_t *ptr = data->buffer;
	int type  = GetFromBuffer8(&ptr);

	if (type == BOOTSTRAP_TYPE_CONNECT) {
		if (data->len != 5) {
			DERROR("BOOTSTRAP_TYPE_CONNECT len incorrect (%i)\n", data->len);
			return;
		}
		time_t t = GetFromBuffer32(&ptr);
		balancing_init_sensor_session(servicesData->balancer, from->ip4addr, t);

		BootstrapRequest bootreq = {BOOTSTRAP_TYPE_CONNECT_ACK};
		Services_Request(servicesData, BootstrapService_Get(), 0, &bootreq);

	} else if (type == BOOTSTRAP_TYPE_DISCONNECT) {
		balancing_break_sensor_session(servicesData->balancer, from->ip4addr);

	} else if (type == BOOTSTRAP_TYPE_CONNECT_ACK) {

		if (from == NULL) {
			DERROR("%s", "Got connect ack broadcasted");
			return;
		}

		if (data->len != 5) {
			DERROR("BOOTSTRAP_TYPE_CONNECT_ACK len incorrect (%i)\n", data->len);
			return;
		}

		int State = balancing_get_state(servicesData->balancer);
		if (State != STATE_WAIT_SENSORS) {
			DERROR("Got connect ack while not in STATE_WAIT_SENSORS from (%s)\n", Ip4ToStr(from->ip4addr));
			return;
		}

		time_t created = GetFromBuffer32(&ptr);
		balancing_init_sensor_session(servicesData->balancer, from->ip4addr, created);

		InfoRequest inforeq = {INFO_TYPE_POP};
		Services_Request(servicesData, InfoService_Get(), 0, &inforeq);

	}

}

