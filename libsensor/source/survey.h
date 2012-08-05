#ifndef SURVEY_H_
#define SURVEY_H_

void survey_perform_survey(const struct InterfaceAddress *current, int packet_sock);
bool survey_process_response(const struct InterfaceAddress *current, const uint8_t *buffer, int length);


#endif /* SURVEY_H_ */
