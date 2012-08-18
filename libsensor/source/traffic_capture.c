#include <stdbool.h>
#include <pcap.h>

#include "traffic_capture.h"
#include "survey.h"
#include "debug.h"


bool TrafficCapture_prepare(struct TrafficCapture *self, sensor_t context) {
	char errbuf[PCAP_ERRBUF_SIZE] = {0};
	pcap_t *pcapHandle = NULL;

	bool success = true;
	pcapHandle = pcap_create(context->opt.capture.device_name, errbuf);
	DINFO("Pcap created %s\n", errbuf);

	success = !pcap_set_promisc(pcapHandle, context->opt.capture.promiscuous);
	success = success && !pcap_set_buffer_size(pcapHandle, context->opt.capture.buffersize);

	if (!pcap_setdirection(pcapHandle, PCAP_D_IN)) {
		DWARNING("%s\n", "can't set PCAP_D_IN");
	}

	success = success && !pcap_activate(pcapHandle);

	if (success) {
		self->handle = pcapHandle;
		DINFO("Pcap initialized successfully\n", errbuf);
	} else {
		pcap_close(pcapHandle);
		DERROR("Pcap crash: %s\n", pcap_geterr(pcapHandle));
	}

	return success;
}

void TrafficCapture_close(struct TrafficCapture *self) {
	pcap_close(self->handle);
}

void *TrafficCapture_start(struct TrafficCapture *self) {
	int res;
	const uint8_t *packet_data;
	struct pcap_pkthdr *packet_header;
	self->active = true;
	while (self->active) {
		res = pcap_next_ex(self->handle, &packet_header, &packet_data);
		if (res < 0) {
			pcap_perror(self->handle, "\n");
			self->active = false;
			return 0;
		} else if (res > 0) {
			MessageQueue_sendCopy(self->queueToCore, (void *)packet_data, packet_header->len);
		}
	}
	return 0;
}

void TrafficCapture_stop(struct TrafficCapture *self) {
	self->active = false;
}

bool TrafficCapture_isRunning(struct TrafficCapture *self) {
	return self->active;
}
