#include <stdbool.h>
#include <stdio.h>
#include <netinet/in.h>
#include "sensor.h"

int main(){


	int socket = create_socket();
	set_iface_promiscuous(socket, "wlan0", 1);

	int i = 10;
	while(i>0){
		printf("iter: %d\n", i);
		get_next_packet(socket, 5);
		i-=1;
	}


	set_iface_promiscuous(socket, "wlan0", 0);


}
