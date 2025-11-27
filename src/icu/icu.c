#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "config.h"
#include "logger.h"
#include "ini.h"
#include "mil_std_1553.h"
#include "client.h"

#define CONFIG_FILE "config.ini"
#define LOG_FILE "icu.log"

int main(int argc, char **argv) {
    Config config;
    memset(&config, 0, sizeof(config));

    if (init_logger(LOG_FILE) != 0) {
        printf("Error: Cannot initialize %s\n", LOG_FILE);
        return 1;
    }

    if(ini_parse(CONFIG_FILE, command_handler, &config) < 0) {
        printf("Error: Cannot load %s\n", CONFIG_FILE);
        return 1;
    }

    add_log("Config parsed successfully.");

    if(init_module_1553(&config) != 0) {
        perror("Failed initialization module 1553 \n");
        return 1;
    }
    add_log("Initialized module 1553 successfully.");

    if(init_socket(&config) != 0) {
        fprintf(stderr, "Failed initialization socket\n");
        return 1;
    }
    add_log("Socket initialized successfully.");

    signal(SIGINT, handle_sigint);

    pthread_join(handle_1553_thread, NULL);
    release_module_1553();

    pthread_join(handle_rxclient_thread, NULL);    
    pthread_join(handle_txclient_thread, NULL);    

    free(config.cmds.messages_rx);
    free(config.cmds.messages_tx);
    close_logger();
    
    printf("Exit.\n");
    return 0;
}