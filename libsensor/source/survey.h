#ifndef SURVEY_H_
#define SURVEY_H_

uint8_t *survey_packet(int *out_length, const uint32_t toaddr, const uint32_t current_ip4,
		const uint8_t current_mac[ETH_ALEN]);
void survey_set_target_ip(uint8_t *buffer, uint32_t ip);
int survey_extract_response(const uint8_t *buffer, int length, uint32_t *out_ip, uint8_t *out_hw);


#endif /* SURVEY_H_ */
