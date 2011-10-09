#!/usr/bin/python

from Sensor import Sensor

if __name__ == '__main__':
    pass

sensor = Sensor()
socket = sensor.create_socket()
print("Socket: ", socket)
print("Setting promisc: ", sensor.set_iface_promiscuous(socket, b"wlan0", True))

i = 10
while(i>0):
    print("iter: ", i)
    sensor.get_next_packet(socket, 1)
    i-=1


print("Unsetting promisc: ", sensor.set_iface_promiscuous(socket, b"wlan0", False))
print(sensor.close_socket(socket))