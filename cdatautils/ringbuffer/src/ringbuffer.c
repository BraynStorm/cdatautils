#include <cdatautils/ringbuffer.h>

#include <stdalign.h>
#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>

#ifdef CDATAUTILS_RINGBUFFER_USE_ASSERT
#include <assert.h>
#else
#define assert(...) (void)(__VA_ARGS__)
#endif

struct ring_buffer
{
    void* data;
    rb_size_t value_size;
    rb_size_t capacity;
    alignas(CDATAUTILS_RING_BUFFER_CACHE_LINE_SIZE * 2) _Atomic rb_size_t read;
    alignas(CDATAUTILS_RING_BUFFER_CACHE_LINE_SIZE * 2) _Atomic rb_size_t read_ahead;
    alignas(CDATAUTILS_RING_BUFFER_CACHE_LINE_SIZE * 2) _Atomic rb_size_t write;
    alignas(CDATAUTILS_RING_BUFFER_CACHE_LINE_SIZE * 2) _Atomic rb_size_t write_ahead;
};

void
ring_buffer_init(
    struct ring_buffer** restrict rb,
    rb_size_t capacity,
    rb_size_t value_size
)
{
    struct ring_buffer* _rb = malloc(sizeof(*_rb));

    assert(_rb);
    assert(capacity > 1);
    assert(((capacity - 1) & capacity) == 0); // power of two

    _rb->data = calloc(capacity, value_size);
    assert(_rb->data);

    _rb->value_size = value_size;
    _rb->capacity = capacity;
    atomic_init(&_rb->read, 0);
    atomic_init(&_rb->write, 0);
    atomic_init(&_rb->read_ahead, 0);
    atomic_init(&_rb->write_ahead, 0);

    *rb = _rb;
}

void
ring_buffer_destroy(struct ring_buffer* restrict rb)
{
    free(rb->data);
    rb->data = NULL;
    free(rb);
}
bool
ring_buffer_maybe_push(struct ring_buffer* restrict rb, void const* restrict item)
{
    rb_size_t const cap = rb->capacity;
    rb_size_t const value_size = rb->value_size;
    rb_size_t const cap_mask = cap - 1u;
    char* const data = rb->data;

    rb_size_t wa = atomic_load_explicit(&rb->write_ahead, memory_order_acquire);

    /* If the buffer is "full", can't push. */
    if (wa - atomic_load_explicit(&rb->read, memory_order_acquire) >= cap)
        return false;

    /* If there is someone writing behind us, we will have to spin on the write
    to WRITE, so just don't. */
    if (atomic_load_explicit(&rb->write, memory_order_acquire) != wa)
        return false;

    /* If someone stole our WRITE-AHEAD slot, we can't push.
    Otherwise, take the WRITE-AHEAD slot, as it guaranteed that we will not block.
    */
    if (!atomic_compare_exchange_strong_explicit(
            &rb->write_ahead,
            &wa,
            wa + 1,
            memory_order_acquire,
            memory_order_acquire
        ))
        return false;

    /* Alright, (WRITE == WRITE-AHEAD) && (WRITE-AHEAD - READ < cap).
    We can finally read. */
    memcpy(data + (wa & cap_mask) * (size_t)value_size, item, value_size);

    /* "Increment" WRITE. */
    atomic_store_explicit(&rb->write, wa + 1, memory_order_release);
    return true;
}
bool
ring_buffer_push(struct ring_buffer* restrict rb, void const* restrict item)
{
    rb_size_t const cap = rb->capacity;
    rb_size_t const value_size = rb->value_size;
    rb_size_t const cap_mask = cap - 1u;
    char* const data = rb->data;
    rb_size_t wa;

    /* Initial WRITE-AHEAD. */
    wa = atomic_load_explicit(&rb->write_ahead, memory_order_acquire);

    /* If someone stole our WRITE-AHEAD slot, we have to wait.
    Otherwise, take the WRITE-AHEAD slot, as it guaranteed that we will not block.
    */
    do {
        /* If the buffer is "full", can't push. */
        if (wa - atomic_load_explicit(&rb->read, memory_order_acquire) >= cap)
            return false;
    } while (!atomic_compare_exchange_weak_explicit(
        &rb->write_ahead,
        &wa,
        wa + 1,
        memory_order_acquire,
        memory_order_acquire
    ));
    /* We've acquired a WRITE-AHEAD slot. */

    /* Actually write the item in the buffer. */
    memcpy(data + (wa & cap_mask) * (size_t)value_size, item, value_size);

    /* When WRITE reaches our WRITE-AHEAD, set WRITE = WRITE-AHEAD + 1. */
    while (atomic_load_explicit(&rb->write, memory_order_acquire) != wa)
        ;
    atomic_store_explicit(&rb->write, wa + 1, memory_order_release);
    return true;
}
void
ring_buffer_deadlock_push(struct ring_buffer* restrict rb, void const* restrict item)
{
    rb_size_t const cap = rb->capacity;
    rb_size_t const value_size = rb->value_size;
    rb_size_t const cap_mask = cap - 1u;
    char* const data = rb->data;

    /* Initial WRITE-AHEAD. */
    rb_size_t r;
    rb_size_t wa = atomic_load_explicit(&rb->write_ahead, memory_order_acquire);

    /* If someone stole our WRITE-AHEAD slot, we have to wait.
    Otherwise, take the WRITE-AHEAD slot, as it guaranteed that we will not block.
    */
    do {
        r = atomic_load_explicit(&rb->read, memory_order_acquire);
        while (wa - r >= cap) {
            /* If the buffer is "full", can't push. This is the potential DEADLOCK. */
            wa = atomic_load_explicit(&rb->write_ahead, memory_order_acquire);
            r = atomic_load_explicit(&rb->read, memory_order_acquire);
        }
    } while (!atomic_compare_exchange_weak_explicit(
        &rb->write_ahead,
        &wa,
        wa + 1,
        memory_order_acquire,
        memory_order_acquire
    ));
    /* We've acquired a WRITE-AHEAD slot. */

    /* Actually write the item in the buffer. */
    memcpy(data + (wa & cap_mask) * (size_t)value_size, item, value_size);

    /* When WRITE reaches our WRITE-AHEAD, set WRITE = WRITE-AHEAD + 1. */
    while (atomic_load_explicit(&rb->write, memory_order_acquire) != wa)
        ;
    atomic_store_explicit(&rb->write, wa + 1, memory_order_release);
}

void
ring_buffer_deadlock_pop(struct ring_buffer* restrict rb, void* restrict out_item)
{
    rb_size_t const cap = rb->capacity;
    rb_size_t const value_size = rb->value_size;
    rb_size_t const cap_mask = cap - 1u;
    char const* const data = rb->data;

    rb_size_t w = atomic_load_explicit(&rb->write, memory_order_acquire);
    rb_size_t ra = atomic_load_explicit(&rb->read_ahead, memory_order_acquire);

    /* If someone stole our READ-AHEAD slot, we have to wait.
    Otherwise, take the READ-AHEAD slot, as it guaranteed that we will not block.
    */
    do {
        /* If the buffer is "empty", can't pop. This is the potential DEADLOCK. */
        while (ra == w) {
            ra = atomic_load_explicit(&rb->read_ahead, memory_order_acquire);
            w = atomic_load_explicit(&rb->write, memory_order_acquire);
        }
    } while (!atomic_compare_exchange_weak_explicit(
        &rb->read_ahead,
        &ra,
        ra + 1,
        memory_order_acquire,
        memory_order_acquire
    ));
    /* We've acquired a READ-AHEAD slot. */

    /* Actually read the item. */
    memcpy(out_item, data + (ra & cap_mask) * (size_t)value_size, value_size);

    /* When READ reaches our READ-AHEAD, set READ = READ-AHEAD + 1. */
    while (atomic_load_explicit(&rb->read, memory_order_acquire) != ra)
        ;
    atomic_store_explicit(&rb->read, ra + 1, memory_order_release);
}
bool
ring_buffer_pop(struct ring_buffer* restrict rb, void* restrict out_item)
{
    rb_size_t const cap = rb->capacity;
    rb_size_t const value_size = rb->value_size;
    rb_size_t const cap_mask = cap - 1u;
    char const* const data = rb->data;

    /* Initial READ-AHEAD slot. */
    rb_size_t ra = atomic_load_explicit(&rb->read_ahead, memory_order_acquire);

    /* If someone stole our READ-AHEAD slot, we have to wait.
    Otherwise, take the READ-AHEAD slot, as it guaranteed that we will not block.
    */
    do {
        /* If the buffer is "empty", can't pop. */
        if (atomic_load_explicit(&rb->write, memory_order_acquire) == ra)
            return false;
    } while (!atomic_compare_exchange_weak_explicit(
        &rb->read_ahead,
        &ra,
        ra + 1,
        memory_order_acquire,
        memory_order_acquire
    ));
    /* We've acquired a READ-AHEAD slot. */

    /* Actually read the item. */
    memcpy(out_item, data + (ra & cap_mask) * (size_t)value_size, value_size);

    /* When READ reaches our READ-AHEAD, set READ = READ-AHEAD + 1. */
    while (atomic_load_explicit(&rb->read, memory_order_acquire) != ra)
        ;
    atomic_store_explicit(&rb->read, ra + 1, memory_order_release);
    return true;
}
bool
ring_buffer_maybe_pop(struct ring_buffer* restrict rb, void* restrict out_item)
{
    rb_size_t const value_size = rb->value_size;
    rb_size_t const cap_mask = rb->capacity - 1u;
    char const* const data = rb->data;

    rb_size_t ra = atomic_load_explicit(&rb->read_ahead, memory_order_acquire);

    /* If the buffer is "empty", nothing to pop. */
    if (atomic_load_explicit(&rb->write, memory_order_acquire) == ra)
        return false;

    /* If there is someone reading behind us, we will have to spin on the write
    to READ, so just don't. */
    if (atomic_load_explicit(&rb->read, memory_order_acquire) != ra)
        return false;

    /* If someone managed to steal our READ-AHEAD slot, we can't pop.
    Otherwise, take the READ-AHEAD slot, as it guaranteed that we will not block.
    */
    if (!atomic_compare_exchange_strong_explicit(
            &rb->read_ahead,
            &ra,
            ra + 1,
            memory_order_acquire,
            memory_order_acquire
        ))
        return false;

    /* Alright, (READ == READ-AHEAD) && (READ-AHEAD != WRITE).
    We can finally read. */

    memcpy(out_item, data + (ra & cap_mask) * (size_t)value_size, value_size);

    /* "Increment" READ. */
    atomic_store_explicit(&rb->read, ra + 1, memory_order_release);

    return true;
}

rb_size_t
ring_buffer_capacity(struct ring_buffer* restrict rb)
{
    return rb->capacity;
}
rb_size_t
ring_buffer_value_size(struct ring_buffer* restrict rb)
{
    return rb->value_size;
}
void
ring_buffer_clear(struct ring_buffer* restrict rb)
{
    rb_size_t w;
    rb_size_t ra;

    ra = atomic_load_explicit(&rb->read_ahead, memory_order_acquire);

    /* Acquire all READ-AHEAD slots after the current one, until WRITE. */
    do {
        w = atomic_load_explicit(&rb->write, memory_order_acquire);

        /* Check if we've already cleared it (unlikely, but possible). */
        if (ra == w)
            return;
    } while (!atomic_compare_exchange_weak_explicit(
        &rb->read_ahead,
        &ra,
        w,
        memory_order_acquire,
        memory_order_acquire
    ));

    /* Wait for other consumers to finish their work. */
    while (atomic_load_explicit(&rb->read, memory_order_acquire) != ra)
        ;

    /* "Increment" the write pointer to the target value (WRITE). */
    atomic_store_explicit(&rb->read, w, memory_order_release);
}
rb_size_t
ring_buffer_size(struct ring_buffer* restrict rb)
{
    return atomic_load_explicit(&rb->write, memory_order_acquire)
           - atomic_load_explicit(&rb->read, memory_order_acquire);
}
