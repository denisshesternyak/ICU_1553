#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "config.h"
#include "ini.h"
#include "server.h"

#define CONFIG_FILE "config.ini"

int main() {
    Config config;
    memset(&config, 0, sizeof(config));

    if(ini_parse(CONFIG_FILE, command_handler, &config) < 0) {
        printf("Can't load %s\n", CONFIG_FILE);
        return 1;
    }

    if(init_socket(&config) < 0) {
        perror("Failed initialization module 1553 \n");
        return 1;
    }

    signal(SIGINT, handle_sigint);

    pthread_join(handle_rxclient_thread, NULL);  
    pthread_join(handle_txclient_thread, NULL);  

    free(config.cmds.messages);

    printf("Exit.\n");
    return 0;
}