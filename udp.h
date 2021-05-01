#ifndef UDP_HEADER
#define UDP_HEADER

#include <cstdint>

#define TOPIC_LENGTH 50
#define PAYLOAD_SIZE 1500

struct __attribute__((__packed__)) udp_message {
    char topic[TOPIC_LENGTH];
    int tip_date;
    char payload[PAYLOAD_SIZE];
};


#endif