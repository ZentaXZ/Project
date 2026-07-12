#include "json_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cJSON* json_utils_load_file(const char* path) {
    if (!path) return NULL;

    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "[JSON] Error opening file: %s\n", path);
        return NULL;
    }

    // Read file content
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';
    fclose(file);

    // Parse JSON
    cJSON* root = cJSON_Parse(buffer);
    free(buffer);

    if (!root) {
        fprintf(stderr, "[JSON] Parse error in file: %s\n", path);
        return NULL;
    }

    return root;
}

bool json_utils_save_file(const char* path, cJSON* root) {
    if (!path || !root) return false;

    char* json_string = cJSON_Print(root);
    if (!json_string) return false;

    FILE* file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "[JSON] Error creating file: %s\n", path);
        free(json_string);
        return false;
    }

    fprintf(file, "%s", json_string);
    fclose(file);
    free(json_string);

    return true;
}
