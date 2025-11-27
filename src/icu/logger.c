#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static FILE* log_file = NULL;
static char log_filename[LOG_FILENAME_MAX_LEN] = DEFAULT_LOG_FILENAME;
static char time_buf[TIME_BUFFER_SIZE];

int init_logger(const char* filename) {
    if (filename != NULL) {
        strncpy(log_filename, filename, sizeof(filename) - 1);
        log_filename[sizeof(filename) - 1] = '\0';
    }
    
    log_file = fopen(log_filename, "a");
    if (log_file == NULL) {
        return -1;
    }
    
    time_t now = time(NULL);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(log_file, "=== LOG STARTED AT %s ===\n", time_buf);
    fflush(log_file);
    
    return 0;
}

void add_log(const char* message) {
    if (log_file == NULL || message == NULL) {
        return;
    }
    
    time_t now = time(NULL);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(log_file, "[%s] %s\n", time_buf, message);
    fflush(log_file);
}

void close_logger() {
    if (log_file == NULL) {
        return;
    }
    
    time_t now = time(NULL);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(log_file, "=== LOG ENDED AT %s ===\n\n", time_buf);
    fclose(log_file);
    log_file = NULL;
}