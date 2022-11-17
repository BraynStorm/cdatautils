#ifndef CDATAUTILS_RING_BUFFER_H
#define CDATAUTILS_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#define restrict
#endif

#ifndef CDATAUTILS_RING_BUFFER_CACHE_LINE_SIZE
#define CDATAUTILS_RING_BUFFER_CACHE_LINE_SIZE 64
#endif

typedef uint32_t rb_size_t;

/* A multi-consumer, multi-producer, lock-free, power-of-two circular buffer. */
struct ring_buffer;

/* Initializes a ring_buffer.

Not thread-safe.

Preconditions:
    - capacity MUST be a power-of-two and > 1.
    - value_size MUST be > 0
*/
void ring_buffer_init(
    struct ring_buffer** restrict,
    rb_size_t capacity,
    rb_size_t value_size
);

/* Destroys a ring buffer immediately, free()-ing all resources.

Not thread-safe.
*/
void ring_buffer_destroy(struct ring_buffer* restrict);

/* Pushes a single item on the buffer. Cannot fail. This function WILL DEADLOCK
if the buffer is full and there are no consumers!

This function always either successfully pushes or blocks until there's space
in the buffer.

Thread safe.

Blocking reasons:
    - The buffer is full.
    - Other producers are currently pushing (multi-producer).
*/
void ring_buffer_deadlock_push(struct ring_buffer* restrict, void const* restrict item);
/* Pushes a single item on the buffer. Fails if buffer is full.

Assuming no producer has crashed in the middle of writing, this operation is guaranteed
to NOT deadlock.

Returns true if the push was successful.
Returns false if the buffer was full.

Thread safe.

Blocking reasons:
    - Other producers are currently pushing (multi-producer).

Failure reasons:
    - The buffer is full.
*/
bool ring_buffer_push(struct ring_buffer* restrict, void const* restrict item);
/* Pushes a single item on the buffer. Fails if buffer is full or other producers
are in the middle of pushing.

Returns true if the push was successful.
Returns false if the push failed and the buffer is unchanged.

Thread safe.

Failure reasons:
    - The buffer is full.
    - Other producers are currently pushing (multi-producer).
*/
bool ring_buffer_maybe_push(struct ring_buffer* restrict, void const* restrict item);

/* Pops an item from the buffer, removing it and returning the value in `out_item`.
This function WILL DEADLOCK if the buffer is empty  and there are no producers!
Cannot fail.

Thread safe.

Blocking reasons:
    - The buffer is empty.
    - Other consumers are currently popping (multi-consumer).
*/
void ring_buffer_deadlock_pop(struct ring_buffer* restrict, void* restrict out_item);
/* Pops an item from the buffer, removing it and returning the value in `out_item`.

Returns true if the pop was successful.
Returns false if the pop failed (the buffer was emtpy).

Thread safe.

Failure reasons:
    - The buffer is empty.

Blocking reasons:
    - Other consumers are currently popping (multi-consumer).
*/
bool ring_buffer_pop(struct ring_buffer* restrict, void* restrict out_item);
/* Pops an item from the buffer, removing it and returning the value in `out_item`.

Returns true if the pop was successful.
Returns false if the pop failed.

Failure reasons:
    - The buffer is empty.
    - There is someone _about to pop_ something from buffer.
*/
bool ring_buffer_maybe_pop(struct ring_buffer* restrict, void* restrict out_item);

/* Returns the size of one item.
Thread safe.
*/
rb_size_t ring_buffer_value_size(struct ring_buffer* restrict);

/* Returns the maximum number of items.
Thread safe.
*/
rb_size_t ring_buffer_capacity(struct ring_buffer* restrict);
/* Clears the buffer, by setting READ and READ-AHEAD equal to WRITE.
Thread safe.
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
