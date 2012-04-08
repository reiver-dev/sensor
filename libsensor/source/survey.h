#ifndef SURVEY_H_
#define SURVEY_H_

uint8_t *survey_packet(int *out_length, const uint32_t toaddr, const uint32_t current_ip4,
		const uint8_t current_mac[6]);
void survey_set_target_ip(uint8_t *buffer, uint32_t ip);
void survey_process_response(const uint8_t *buffer, int length, struct CurrentAddress *current);


#endif /* SURVEY_H_ */
