#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include <stdint.h>

#include "config.h"

#pragma pack(push, 1)
typedef struct {
    uint32_t sync_word;
    uint32_t msg_opcode;
    uint32_t msg_length;
    uint32_t msg_seq_number;
    uint32_t payload_crc32;
    uint64_t send_time;
    uint64_t receive_time;
    uint64_t spare;
} MsgHeader1553_t;
#pragma pack(pop)

typedef enum {
    MIL_STD_1553,
    UDP,
} Prtcl_t;

extern pthread_t recv_client_thread;

void close_socket();
int init_socket(Config *config);
int send_data(const char *message, size_t len);
void *receive_data(void *arg);
uint32_t crc32(const void *data, size_t length);
void handle_received_data(uint32_t subaddr, const char *text, uint32_t len);

#endif //CLIENT_H