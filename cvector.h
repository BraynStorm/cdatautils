#ifndef VECTOR_H
#define VECTOR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#define restrict
#endif

/* A growable array of same-sized items.

It is expected that `value_size` does not change and is not changed by the user.
*/
struct vector
{
    /* Pointer to buffer that holds the ITEMS.

    This value is NULL when capacity is 0.
    This value is NOT NULL when capacity is not 0.

    This value is always a result of a call to one of:
        - malloc
        - realloc
        - calloc

    Modifying manually:
        Not advised.

        If needed, be sure to limit `capacity` and `size` to no more than the
        size of the allocated block divided by `value_size`.
    */
    void* data;

    /* The number of ITEMS in the vector currently.

    This is not an `unsigned` value, because it would make life more complicated.

    Must never be bigger than `capacity`.

    Modifying manually:
        Setting this value to 0 is equivalent to clearing the vector.

        Decreasing this value:
            - below 0 is illegal.
            - by 1 (previous rule still applies) is equivalent to removing
            the last element of the vector.

        Increasing this value:
            - up to `capacity` is permitted, but may result in old or
            uninitialized data being able to be read.
            - beyond `capacity` is illegal, and will produce crashes that are
            hard to debug.
    */
    int size;

    /* The number of ITEMS the vector can hold, before having to grow.

    This is not an `unsigned` value, because it would make life more complicated.

    This value must never be negative.

    Modifying manually:
        Don't.
    */
    int capacity;

    /* The size of an ITEM in bytes.

    This is not an `unsigned` value, because it would make life more complicated.

    If this value is zero, the vector is not initialized / has been destroyed.

    Modifying manually:
        Don't. Really.

        `vector` uses this to determine the boundaries of the items in the data
        block, decreasing this is legal, but highly dangerous.
        Use a lot of tests (caution).
        Increasing this is VERY dangerous and could easily send you into UB town
        (the real one).
    */
    int value_size;
};

/* Creates a vector suitable for a given type.  */
#ifdef __cplusplus
#define vector_create(type)    \
    {                          \
        0, 0, 0, sizeof(type), \
    }
#else
#define vector_create(type)   \
    (struct vector)           \
    {                         \
        0, 0, 0, sizeof(type) \
    }
#endif
/* Initializes the vector for use.

Notes:
    - Calling this on a vector with non-zero capacity results in memory leaks.
    - Must be called after vector_destroy if a vector is to be reused.

See also vector_create(type)
*/
void vector_init(struct vector* vec, int value_size);

/* Frees the internal buffer of the vector.

After this function, the whole struct is zeroed to prevent errors.

Equivalent to:
```
free(vec->data);
vec->value_size = 0;
vec->size = 0;
vec->capacity = 0;
vec->data = 0;
```
*/
void vector_destroy(struct vector* vec);

/* Destroys all items in the vector.

Notes:
    - The memory is not freed.
    - If the vector is empty, calling this is a noop.

Equivalent to:
```
vec->size = 0;
```
*/
void vector_clear(struct vector* vec);

/* Calls free() on each element of the vector, then calls `vector_clear`.

Notes:
    - The caller is responsible for ensuring the type of the content is suitable
for this behaviour. Notably:
        - the values are pointers.
        - the values are allocated by malloc/calloc/realloc.
        - the values have not already been freed.

Equivalent to:
```
    for (int i = 0; i < vec->size; ++i) {
        free(vector_get_voidptr(vec, i));
    }
    vector_clear(vec);
```
*/
void vector_clear_and_free(struct vector* vec);

/* Inserts an element at the given location.

Grows the vector if necessary.
*/
void vector_insert(struct vector* vec, int index, void const* restrict value);

/* Inserts all items from the provided array in the vector, starting from the given
index.

Equivalent to:
```
for (int i = 0; i < n_values; ++i) {
    vector_insert(vec, index + i, &values[i]);
}
```
*/
void vector_insert_array(
    struct vector* vec,
    int index,
    int n_values,
    void const* restrict values
);

/* Inserts an element at the end of the vector.

Grows the vector if necessary.

Equivalent to:
```
    vector_insert(vec, vec->size, value);
```
*/
void vector_push(struct vector* vec, void const* restrict value);

/* Inserts `n_values` items in to the vector, reading each one consecutively
from `values`.

Grows the vector if necessary with one call to `vector_grow`.
Performs a bitwise copy of the values, inserting .

Note:
    - The user is responsible for ensuring `values` points to an "array" of
`n_values` items and that the type of these items is the same as the type
of the vector.

Equivalent to:
```
    T const values[] = { ... };
    int const n_values = sizeof(values) / sizeof(values[0]);

    vector_reserve_more(vec, n_values);
    for (int i = 0; i < n_values; ++i)
        vector_push(vec, &values[i]);
```
*/
void vector_push_array(struct vector* vec, int n_values, void const* restrict values);

/* Pushes the characters of a null-terminated string `string` in the vector.

Notes:
    - Does NOT add a \0 at the end. Push it manually if needed.

Equivalent to:
```
vector_push_array(vec, strlen(string), string);
```
*/
void vector_push_string(struct vector* vec, char const* restrict string);

#if defined(__clang__) || defined(__GNUC__)
#define CVECTOR_PRINTF_ATTRIBUTE __attribute__((format(printf, 2, 3)))
#define CVECTOR_PRINTF_FSTRING
#elif defined(MSVC)
#define CVECTOR_PRINTF_ATTRIBUTE
#define CVECTOR_PRINTF_FSTRING _Printf_format_string_
#else
#define CVECTOR_PRINTF_ATTRIBUTE
#define CVECTOR_PRINTF_FSTRING
#endif

/* Pushes formatted data to the internal buffer.

Similar in spirit to sprintf() but has slightly different semantics.

TODO:
    Rename this to `vector_pushf` and make `vector_push_sprintf` match the
sprintf spec.

TODO:
    Add `vector_insertf`.

Supports:
    - `%%`  -> Literal '%'
    - `%s`  -> char const*
    - `%c`  -> char
    - `%*c` -> char, repeated N times. Expects uint32_t N, followed by a char.
    - `%f`  -> double
    - `%u`  -> uint32_t
    - `%lu` -> uint64_t
    - `%i`  -> int32_t
    - `%li` -> int64_t

Notes:
    - Does not null-terminate the buffer (no \0).
        For that, use `vector_push_sprintf_terminated`.
*/
void vector_push_sprintf(
    struct vector* vec,
    CVECTOR_PRINTF_FSTRING char const* restrict format,
    ...
) CVECTOR_PRINTF_ATTRIBUTE;
void vector_push_vsprintf(
    struct vector* vec,
    char const* restrict format,
    va_list args
);
void vector_push_sprintf_terminated(
    struct vector* vec,
    CVECTOR_PRINTF_FSTRING char const* restrict format,
    ...
) CVECTOR_PRINTF_ATTRIBUTE;

/* Ensures the vector can fit at least `at_least` items.

If the can already fit them, this is a noop.
*/
void vector_reserve(struct vector* vec, int at_least);

/* Ensures the vector can fit `more` more items.

If the can already fit them, this is a noop.
*/
void vector_reserve_more(struct vector* vec, int more);

/* Returns a pointer to the `index`th element. No bounds checking.

Equivalent to:
```
(char*)vec->data + (vec->value_size * index);
```

See also GENERATE_VECTOR_GETTERS.
See also GENERATE_VECTOR_REF_GETTER.
See also GENERATE_VECTOR_VALUE_GETTER.
*/
void* vector_get(struct vector* vec, int index);

/* Returns a typed ref from a `struct vector`.

Useful if you need a one-off ref getter for a given type, and dont want to
pollute the namespace.

Parameters:
    struct vector* vector,
    int index,
    <TYPE> type,

Notes:
    - It is the user's responsibilty to ensure the type is correct.

Example:
```
    struct vector v;
    vector_init(&v, sizeof(short));

    short a_short = 741;
    vector_push(&v, &a_short);

    short* first_short = vector_ref_generic(&v, 0, short);
    // *first_short == 741;
```
*/
#define vector_ref_generic(vector, index, type) (((type)*)vector_get(vector, index))

/* Returns a typed value from a `struct vector`.

Useful if you need a one-off getter for a given type, and don't want to pollute
the namespace.

Parameters:
    struct vector* vector,
    int index,
    <TYPE> type,

Notes:
    - It is the user's responsibilty to ensure the type is correct.

Equivalent to:
```
(*vector_ref_generic(vector, index, type))
```

Example:
```
    struct vector v;
    vector_init(&v, sizeof(short));

    short a_short = 741;
    vector_push(&v, &a_short);

    short first_short = vector_get_generic(&v, 0, short);
    // first_short == 741;
```
*/
#define vector_get_generic(vector, index, type) \
    (*vector_ref_generic(vector, index, type))

/* Generates a getter for a `struct vector` like `TYPE* vector_ref_NAME(struct
vector*, int index)`.

The definitions are marked as `static inline` so they are kinda hard on the
linker but do not require LTO to inline.

See the `vector.h` header for examples.
 */
#define GENERATE_VECTOR_REF_GETTER(type, name)                           \
    static inline type* vector_ref_##name(struct vector* vec, int index) \
    {                                                                    \
        return (type*)vector_get(vec, index);                            \
    }

/* Generates a getter for a `struct vector` like `TYPE vector_get_NAME(struct
vector*, int index)`.

The definitions are marked as `static inline` so they are kinda hard on the
linker but do not require LTO to inline.

See the `vector.h` header for examples.
*/
#define GENERATE_VECTOR_VALUE_GETTER(type, name)                        \
    static inline type vector_get_##name(struct vector* vec, int index) \
    {                                                                   \
        return *(type*)vector_get(vec, index);                          \
    }

/* Generates two getters for a `struct vector` - a by-value and a by-ref getter.

See the `vector.h` header for examples.
*/
#define GENERATE_VECTOR_GETTERS(type, name)  \
    GENERATE_VECTOR_VALUE_GETTER(type, name) \
    GENERATE_VECTOR_REF_GETTER(type, name)

GENERATE_VECTOR_GETTERS(long long, longlong)
GENERATE_VECTOR_GETTERS(long, long)
GENERATE_VECTOR_GETTERS(int, int)
GENERATE_VECTOR_GETTERS(short, short)
GENERATE_VECTOR_GETTERS(char, char)

GENERATE_VECTOR_GETTERS(int64_t, i64)
GENERATE_VECTOR_GETTERS(int32_t, i32)
GENERATE_VECTOR_GETTERS(int16_t, i16)
GENERATE_VECTOR_GETTERS(int8_t, i8)
GENERATE_VECTOR_GETTERS(uint64_t, u64)
GENERATE_VECTOR_GETTERS(uint32_t, u32)
GENERATE_VECTOR_GETTERS(uint16_t, u16)
GENERATE_VECTOR_GETTERS(uint8_t, u8)
GENERATE_VECTOR_GETTERS(double, f64)
GENERATE_VECTOR_GETTERS(float, f32)

GENERATE_VECTOR_GETTERS(char*, string)

GENERATE_VECTOR_GETTERS(void*, voidptr)
GENERATE_VECTOR_GETTERS(int*, intptr)

GENERATE_VECTOR_REF_GETTER(struct vector, vector);

#ifdef __cplusplus
#undef restrict
}
#endif

#endif
