#include "str.h"

#include <stdlib.h>
#include <string.h>

static const char escaped_chars[] = {
    '\a', '\b', '\f', '\n', '\r', '\t', '\v', '\"', '\0'};
static const char unescaped_chars[] = {
    'a', 'b', 'f', 'n', 'r', 't', 'v', '\"', '0'};

static int find_escaped_char(const char c) {
    for (size_t i = 0; i < sizeof(escaped_chars) / sizeof(char); i++) {
        if (escaped_chars[i] == c) {
            return i;
        }
    }

    return -1;
}

static int find_unescaped_char(const char c) {
    for (size_t i = 0; i < sizeof(unescaped_chars) / sizeof(char); i++) {
        if (unescaped_chars[i] == c) {
            return i;
        }
    }

    return -1;
}

char* str_escape(const char* s) {
    size_t len = strlen(s);
    size_t num_replace = 0;

    for (size_t i = 0; i < len; i++) {
        if (find_escaped_char(s[i]) != -1) {
            num_replace++;
        }
    }

    char* result = malloc(len + 1 + num_replace);

    if (num_replace == 0) {
        strcpy(result, s);
    } else {
        size_t j = 0;
        for (size_t i = 0; i < len; i++) {
            int pos = find_escaped_char(s[i]);
            if (pos != -1) {
                result[j++] = '\\';
                result[j] = unescaped_chars[pos];
            } else {
                result[j] = s[i];
            }
            j++;
        }
        result[j] = '\0';
    }

    return result;
}

char* str_unescape(const char* s) {
    size_t len = strlen(s);
    size_t num_replace = 0;

    size_t i = 0;
    while (i < len) {
        if (s[i] == '\\' && i < len - 1) {
            if (find_unescaped_char(s[i + 1]) != -1) {
                num_replace++;
                i++;
            }
        }
        i++;
    }

    char* result = malloc(len + 1 - num_replace);

    if (num_replace == 0) {
        strcpy(result, s);
    } else {
        size_t i = 0;
        size_t j = 0;
        while (i < len) {
            if (s[i] == '\\' && i < len - 1) {
                int pos = find_unescaped_char(s[i + 1]);
                if (pos != -1) {
                    result[j] = escaped_chars[pos];
                    i++;
                } else {
                    result[j] = '\\';
                }
            } else {
                result[j] = s[i];
            }
            i++;
            j++;
        }
        result[j] = '\0';
    }

    return result;
}
