#ifndef APP_APP_JSON_H_
#define APP_APP_JSON_H_

#include <stddef.h>
#include <stdint.h>

bool app_json_get_string(const char *json, const char *key, char *out, size_t out_size);
bool app_json_get_bool(const char *json, const char *key, bool *out);
bool app_json_get_int(const char *json, const char *key, int32_t *out);
bool app_json_get_float(const char *json, const char *key, float *out);
void app_json_escape(const char *input, char *out, size_t out_size);

#endif
