#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "../../../third_party/cJSON/cJSON.h"

/**
 * Load JSON from a file.
 * @param path File path
 * @return cJSON* object, or NULL on error. Caller must free with cJSON_Delete().
 */
cJSON* json_utils_load_file(const char* path);

/**
 * Save JSON to a file.
 * @param path File path
 * @param root cJSON object to save
 * @return true on success
 */
bool json_utils_save_file(const char* path, cJSON* root);

#endif // JSON_UTILS_H
