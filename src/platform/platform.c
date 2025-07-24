#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "galahad_px.h"
#include "config.h"
#include "ini.h"
#include "mil_std_1553.h"

#define CONFIG_FILE "config.ini"

int main(int argc, char **argv) {
    Config config;
    memset(&config, 0, sizeof(config));

    if(ini_parse(CONFIG_FILE, command_handler, &config) < 0) {
        printf("Can't load %s\n", CONFIG_FILE);
        return 1;
    }

    if(init_module_1553(&config) != 0) {
        perror("Failed initialization module 1553 \n");
        return 1;
    }

    signal(SIGINT, handle_sigint);

    pthread_join(handle_1553_thread, NULL);
    release_module_1553();

    free(config.cmds.messages_tx);
    free(config.cmds.messages_rx);
    
    printf("Exit.\n");
    return 0;
}

