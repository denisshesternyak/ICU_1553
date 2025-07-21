#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define HEADER_SIZE 44

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

uint32_t crc32(const void *data, size_t length) {
    const int *bytes = (const int *)data;
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return ~crc;
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

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        socklen_t addr_len = sizeof(client_addr);
        int received = recvfrom(sockfd, buffer, BUFFER_SIZE-1, 0, 
                               (struct sockaddr *)&client_addr, &addr_len);
        if(received < 0) {
            perror("Receive failed");
            continue;
        }
        
        buffer[received] = '\0';
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        MsgHeader1553_t header;
        memcpy(&header, buffer, sizeof(MsgHeader1553_t));
        print_header(&header);

        printf("Received from %s:%d: %s\n", client_ip, ntohs(client_addr.sin_port), buffer+HEADER_SIZE);

        uint32_t crc = crc32(buffer+HEADER_SIZE, header.msg_length-HEADER_SIZE);
        printf("CRC32 0x%08X 0x%08X\n", crc, header.payload_crc32);

        const char *response = "Hello from server!";
        int sent = sendto(sockfd, response, strlen(response), 0, 
                        (struct sockaddr *)&client_addr, addr_len);
        if(sent < 0) {
            perror("Sendto failed");
        } else {
            printf("Sent to %s:%d: %s\n", client_ip, ntohs(client_addr.sin_port), response);
        }
    }

    close(sockfd);
    return 0;
}