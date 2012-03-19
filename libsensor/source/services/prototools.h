#ifndef PROTOTOOLS_H_
#define PROTOTOOLS_H_
#include <stdint.h>
#include <stdbool.h>

#include "../nodes.h"

void send_service(int sock, int serviceID, bool request, uint8_t *data, int len, struct Node *to);

#endif /* PROTOTOOLS_H_ */
