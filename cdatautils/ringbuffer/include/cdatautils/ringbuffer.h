#ifndef CDATAUTILS_RING_BUFFER_H
#define CDATAUTILS_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#define restrict
#endif

#define CDATAUTILS_RING_BUFFER_CACHE_LINE_SIZE 64
typedef uint32_t rb_size_t;

/* A multi-consumer, multi-producer, lock-free, power-of-two circular buffer. */
struct ring_buffer;

/* Initializes a ring_buffer.
    capacity MUST be a power-of-two.
*/
void ring_buffer_init(
    struct ring_buffer** restrict,
    rb_size_t capacity,
    rb_size_t value_size
);

/* Destroys a ring_buffer immediately. */
void ring_buffer_destroy(struct ring_buffer* restrict);

/* Blocking insert operation.
    Writes a single item in the buffer, blocking if the buffer is full.
*/
void ring_buffer_push(struct ring_buffer* restrict, void const* restrict item);
/* Tries to push an item in the buffer.
    Returns true if the push was successful.
    Returns false if the push failed.

Failure reasons:
    - The buffer is full.
    - There is someone _about to push_ something in buffer.
*/
bool ring_buffer_maybe_push(struct ring_buffer* restrict, void const* restrict item);

/* Blocking retrieve-and-remove operation.
    Reads on item from the buffer (block if the buffer is empty).
*/
void ring_buffer_pop(struct ring_buffer* restrict, void* restrict out_item);
/* Tries to pop an item from the buffer.
    Returns true if the pop was successful.
    Returns false if the pop failed.

Failure reasons:
    - The buffer is empty.
    - There is someone _about to pop_ something from buffer.
*/
bool ring_buffer_maybe_pop(struct ring_buffer* restrict, void* restrict out_item);

rb_size_t ring_buffer_value_size(struct ring_buffer* restrict);
rb_size_t ring_buffer_capacity(struct ring_buffer* restrict);
/* NOTE:
```
a
```
Clears the buffer, by setting all pointers to 0.
(READ, READ-AHEAD, WRITE, WRITE-AHEAD).

Data race:
    Ensure that no consumers or producers are currently working with this buffer.
*/
void ring_buffer_clear(struct ring_buffer* restrict);
/* Returns the difference between WRITE and READ.

This value will always be inaccurate if used with multiple producers/consumers.
*/
rb_size_t ring_buffer_size(struct ring_buffer* restrict);

#ifdef __cplusplus
}
#endif

#endif
