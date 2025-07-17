#include <stddef.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <errno.h>

#include "client.h"

#define HEADER_SIZE 44
#define BUFFER_SIZE 1024

static int sockfd = -1;
static struct sockaddr_in server_addr;
static MsgHeader1553_t header;

pthread_t recv_client_thread;

uint32_t crc32(const void *data, size_t length) {
    const int *bytes = (const int *)data;
    uint32_t crc = 0xFFFFFFFF;

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

uint64_t get_time_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

void close_socket() {
    if (sockfd > 0) {
        shutdown(sockfd, SHUT_RD);
        close(sockfd);
        sockfd = -1;
    }
    printf("Close socket\n");
}

int init_socket(Config *config) {
    header.sync_word = config->device.sync_word;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(config->network.source.port);
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->network.destination.port);
    if (inet_pton(AF_INET, config->network.destination.ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(sockfd);
        return -1;
    }

    if (pthread_create(&recv_client_thread, NULL, receive_data, config) != 0) {
        perror("Thread creation failed");
        close(sockfd);
        return 1;
    }

    return 0;
}

int send_data(const char *message, size_t len) {
    ssize_t sent = sendto(sockfd, message, len, 0, 
                         (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent < 0) {
        perror("Failed to send the message");
        return -1;
    }

    printf("Sent to server: %s\n", message+HEADER_SIZE);
    return 0;
}

void *receive_data(void *arg) {
    //Config *config = (Config*)arg;

    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);

    while (!stop_flag) {
        int received = recvfrom(sockfd, buffer, BUFFER_SIZE-1, 0, 
                               (struct sockaddr *)&from_addr, &addr_len);
        if (received <= 0) {
            if (errno == EBADF) {
                fprintf(stderr, "Socket was closed (EBADF), exiting thread.\n");
            } else if (errno == EINTR) {
                fprintf(stderr, "Interrupted by signal (EINTR), exiting thread.\n");
            } else {
                perror("Receive failed");
            }
            break;
        }

        buffer[received] = '\0';
        char from_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from_addr.sin_addr, from_ip, INET_ADDRSTRLEN);
        printf("Received from %s:%d: %s\n", from_ip, ntohs(from_addr.sin_port), buffer);
    }

    return NULL;
}

static void print_header(const MsgHeader1553_t *hdr) {
    printf("HEADER STRUCTURE:\n");
    printf("  Sync Word      : 0x%08X\n", hdr->sync_word);
    printf("  Msg OpCode     : 0x%08X\n", hdr->msg_opcode);
    printf("  Msg Length     : %u bytes\n", hdr->msg_length);
    printf("  Msg Sequence # : %u\n", hdr->msg_seq_number);
    printf("  CRC-32         : 0x%08X\n", hdr->payload_crc32);
    printf("  Send Time      : %lu us\n", (uint64_t)hdr->send_time);
    printf("  Receive Time   : %lu us\n", (uint64_t)hdr->receive_time);
    printf("  Spare          : %lu\n", (uint64_t)hdr->spare);
}

void handle_received_data(uint32_t subaddr, const char *text, uint32_t len) {
    static uint32_t msg_seq_counter = 1;

    header.msg_opcode = subaddr;
    header.msg_length = len + HEADER_SIZE;
    header.msg_seq_number = msg_seq_counter++;
    header.payload_crc32 = crc32(text, len);
    header.send_time = get_time_microseconds();
    header.receive_time = 0;

    print_header(&header);
    printf("Received text -> \"%s\"\n", text);

    char message[128];
    memcpy(message, &header, HEADER_SIZE);
    memcpy(message+HEADER_SIZE, text, len);

    send_data(message, header.msg_length);
}

