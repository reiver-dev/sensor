#include <stdbool.h>
#include <pcap.h>

#include "traffic_capture.h"
#include "survey.h"
#include "debug.h"


void *capture_thread(void *arg) {
	struct TrafficCapture *self = arg;
	sensor_t context = self->context;
	pcap_t *pcapHandle = NULL;
	char errbuf[PCAP_ERRBUF_SIZE] = {0};

	bool success = true;
	pcapHandle = pcap_create(context->opt.capture.device_name, errbuf);
	DINFO("Pcap created %s\n", errbuf);

	success = !pcap_set_promisc(pcapHandle, context->opt.capture.promiscuous);
	success = success && !pcap_set_buffer_size(pcapHandle, context->opt.capture.buffersize);
	if (!pcap_setdirection(pcapHandle, PCAP_D_IN)) {
		DERROR("%s\n", "can't set PCAP_D_IN");
	}
	success = success && !pcap_activate(pcapHandle);

	if (!success) {
		DERROR("Pcap crash: %s\n", pcap_geterr(pcapHandle));
		return 0;
	}

	int res;
	const uint8_t *packet_data;
	struct pcap_pkthdr *packet_header;
	while (context->activated) {
		res = pcap_next_ex(pcapHandle, &packet_header, &packet_data);

		if (res < 0) {
			pcap_perror(pcapHandle, "\n");
			sensor_breakloop(context);
		} else if (res > 0) {
			//survey_process_response(&context->current, packet_data, packet_header->len);
			DINFO("got %i\n", packet_header->len);
			MessageQueue_sendCopy(self->queueToCore, (void *)packet_data, packet_header->len);
		}
	}

	pcap_close(pcapHandle);
	return 0;
}
