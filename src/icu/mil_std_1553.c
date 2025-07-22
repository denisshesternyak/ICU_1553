#include <pthread.h>

#include "client.h"
#include "mil_std_1553.h"

static int handle = -1;
pthread_t recv_1553_thread;

static void* receive_1553_thread(void* arg);
static void add_text(const char *str, usint *msgdata, size_t len);
static int getWordCount(const char *msg);
static void print_status_1553(usint stat);
static int handle_error(int status, const char *msg);

int release_module_1553() {
    if(handle >= 0) {
        Stop_Px(handle);
        Release_Module_Px(handle);
        printf("\nRelease MODULE_1553\n");
        handle = -1;
    }
    return 0;
}

int init_module_1553(Config *config) {
    usint device = (usint)config->device.device_number;
    usint module = (usint)config->device.module_number;
    int status;

    status = Init_Module_Px(device, module);
    if(status < 0) return handle_error(status, "Init_Module_Px failure");

    handle = status;
    printf("Init MODULE_1553 success!\n");

    status = Get_Board_Status_Px(handle);
    printf("  Board status is ");
    if(status & SELF_TEST_OK)
        printf("SELF TEST OK  ");
    if(status & RAM_OK)
        printf("RAM OK  ");
    if(status & TIMERS_OK)
        printf("TIMERS OK  ");
    if(status & BOARD_READY)
        printf("BOARD READY\n");
    else
        printf("BOARD NOT READY\n");

    if(pthread_create(&recv_1553_thread, NULL, receive_1553_thread, config) != 0) {
        perror("Failed to create receive 1553 thread");
        release_module_1553(handle);
        return -1;
    }

    return 0;
}

int transmit_1553(const char *text, int rt_addr) {
    int status, frameid;
    short int id1;
    usint statusword;
    struct FRAME framestruct[2];

    status = Set_Mode_Px(handle, BC_MODE);
    if(status < 0) {
        printf("Set_Mode failure. %s\n", Print_Error_Px(status));
        return status;
    }

    usint msgdata[33] = {0};

    size_t len = strlen(text);
    add_text(text, msgdata, len);

    status = Command_Word_Px(rt_addr, RECEIVE, 1, getWordCount(text), &msgdata[0]);
    if(status < 0) return handle_error(status, "Command_Word_Px failure");

    status = Create_1553_Message_Px(handle, BC2RT, msgdata, &id1);
    if(status < 0) return handle_error(status, "Create_1553_Message failure");

    framestruct[0].id = id1;
    framestruct[0].gaptime = 5000;
    framestruct[1].id = 0;

    frameid = Create_Frame_Px(handle, &framestruct[0]);
    if(status < 0) return handle_error(status, "Create_Frame failure");

    status = Start_Frame_Px(handle, frameid, FULLFRAME);
    if(status < 0) return handle_error(status, "Start_Frame failure");

    status = Run_BC_Px(handle, 1);
    if(status < 0) return handle_error(status, "Run_BC failure");

    usleep(1000000L);
    do {
        status = Get_Msgentry_Status_Px(handle, frameid, 0, &statusword);
    } while ((statusword & 0x8000) != 0x8000);

    printf("Status of message %d: %04X\n", id1, statusword);
    print_status_1553(statusword);

    return 0;
}

static void* receive_1553_thread(void* arg) {
    printf("  Running the receive MODULE_1553 thread...\n");
    printf("\n%-14s %-10s %-10s %-8s %-6s %-s\n", "Time", "From", "To", "OpCode", "Len", "Message");

    Config *config = (Config*)arg;
    int rt_addr = config->device.rt_addr;
    
    char errstr[255];
    int status;
    usint rt, subaddr, direction, wordCount, rt_id;
    usint msgdata[33] = {0};

    status = Set_Mode_Px(handle, RT_MODE);
    if(status < 0) { handle_error(status, "Set_Mode failure"); goto end; }

    status = Set_RT_Active_Px(handle, rt_addr, 0);
    if(status < 0) { handle_error(status, "Set_RT_Active failure"); goto end; }

    int blknum = 0;
    for (size_t i = 0; i < 32; ++i) {
        int rtid;
        int sa = blknum;
        RT_Id_Px(rt_addr, RECEIVE, sa, &rtid);
        Assign_RT_Data_Px(handle, rtid, blknum++);
    }

    status = Run_RT_Px(handle);
    if(status < 0) { handle_error(status, "Run_RT failure"); goto end; }

    struct CMDENTRYRT rtcmd;
    while (!stop_flag) {
        status = Get_Next_RT_Message_Px(handle, &rtcmd);
        if(status < 0) {
            usleep(5000);
            continue;
        }

        Parse_CommandWord_Px(rtcmd.command, &rt, &subaddr, &direction, &wordCount, &rt_id);
        if(wordCount == 0) wordCount = 32; // 0 indicates a word count of 32.

        blknum = subaddr;

        status = Read_Datablk_Px(handle, blknum, msgdata);
        if(status >= 0) {
            char received_text[65] = {0};
            uint32_t len = 0;
            for (int j = 0; j < wordCount && len < 64; j++) {
                unsigned char c1 = (msgdata[j] >> 8) & 0xFF;
                unsigned char c2 = msgdata[j] & 0xFF;
                if(c1 >= 32 && c1 <= 126) received_text[len++] = c1;
                if(c2 >= 32 && c2 <= 126 && len < 64) received_text[len++] = c2;
            }

            if(len > 0) {
                handle_received_data(subaddr, received_text, len);
            }
        } else {
            Get_Error_String_Px(status, errstr);
            printf("Failed to read data block %d: %s\n", blknum, errstr);
        }
    }

end:
    pthread_exit(NULL);
}

static void add_text(const char *str, usint *msgdata, size_t len) {
    for (int i = 0; i < len; i += 2) {
        unsigned char c1 = (i < len) ? str[i] : 0;
        unsigned char c2 = (i + 1 < len) ? str[i + 1] : 0;
        msgdata[i / 2 + 1] = (c1 << 8) | c2;
    }
}

static int getWordCount(const char *msg) {
    size_t len = strlen(msg);
    if(len >= 64) return 0; // 0 indicates a word count of 32.
    return (len + 1) / 2;
}

static int handle_error(int status, const char *msg) {
    char errstr[255];
    Get_Error_String_Px(status, errstr);
    printf("%s: %s\n", msg, errstr);
    Release_Module_Px(handle);
    return status;
}

static void print_status_1553(usint stat) {
    int length = 0;
    if((stat & 0x8000) == 0x8000) {
        printf("       **  END_OF_MSG  ");
        length = 23;
    } else {
        printf("Invalid status - Message not yet complete\n");
        return;
    }
    if((stat & 0x2000) == 0x2000) {
        printf("MSG_ERROR  ");
        length += 11;
    }
    if((stat & 0x0100) == 0x0100) {
        printf("ME_SET  ");
        length += 8;
    }
    if((stat & 0x0800) == 0x0800) {
        printf("BAD_STATUS  ");
        length += 12;
    }
    if((stat & 0x0400) == 0x0400) {
        printf("INVALID_MSG  ");
        length += 13;
    }
    if(length > 60) {
        printf("**\n       **  ");
        length = 11;
    }
    if((stat & 0x0200) == 0x0200) {
        printf("LATE_RESP  ");
        length += 11;
    }
    if(length > 60) {
        printf("**\n       **  ");
        length = 11;
    }
    if((stat & 0x0080) == 0x0080) {
        printf("INVALID_WORD  ");
        length += 14;
    }
    if(length > 60) {
        printf("**\n       **  ");
        length = 11;
    }
    printf("**\n");
}