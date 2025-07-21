#define _POSIX_C_SOURCE 200809L

#include "mil_std_1553.h"
#include "config.h"

static int handle = -1;
pthread_t trmt_1553_thread;

static void* transmit_1553_thread(void* arg);
static const char *format_time(uint64_t usec);
static uint64_t get_time_microseconds();
static void print_msg(const char *dir, const int opcode, const char *text, size_t len);
static int handle_error(int status, const char *msg);
static int get_frameid(int rt, const char *str, int subaddr, int *frameid);
static void add_text(const char *str, usint *msgdata, size_t len);
static int getWordCount(const char *msg);
static void print_status_1553(usint stat);

int release_module_1553() {
    if(handle >= 0) {
        Stop_Px(handle);
        Release_Module_Px(handle);
        printf("\nRelease MODULE_1553\n");
        handle = -1;
    }
    return 0;
}

static void* transmit_1553_thread(void* arg) {
    printf("  Running the transmit MODULE_1553 thread...\n");
    printf("\n%-14s %-4s %-8s %-6s %-s\n", "Time", "Dir", "OpCode", "Len", "Message");

    Config *config = (Config*)arg;

    int status, frameid;
    usint statusword;
    int frame_executed = 0;

    int opcode = 0;
    const char *msg;

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (!stop_flag) {
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                         (current_time.tv_nsec - start_time.tv_nsec) / 1000000;

        for(int i = 0; i < config->cmds.count; i++) {
            int rate = 1000 / config->cmds.messages[i].rate;
            long last_time = config->cmds.messages[i].frame.last_time;
            
            if(elapsed_ms - last_time >= rate) {
                config->cmds.messages[i].frame.status = 1;
                config->cmds.messages[i].frame.last_time = elapsed_ms;
            }
        }

        for(int i = 0; i < config->cmds.count; i++) {
            if(config->cmds.messages[i].frame.status) {
                opcode = config->cmds.messages[i].op_code;
                msg = config->cmds.messages[i].text;

                frameid = config->cmds.messages[i].frame.id;
                config->cmds.messages[i].frame.status = 0;
                frame_executed = 1;
                break;
            }
        }

        if(!frame_executed) {
            usleep(1000);
            continue;
        }
        frame_executed = 0;

        status = Start_Frame_Px(handle, frameid, FULLFRAME);
        if(status < 0) { handle_error(status, "Start_Frame failure"); break; }

        status = Run_BC_Px(handle, 1);
        if(status < 0) { handle_error(status, "Run_BC failure"); break; }

        print_msg("T", opcode, msg, strlen(msg));

        do {
            status = Get_Msgentry_Status_Px(handle, frameid, 0, &statusword);
        } while ((statusword & END_OF_MSG) != END_OF_MSG);
    }

    pthread_exit(NULL);
}

int init_module_1553(Config *config) {
    usint device = (usint)config->device.device_number;
    usint module = (usint)config->device.module_number;
    int status;

    status = Init_Module_Px(device, module);
    if(status < 0) { return handle_error(status, "Init_Module_Px failure"); }

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

    status = Set_Mode_Px(handle, BC_MODE);
    if(status < 0) {
        printf("Set_Mode Failure. %s\n", Print_Error_Px(status));
        Release_Module_Px(handle);
        exit(0);
    }

    for(int i = 0; i < config->cmds.count; i++) {
        int rt = config->device.rt_addr;
        char *msg = config->cmds.messages[i].text;
        int subaddr = config->cmds.messages[i].sub_address;
        int *frameid = &config->cmds.messages[i].frame.id;

        if(get_frameid(rt, msg, subaddr, frameid) != 0) {
            perror("Failed create a message\n");
            return 1;
        }
    }

    if(pthread_create(&trmt_1553_thread, NULL, transmit_1553_thread, config) != 0) {
        perror("Failed to create receive 1553 thread");
        release_module_1553(handle);
        return 1;
    }

    return 0;
}

static const char *format_time(uint64_t usec) {
    static char buf[20];
    time_t seconds = usec / 1000000;
    struct tm *tm_info = localtime(&seconds);
    int millisec = (usec % 1000000) / 1000;

    snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d",
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             millisec);
    return buf;
}

static uint64_t get_time_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void print_msg(const char *dir, const int opcode, const char *text, size_t len) {
    printf("%-14s %-4s 0x%-6.2X %-6lu %-s\n",
        format_time(get_time_microseconds()),
        dir,
        opcode,
        len,
        text);
}

static int get_frameid(int rt, const char *msg, int subaddr, int *frameid) {
    int status;
    short int id;
    struct FRAME framestruct[2];
    usint msgdata[33];
    memset(msgdata, 0, sizeof(msgdata));
    
    size_t len = strlen(msg);
    add_text(msg, msgdata, len);

    status = Command_Word_Px(rt, RECEIVE, subaddr, getWordCount(msg), &msgdata[0]);
    if(status < 0) return handle_error(status, "Command_Word_Px failure");

    status = Create_1553_Message_Px(handle, BC2RT, msgdata, &id);
    if(status < 0) return handle_error(status, "Create_1553_Message failure");

    framestruct[0].id = id;
    framestruct[0].gaptime = 5000;
    framestruct[1].id = 0;

    int frame_id = Create_Frame_Px(handle, &framestruct[0]);
    if(frame_id < 0) return handle_error(frame_id, "Create_Frame failure");

    *frameid = frame_id;
    return 0;
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
    release_module_1553();
    return 1;
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