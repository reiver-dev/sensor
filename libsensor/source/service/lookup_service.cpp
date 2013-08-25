#include "lookup_service.hpp"

LookupService::LookupService(char *deviceName, char *multicastAddr, uint16_t port) {
	pgm_sock_t *sender, *receiver;

	sender = create_pgm_socket(deviceName, multicastAddr, port, false);
	set_pgm_sender(sender);

	receiver = create_pgm_socket(deviceName, multicastAddr, port, false);
	set_pgm_receiver(receiver);


}
