#include <signal.h>
#include <pthread.h>

#include "udp.h"
#include "mil_std_1553.h"

volatile sig_atomic_t stop_flag = 0;

void handle_sigint(int sig) {
    stop_flag = 1;
}

int release_module_1553(int handle) {
    Stop_Px(handle);
    Release_Module_Px(handle);
    printf("\nRelease module\n");
    return 0;
}

int init_module_1553(int devnum, int modnum, int *handle) {
    usint device = (usint)devnum;
    usint module = (usint)modnum;
    char errstr[255];
    int status;

    status = Init_Module_Px(device, module);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Init_Module_Px Failure. %s\n", errstr);
        return status;
    }
    *handle = status;

    status = Get_Board_Status_Px(*handle);
    printf("Board status is ");
    if (status & SELF_TEST_OK)
        printf("SELF TEST OK  ");
    if (status & RAM_OK)
        printf("RAM OK  ");
    if (status & TIMERS_OK)
        printf("TIMERS OK  ");
    if (status & BOARD_READY)
        printf("BOARD READY\n");
    else
        printf("BOARD NOT READY\n");

    return 0;
}

int transmit_1553(int handle, const char *text, int rt_addr) {
    char errstr[255];
    int status, frameid;
    short int id1;
    usint statusword;
    struct FRAME framestruct[2];

    status = Set_Mode_Px(handle, BC_MODE);
    if (status < 0) {
        printf("Set_Mode failure. %s\n", Print_Error_Px(status));
        return status;
    }

    usint msgdata[32] = {0};
    int len = strlen(text);
    if (len > 64) len = 64;
    if (len % 2 != 0) {
        len++;
    }
    int word_count = len / 2;

    status = Command_Word_Px(rt_addr, RECEIVE, 1, word_count, &msgdata[0]);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Command_Word_Px failure. %s\n", errstr);
        return status;
    }
    //msgdata[0] = 0x2827; // Command word
    
    for (int i = 0; i < len; i += 2) {
        unsigned char c1 = (i < len) ? text[i] : 0;
        unsigned char c2 = (i + 1 < len) ? text[i + 1] : 0;
        msgdata[i / 2 + 1] = (c1 << 8) | c2;
    }

    status = Create_1553_Message_Px(handle, BC2RT, msgdata, &id1);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Create_1553_Message failure. %s\n", errstr);
        return status;
    }

    framestruct[0].id = id1;
    framestruct[0].gaptime = 5000;
    framestruct[1].id = 0;

    frameid = Create_Frame_Px(handle, &framestruct[0]);
    if (frameid < 0) {
        Get_Error_String_Px(frameid, errstr);
        printf("Create_Frame failure. %s\n", errstr);
        return frameid;
    }

    status = Start_Frame_Px(handle, frameid, FULLFRAME);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Start_Frame failure. %s\n", errstr);
        return status;
    }

    status = Run_BC_Px(handle, 1);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Run_BC failure. %s\n", errstr);
        return status;
    }

    usleep(1000000L);
    do {
        status = Get_Msgentry_Status_Px(handle, frameid, 0, &statusword);
    } while ((statusword & 0x8000) != 0x8000);

    printf("Status of message %d: %04X\n", id1, statusword);
    print_status_1553(statusword);

    return 0;
}

void* receive_1553_thread(void* arg) {
    struct rt_args *args = (struct rt_args *)arg;
    int handle = args->handle;
    Config config = args->config;
    int rt_addr = config.device.rt_addr;
    
    char errstr[255];
    int status;
    usint rt, subaddr, direction, wordCount, rt_id;
    usint msgdata[32] = {0};

    status = Set_Mode_Px(handle, RT_MODE);
    if (status < 0) {
        printf("Set_Mode returned error: %s\n", Print_Error_Px(status));
        goto end;
    }

    status = Set_RT_Active_Px(handle, rt_addr, 0);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Set_RT_Active(%d,0) returned error: %s\n", rt_addr, errstr);
        goto end;
    }

    int blknum = 1;
    for (size_t i = 0; i < config.cmds.count; ++i) {
        int rtid;
        int sa = config.cmds.messages[i].sub_address;
        RT_Id_Px(rt_addr, RECEIVE, sa, &rtid);
        Assign_RT_Data_Px(handle, rtid, blknum++);
    }

    status = Run_RT_Px(handle);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Run_RT returned error: %s\n", errstr);
        goto end;
    }

    struct CMDENTRYRT rtcmd;
    while (!stop_flag) {
        status = Get_Next_RT_Message_Px(handle, &rtcmd);
        if (status < 0) {
            usleep(5000);
            continue;
        }

        Parse_CommandWord_Px(rtcmd.command, &rt, &subaddr, &direction, &wordCount, &rt_id);

        blknum = -1;
        for (size_t i = 0; i < config.cmds.count; ++i) {
            if (config.cmds.messages[i].sub_address == subaddr) {
                blknum = i + 1;
                break;
            }
        }
        if (blknum == -1) {
            printf("Don't support the subaddress: 0x%02X\n", subaddr);
            continue;
        }

        status = Read_Datablk_Px(handle, blknum, msgdata);
        if (status >= 0) {
            char received_text[65] = {0};
            int text_len = 0;
            for (int j = 0; j < wordCount && text_len < 64; j++) {
                unsigned char c1 = (msgdata[j] >> 8) & 0xFF;
                unsigned char c2 = msgdata[j] & 0xFF;
                if (c1 >= 32 && c1 <= 126) received_text[text_len++] = c1;
                if (c2 >= 32 && c2 <= 126 && text_len < 64) received_text[text_len++] = c2;
            }

            if (text_len > 0) {
                handle_received_data(subaddr, received_text, text_len);
            }
        } else {
            Get_Error_String_Px(status, errstr);
            printf("Failed to read data block %d: %s\n", blknum, errstr);
        }
    }

end:
    pthread_exit(NULL);
}

void print_status_1553(usint stat) {
    int length = 0;
    if ((stat & 0x8000) == 0x8000) {
        printf("       **  END_OF_MSG  ");
        length = 23;
    } else {
        printf("Invalid status - Message not yet complete\n");
        return;
    }
    if ((stat & 0x2000) == 0x2000) {
        printf("MSG_ERROR  ");
        length += 11;
    }
    if ((stat & 0x0100) == 0x0100) {
        printf("ME_SET  ");
        length += 8;
    }
    if ((stat & 0x0800) == 0x0800) {
        printf("BAD_STATUS  ");
        length += 12;
    }
    if ((stat & 0x0400) == 0x0400) {
        printf("INVALID_MSG  ");
        length += 13;
    }
    if (length > 60) {
        printf("**\n       **  ");
        length = 11;
    }
    if ((stat & 0x0200) == 0x0200) {
        printf("LATE_RESP  ");
        length += 11;
    }
    if (length > 60) {
        printf("**\n       **  ");
        length = 11;
    }
    if ((stat & 0x0080) == 0x0080) {
        printf("INVALID_WORD  ");
        length += 14;
    }
    if (length > 60) {
        printf("**\n       **  ");
        length = 11;
    }
    printf("**\n");
}