#include "cvector.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define internal static

#ifdef VECTOR_USE_ASSERT_H
#include <assert.h>
#else
#define assert(...)
#endif

internal
bool
vector_realloc(void** data_block, int element_size, int old_elements, int new_elements)
{
    void* old_block = *data_block;
    void* new_block = realloc(old_block, new_elements * element_size);
    if (!new_block) {
#ifdef VECTOR_LOG_ALLOCS
        fprintf(
            stderr,
            "[vector.c] Failed to realloc: "
            "old_element_count=%d, element_size=%d, element_count=%d. "
            "Falling back to calloc+memcpy.\n",
            element_size,
            element_count
        );
#endif
        new_block = malloc(new_elements * element_size);
        if (!new_block) {
#ifdef VECTOR_LOG_ALLOCS
            fprintf(
                stderr,
                "[vector.c] Failed to calloc: "
                "old_element_count=%d, element_size=%d, element_count=%d.\n",
                element_size,
                element_count
            );
#endif
            return false;
        } else {
            memcpy(new_block, old_block, old_elements * element_size);
            free(old_block);
        }
    }
#ifdef VECTOR_LOG_ALLOCS
    printf(
        "[vector.c] realloc: "
        "old_block=%p old_n_capacity=%d "
        "-> new_block=%d new_n_capacity=%p\n",
        *data_block,
        old_elements,
        new_elements,
        new_block
    );
#endif

    *data_block = new_block;
    return true;
}
internal
void
vector_grow(struct vector* vec, int more, bool exact)
{
    int old_capacity = vec->capacity;
    int new_capacity;
    if (more == 1 && old_capacity == 0 && !exact)
        new_capacity = 8;
    else {
        /* TODO(boz):
            INEFFICIENT! Make two different versions of grow(), one for
            resize/reserve/reserve_more and one for insert/push.
        */
        if (exact) {
            new_capacity = old_capacity + more;
        } else {
            new_capacity = old_capacity;
            do {
                new_capacity += old_capacity;
            } while (new_capacity < old_capacity + more);
        }
    }

    if (!vector_realloc(&vec->data, vec->value_size, vec->size, new_capacity)) {
        fputs("[vector_grow] OOM.", stderr);
        abort();
    } else {
        vec->capacity = new_capacity;
    }
}

void
vector_init(struct vector* vec, int value_size)
{
    memset(vec, 0, sizeof(*vec));
    vec->value_size = value_size;
}
void
vector_destroy(struct vector* vec)
{
    free(vec->data);
    memset(vec, 0, sizeof(*vec));
}
void
vector_clear(struct vector* vec)
{
    vec->size = 0;
}
void
vector_clear_and_free(struct vector* vec)
{
    assert(vec->value_size == sizeof(void*));

    for (int i = 0; i < vec->size; ++i)
        free(((void**)vec->data)[i]);

    vector_clear(vec);
}
void
vector_insert(struct vector* vec, int index, void const* value)
{
    int const size = vec->size;
    int const value_size = vec->value_size;

    assert(index >= 0);
    assert(index <= size);

    if (size + 1 > vec->capacity)
        vector_grow(vec, 1, false);

    int move_amount = size - index;
    if (move_amount) {
        memmove(
            vector_get(vec, index + 1),
            vector_get(vec, index),
            move_amount * value_size
        );
    }

    memcpy(vector_get(vec, index), value, value_size);
    vec->size = size + 1;
}
void
vector_push(struct vector* vec, void const* value)
{
    vector_insert(vec, vec->size, value);
}
void
vector_push_array(struct vector* vec, int n_values, void const* values)
{
    int const value_size = vec->value_size;
    int const vec_size = vec->size;

    assert(value_size > 0);
    assert(n_values >= 0);
    assert(values != NULL);

    vector_reserve_more(vec, n_values);

    memcpy((char*)vec->data + vec_size * value_size, values, n_values * value_size);
    vec->size = vec_size + n_values;
}
void
vector_push_string(struct vector* vec, char const* str)
{
    vector_push_array(vec, strlen(str), str);
}
void
vector_push_sprintf(struct vector* vec, char const* format, ...)
{
    va_list args;
    va_start(args, format);
    vector_push_vsprintf(vec, format, args);
    va_end(args);
}
void
vector_push_vsprintf(struct vector* vec, char const* format, va_list args)
{
    assert(vec->value_size == sizeof(char));

    int i = 0;
    int written = 0;
    char* tmp = 0;
    int last_replacement = 0;

    for (; format[i]; ++i) {
        char c = format[i];

        switch (c) {
            case '%': {
                // Push the 'skipped' part of `format`.
                vector_push_array(vec, i - last_replacement, format + last_replacement);

                // Get the next char.
                c = format[++i];

                switch (c) {
                    case '%': vector_push(vec, &c); break;
                    case 's': vector_push_string(vec, va_arg(args, char const*)); break;
                    case 'l':
                        c = format[++i];
                        switch (c) {
                            case 'i':
                                // -9223372036854775807 == 20
                                vector_reserve_more(vec, 20);
                                written = sprintf(
                                    vector_ref_char(vec, vec->size),
                                    "%lli",
                                    va_arg(args, int64_t)
                                );
                                vec->size += written;
                                break;
                            case 'u':
                                // 9223372036854775807 == 19
                                vector_reserve_more(vec, 19);
                                written = sprintf(
                                    vector_ref_char(vec, vec->size),
                                    "%llu",
                                    va_arg(args, uint64_t)
                                );
                                vec->size += written;
                                break;
                            case 'f':
                                vector_reserve_more(vec, 20);
                                written = sprintf(
                                    vector_ref_char(vec, vec->size),
                                    "%f",
                                    va_arg(args, double)
                                );
                                vec->size += written;
                                break;
                        }
                        break;
                    case 'i':
                        // signed 32 bit -> negative 2 billion == 11 chars.
                        vector_reserve_more(vec, 11);
                        written = sprintf(
                            vector_ref_char(vec, vec->size),
                            "%i",
                            va_arg(args, int32_t)
                        );
                        vec->size += written;
                        break;
                    case 'u':
                        // unsigned 32 bit -> negative 2 billion == 10 chars.
                        vector_reserve_more(vec, 10);
                        written = sprintf(
                            vector_ref_char(vec, vec->size),
                            "%u",
                            va_arg(args, uint32_t)
                        );
                        vec->size += written;
                        break;
                }
                last_replacement = i + 1;
            } break;
        }
    }

    vector_push_string(vec, format + last_replacement);
}
void
vector_push_sprintf_terminated(struct vector* vec, char const* format, ...)
{
    if (vec->size > 0 && ((char*)vec->data)[vec->size - 1] != 0) {
        // We need to overwrite the last \0.
        --vec->size;
    }

    va_list va;
    va_start(va, format);
    vector_push_vsprintf(vec, format, va);
    va_end(va);

    char null = 0;
    vector_push(vec, &null);
}
void
vector_reserve(struct vector* vec, int at_least)
{
    int more = at_least - vec->capacity;
    if (more > 0)
        vector_grow(vec, more, true);
}
void
vector_reserve_more(struct vector* vec, int more)
{
    more += vec->size;
    vector_reserve(vec, more);
}
void*
vector_get(struct vector* vec, int index)
{
    return (char*)vec->data + (vec->value_size * index);
}