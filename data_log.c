#include "data_log.h"
#include <ctype.h>

#define MAX_LINE_LENGTH 1024
#define MAX_COLUMNS 100
#define INITIAL_CHANNEL_CAPACITY 10

DataLog* datalog_create(const char* name) {
    DataLog* log = (DataLog*)malloc(sizeof(DataLog));
    if (!log) return NULL;
    
    log->name = strdup(name);
    log->channel_capacity = INITIAL_CHANNEL_CAPACITY;
    log->channel_count = 0;
    log->channels = (Channel**)malloc(sizeof(Channel*) * log->channel_capacity);
    
    return log;
}

void datalog_clear(DataLog* log) {
    for (size_t i = 0; i < log->channel_count; i++) {
        channel_destroy(log->channels[i]);
    }
    log->channel_count = 0;
}

void datalog_destroy(DataLog* log) {
    if (log) {
        datalog_clear(log);
        free(log->channels);
        free(log->name);
        free(log);
    }
}

int datalog_from_csv(DataLog* log, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1;

    char line[MAX_LINE_LENGTH];
    char* header = NULL;
    char* units = NULL;
    
    // Read header
    if (fgets(line, MAX_LINE_LENGTH, file)) {
        header = strdup(line);
    }
    
    // Read units
    if (fgets(line, MAX_LINE_LENGTH, file)) {
        units = strdup(line);
    }

    // Parse header and create channels
    char* token = strtok(header, ",");
    token = strtok(NULL, ","); // Skip time column
    
    char* unit_token = strtok(units, ",");
    unit_token = strtok(NULL, ","); // Skip time unit

    while (token && unit_token) {
        // Remove newline and whitespace
        char* clean_name = strtok(token, "\n\r ");
        char* clean_unit = strtok(unit_token, "\n\r ");
        
        datalog_add_channel(log, clean_name, clean_unit, 3);
        
        token = strtok(NULL, ",");
        unit_token = strtok(NULL, ",");
    }

    // Parse data
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        char* value_str = strtok(line, ",");
        double timestamp = atof(value_str);
        
        for (size_t i = 0; i < log->channel_count; i++) {
            value_str = strtok(NULL, ",");
            if (value_str) {
                double value = atof(value_str);
                // Add message to channel
                Channel* channel = log->channels[i];
                if (channel->message_count >= channel->message_capacity) {
                    channel->message_capacity *= 2;
                    channel->messages = realloc(channel->messages, 
                        channel->message_capacity * sizeof(Message));
                }
                channel->messages[channel->message_count].timestamp = timestamp;
                channel->messages[channel->message_count].value = value;
                channel->message_count++;
            }
        }
    }

    free(header);
    free(units);
    fclose(file);
    return 0;
}

// csv parsing:

// Helper function to trim whitespace
void trim_whitespace(char* str) {
    char* end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

// Helper function to check if string is numeric
int is_numeric(const char* str) {
    char* endptr;
    strtod(str, &endptr);
    return *endptr == '\0' || isspace((unsigned char)*endptr);
}

// Helper function to split CSV line
char** split_csv_line(char* line, int* count) {
    char** tokens = malloc(MAX_COLUMNS * sizeof(char*));
    *count = 0;
    char* token = strtok(line, ",");
    
    while (token != NULL && *count < MAX_COLUMNS) {
        tokens[*count] = strdup(token);
        trim_whitespace(tokens[*count]);
        (*count)++;
        token = strtok(NULL, ",");
    }
    
    return tokens;
}