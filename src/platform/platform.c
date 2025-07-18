#include <stdio.h>
#ifdef _WIN32
#include <conio.h>
#else
#include <string.h>
#endif
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "galahad_px.h"

int isCmd;

#include "cmdFunctions.h"

struct FRAME framestruct[2];

int printstat(usint stat) {
    int length;
    if ((stat & 0x8000) == 0x8000) {
        printf("       **  END_OF_MSG  ");
        length = 23;
    } else {
        printf("Invalid status - Message not yet complete\n");
        return 0;
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
    return 0;
}

int main(int argc, char **argv) {
    int status, devnum, modnum, handle, frameid;
    short int id1;
    usint device, module, statusword, cardTypeBC;
    char errstr[255];

    int numArguments = 3;
    char* usageInformation = "platform device_number module_number";

    if (argc <= 1) {
        isCmd = 0;
    } else {
        isCmd = 1;
        if ((strcmp(argv[1], "/?") == 0) || (strcmp(argv[1], "?") == 0)) {
            printf(usageInformation);
            return 0;
        }
        if (argc < numArguments) {
            printf("Not enough arguments\n");
            return 0;
        }
    }

    if (isCmd) {
        devnum = atoi(argv[1]);
        modnum = atoi(argv[2]);
    } else {
        devnum = modnum = 0;
    }

    if (!isCmd) {
        printf("Enter the device number of the board: ");
        ReadInt(&devnum);
        if (devnum != 999) {
            printf("Enter the module number: ");
            ReadInt(&modnum);
        }
    }

    module = (usint)modnum;
    device = (usint)devnum;

    if (devnum == 999) {
        device = SIMULATE;
        module = 0;
    }

    status = Init_Module_Px(device, module);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Init_Module_Px Failure. %s\n", errstr);
        ReadKey();
        exit(0);
    }
    handle = status;

    status = Get_Board_Status_Px (handle);
	printf ("Board status is ");
	if (status & SELF_TEST_OK)
   	printf ("SELF TEST OK  ");
	if (status & RAM_OK)
		printf ("RAM OK  ");
	if (status & TIMERS_OK)
		printf ("TIMERS OK  ");
	if (status & BOARD_READY)
		printf ("BOARD READY\n");
	else
		printf ("BOARD NOT READY\n");

    status = Set_Mode_Px(handle, BC_MODE);
    if (status < 0) {
        printf("Set_Mode Failure. %s\n", Print_Error_Px(status));
        ReadKey();
        Release_Module_Px(handle);
        exit(0);
    }
    
    int subaddr;
    printf("Enter subaddress (1–31): ");
    if (scanf("%d", &subaddr) != 1 || subaddr < 1 || subaddr > 31) {
        printf("Invalid subaddress\n");
        return 1;
    }
    while (getchar() != '\n');

    printf("Enter text to send: ");
    char input[65];
    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("Error reading input\n");
        return 1;
    }
    input[strcspn(input, "\n")] = 0;

    char *text = input;
    int len = strlen(text);
    if (len > 64) len = 64;
    int word_count = (len + 1) / 2;
    usint msgdata1[33] = {0};
    //msgdata1[0] = 0x2827;
    
    status = Command_Word_Px(5, RECEIVE, subaddr, word_count, &msgdata1[0]);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Command_Word_Px failure. %s\n", errstr);
        return 1;
    }

    for (int i = 0; i < len; i += 2) {
        unsigned char c1 = (i < len) ? text[i] : 0;
        unsigned char c2 = (i + 1 < len) ? text[i + 1] : 0;
        msgdata1[i / 2 + 1] = (c1 << 8) | c2;
    }

    status = Create_1553_Message_Px(handle, BC2RT, msgdata1, &id1);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Create_1553_Message Failure. %s\n", errstr);
        Release_Module_Px(handle);
        return 1;
    }

    framestruct[0].id = id1;
    framestruct[0].gaptime = 5000;
    framestruct[1].id = 0;

    frameid = Create_Frame_Px(handle, &framestruct[0]);
    if (frameid < 0) {
        Get_Error_String_Px(frameid, errstr);
        printf("Create_Frame Failure. %s\n", errstr);
        Release_Module_Px(handle);
        return 1;
    }

    status = Start_Frame_Px(handle, frameid, FULLFRAME);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Start_Frame Failure. %s\n", errstr);
        Release_Module_Px(handle);
        return 1;
    }

    status = Get_Card_Type_Px(handle, &cardTypeBC);
    if (status >= 0 && !(cardTypeBC == MOD_1553_SF || cardTypeBC == MOD_1760_SF)) {
        for (int i = 0; i < 31; i++) {
            status = Set_RT_Active_Px(handle, i, 0);
            if (status < 0) break;
        }
    }

    status = Run_BC_Px(handle, 1);
    if (status < 0) {
        Get_Error_String_Px(status, errstr);
        printf("Run_BC Failure. %s\n", errstr);
        Release_Module_Px(handle);
        return 1;
    }

    usleep(1000000L);

    if (device != SIMULATE) {
        do {
            status = Get_Msgentry_Status_Px(handle, frameid, 0, &statusword);
        } while ((statusword & 0x8000) != 0x8000);
        printf("Status of message %d: %04X\n", id1, statusword);
        printstat(statusword);
    }

    Stop_Px(handle);
    Release_Module_Px(handle);
    printf("Demo complete.\n");
    ReadKey();
    return 0;
}
