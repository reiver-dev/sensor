#ifndef SPOOF_H_
#define SPOOF_H_

#include <stdint.h>
#include "sensor_private.h"
#include "nodes.h"

void Spoof_nodes(int packet_sock, ArrayList owned, const struct InterfaceInfo *current);


#endif /* SPOOF_H_ */
