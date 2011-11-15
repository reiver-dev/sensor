#ifndef REDIRECT_H_
#define REDIRECT_H_

void get_current_mac(int sock, const char* interfaceName);
uint32_t get_current_address(int sock, const char* interfaceName);
void get_current_address6(int sock, const char* interfaceName);

#endif /* REDIRECT_H_ */
