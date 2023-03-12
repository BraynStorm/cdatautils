#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#define restrict
#endif

struct text
{
    union
    {
        struct
        {
            char* data;
            int64_t size;
        };
        struct
        {
            char chars[16];
        };
    };
};

typedef struct text text_t;

text_t text_from_utf8_z(char const* string_z);
text_t text_from_utf8(char const* string, int64_t size);

/* creates new null-terminated string from the given text. */
char* text_to_utf8_z(text_t);

/* creates new null-terminated string from the given text's range. */
char* text_range_to_utf8_z(text_t, int64_t first, int64_t last);

text_t text_copy(text_t);

/* copies the first n_chars into a new text instance. */
text_t text_copy_first(text_t, int64_t n_chars);

/* copies the range of characters into a new text instance. */
text_t text_copy_range(text_t, int64_t first, int64_t last);

/* concatenate the two strings. */
text_t text_concat(text_t left, text_t right);

bool text_contains_char(text_t haystack, char needle);
bool text_contains_text(text_t haystack, text_t needle);
bool text_contains_utf8_z(text_t haystack, text_t needle);

/* Destroy a text instance. */
void text_destroy(text_t*);

#ifdef __cplusplus
}
#endif
