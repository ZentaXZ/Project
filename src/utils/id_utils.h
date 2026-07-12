#ifndef ID_UTILS_H
#define ID_UTILS_H

/**
 * Generate a unique ID string in UUID-like format (8-4-4-4-12 hex digits).
 * @param out_buffer Buffer to write into; must hold at least 37 bytes.
 */
void generate_uuid(char* out_buffer);

#endif // ID_UTILS_H
