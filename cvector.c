#include "cvector.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define internal static

#ifdef VECTOR_USE_ASSERT
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
vector_insert(struct vector* vec, int index, void const* restrict value)
{
    int const size = vec->size;
    int const value_size = vec->value_size;

    assert(index >= 0);
    assert(index <= size);

    if (size + 1 > vec->capacity)
        vector_grow(vec, 1, false);

    int move_amount = size - index;
    if (move_amount > 0) {
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
vector_insert_array(
    struct vector* vec,
    int index,
    int const n_values,
    void const* restrict values
)
{
    int const size = vec->size;
    int const value_size = vec->value_size;

    assert(index <= size);

    // `vector_reserve_more(n_values)` but 'faster' (less going to memory).
    vector_reserve(vec, size + n_values);

    int move_amount = size - index;

    // Move the items already in the vector. {#000}
    /* NOTE(braynstorm):
        Using greater-than should also cover the case where `index > size`.
    */
    if (move_amount > 0)
        memmove(
            vector_get(vec, index + n_values),
            vector_get(vec, index),
            move_amount * value_size
        );

    // Copy the whole vector in the newly cleared up space.
    memcpy(vector_get(vec, index), values, n_values * value_size);

    // Finish the insertion by increasing the size.
    vec->size = size + n_values;
}
void
vector_push(struct vector* vec, void const* restrict value)
{
    vector_insert(vec, vec->size, value);
}
void
vector_push_array(struct vector* vec, int n_values, void const* restrict values)
{
    vector_insert_array(vec, vec->size, n_values, values);
}
void
vector_push_string(struct vector* vec, char const* restrict str)
{
    vector_push_array(vec, strlen(str), str);
}
void
vector_push_sprintf(struct vector* vec, char const* restrict format, ...)
{
    va_list args;
    va_start(args, format);
    vector_push_vsprintf(vec, format, args);
    va_end(args);
}
void
vector_push_vsprintf(struct vector* vec, char const* restrict format, va_list args)
{
    assert(vec->value_size == sizeof(char));

    int i = 0;
    int written = 0;
    char* tmp = 0;
    int last_replacement = 0;

    for (; format[i]; ++i) {
        char c = format[i];
        if (c != '%')
            continue;

        // Push the 'skipped' part of `format`.
        vector_push_array(vec, i - last_replacement, format + last_replacement);

        // Get the next char.
        c = format[++i];

        int size = vec->size;

        switch (c) {
            case '%': vector_push(vec, &c); break;
            case 's': vector_push_string(vec, va_arg(args, char const*)); break;
            case '*':
                c = format[++i];
                switch (c) {
                    case 'c': {
                        unsigned reps = va_arg(args, unsigned);
                        c = va_arg(args, int);
                        vector_reserve_more(vec, reps);
                        memset(vector_ref_char(vec, vec->size), c, reps);
                        vec->size += reps;
                    } break;
                }
                break;
            case 'c':
                c = va_arg(args, int);
                vector_push(vec, &c);
                break;
            case 'l':
                c = format[++i];
                switch (c) {
                    case 'i':
                        // -9223372036854775807 == 20
                        vector_reserve_more(vec, 20);
                        written = sprintf(
                            vector_ref_char(vec, size),
                            "%lli",
                            va_arg(args, int64_t)
                        );
                        vec->size = size + written;
                        break;
                    case 'u':
                        // 9223372036854775807 == 19
                        vector_reserve_more(vec, 19);
                        written = sprintf(
                            vector_ref_char(vec, vec->size),
                            "%llu",
                            va_arg(args, uint64_t)
                        );
                        vec->size = size + written;
                        break;
                    case 'f':
                        vector_reserve_more(vec, 20);
                        written = sprintf(
                            vector_ref_char(vec, vec->size),
                            "%f",
                            va_arg(args, double)
                        );
                        vec->size = size + written;
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
                // unsigned 32 bit -> 4 billion == 10 chars.
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
    }

    // Push the last part of the string.
    // Also handles cases where the sprintf() is called without any %replacements.
    vector_push_string(vec, format + last_replacement);
}
void
vector_push_sprintf_terminated(struct vector* vec, char const* restrict format, ...)
{
    /* NOTE(bozho2):
        `vector_push_sprintf_terminated` could be chained.
        Figure out if it is by checking the last character of the vector before
        inserting anything.
    */
    if (vec->size > 0 && ((char*)vec->data)[vec->size - 1] == 0) {
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
    assert(more >= 0);
    more += vec->size;
    vector_reserve(vec, more);
}
void*
vector_get(struct vector* vec, int index)
{
    return (char*)vec->data + (vec->value_size * index);
}
void
vector_remove(struct vector* vec, int index)
{
    vector_remove_range(vec, index, index + 1);
}
void
vector_remove_range(struct vector* vec, int first, int last)
{
    int const value_size = vec->value_size;
    int const size = vec->size;
    char* data = (char*)vec->data;

    int const n_removed = last - first;
    int const n_moved = size - last;

    assert(size > n_removed);
    assert(first >= 0);
    assert(first <= size);
    assert(last >= 0);
    assert(last <= size);
    assert(first <= last);

    if (n_moved > 0)
        memmove(
            data + first * value_size,
            data + last * value_size,
            value_size * n_moved
        );
    vec->size = size - n_removed;
}