#include <stddef.h>
#include <sys/time.h>
#include <stdio.h>
#include "udp.h"

static MsgHeader1553_t header;

void set_sync_word(unsigned int val) {
    header.sync_word = val;
}

unsigned int crc32(const void *data, size_t length) {
    const int *bytes = (const int *)data;
    unsigned int crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}

unsigned long long get_time_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void print_header(const MsgHeader1553_t *hdr) {
    printf("HEADER STRUCTURE:\n");
    printf("  Sync Word      : 0x%08X\n", hdr->sync_word);
    printf("  Msg OpCode     : 0x%08X\n", hdr->msg_opcode);
    printf("  Msg Length     : %u bytes\n", hdr->msg_length);
    printf("  Msg Sequence # : %u\n", hdr->msg_seq_number);
    printf("  CRC-32         : 0x%08X\n", hdr->payload_crc32);
    printf("  Send Time      : %llu us\n", (unsigned long long)hdr->send_time);
    printf("  Receive Time   : %llu us\n", (unsigned long long)hdr->receive_time);
    printf("  Spare          : %llu\n", (unsigned long long)hdr->spare);
}

void handle_received_data(unsigned int subaddr, const char *text, unsigned int len) {
    static unsigned int msg_seq_counter = 0;

    header.msg_opcode = subaddr;
    header.msg_length = len + HEADER_SIZE;
    header.msg_seq_number = msg_seq_counter++;
    header.payload_crc32 = crc32(text, len);
    header.send_time = get_time_microseconds();
    header.receive_time = 0;

    print_header(&header);
    printf("Received text -> \"%s\"\n", text);
}

