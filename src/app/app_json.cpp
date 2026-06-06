#include "app/app_json.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{
const char *find_key(const char *json, const char *key)
{
    if ((NULL == json) || (NULL == key))
    {
        return NULL;
    }

    char token[48];
    const int written = snprintf(token, sizeof(token), "\"%s\"", key);
    if ((written <= 0) || (static_cast<size_t>(written) >= sizeof(token)))
    {
        return NULL;
    }

    const char *pos = strstr(json, token);
    if (NULL == pos)
    {
        return NULL;
    }

    pos += strlen(token);
    while (isspace(static_cast<unsigned char>(*pos)))
    {
        pos++;
    }

    if (':' != *pos)
    {
        return NULL;
    }

    pos++;
    while (isspace(static_cast<unsigned char>(*pos)))
    {
        pos++;
    }

    return pos;
}

}

bool app_json_get_string(const char *json, const char *key, char *out, size_t out_size)
{
    const char *pos = find_key(json, key);
    if ((NULL == pos) || ('"' != *pos) || (NULL == out) || (0U == out_size))
    {
        return false;
    }

    pos++;
    char *dst = out;
    size_t remaining = out_size - 1U;

    while (('\0' != *pos) && ('"' != *pos))
    {
        char ch = *pos;
        if (('\\' == ch) && ('\0' != pos[1]))
        {
            pos++;
            ch = *pos;
        }

        if (remaining > 0U)
        {
            *dst = ch;
            dst++;
            remaining--;
        }

        pos++;
    }

    *dst = '\0';
    return '"' == *pos;
}

bool app_json_get_bool(const char *json, const char *key, bool *out)
{
    const char *pos = find_key(json, key);
    if ((NULL == pos) || (NULL == out))
    {
        return false;
    }

    if (0 == strncmp(pos, "true", 4U))
    {
        *out = true;
        return true;
    }

    if (0 == strncmp(pos, "false", 5U))
    {
        *out = false;
        return true;
    }

    return false;
}

bool app_json_get_int(const char *json, const char *key, int32_t *out)
{
    const char *pos = find_key(json, key);
    if ((NULL == pos) || (NULL == out))
    {
        return false;
    }

    char *end = NULL;
    const long value = strtol(pos, &end, 10);
    if (end == pos)
    {
        return false;
    }

    *out = static_cast<int32_t>(value);
    return true;
}

bool app_json_get_float(const char *json, const char *key, float *out)
{
    const char *pos = find_key(json, key);
    if ((NULL == pos) || (NULL == out))
    {
        return false;
    }

    char *end = NULL;
    const float value = strtof(pos, &end);
    if (end == pos)
    {
        return false;
    }

    *out = value;
    return true;
}

void app_json_escape(const char *input, char *out, size_t out_size)
{
    if ((NULL == out) || (0U == out_size))
    {
        return;
    }

    if (NULL == input)
    {
        out[0] = '\0';
        return;
    }

    char *dst = out;
    size_t remaining = out_size - 1U;

    while (('\0' != *input) && (remaining > 0U))
    {
        if (('"' == *input) || ('\\' == *input))
        {
            if (remaining < 2U)
            {
                break;
            }

            *dst++ = '\\';
            *dst++ = *input++;
            remaining -= 2U;
        }
        else
        {
            *dst++ = *input++;
            remaining--;
        }
    }

    *dst = '\0';
}
