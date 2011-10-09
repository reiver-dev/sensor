

from ctypes import *


class Sensor:

    libsensor = None

    def __init__(self):
        self.libsensor = CDLL("/home/reiver/DEVELOP/EPIC-IDS/libsensor/bin/libsensor.so")

    def create_socket(self):
        return self.libsensor.create_socket()

    def close_socket(self, socket):
        return self.libsensor.close_socket(c_int32(socket))

    def set_iface_promiscuous(self, socket, interfaceName, state):
        return self.libsensor.set_iface_promiscuous(c_int32(socket), c_char_p(interfaceName), c_bool(state))

    def get_next_packet(self, socket, seconds):
        self.libsensor.get_next_packet(c_int32(socket), c_int32(seconds))
