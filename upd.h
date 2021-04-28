#ifndef UDP_HEADER
#define UDP_HEADER

#include <cstdint>

#define TOPIC_LENGTH 50
#define PAYLOAD_SIZE 1500

struct udp_message {
    char topic[TOPIC_LENGTH];
    uint8_t tip_date;
    char payload[PAYLOAD_SIZE];
};


#endif