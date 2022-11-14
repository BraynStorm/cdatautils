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
    _Atomic rb_size_t read_ahead;
    alignas(CDATAUTILS_RING_BUFFER_CACHE_LINE_SIZE * 2) _Atomic rb_size_t write;
    _Atomic rb_size_t write_ahead;
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
    memset(rb, 0, sizeof(*rb));
}
bool
ring_buffer_maybe_push(struct ring_buffer* restrict rb, void const* restrict item)
{
    rb_size_t const cap = rb->capacity;
    rb_size_t const value_size = rb->value_size;
    rb_size_t const cap_mask = cap - 1u;
    char* const data = rb->data;

    rb_size_t wa = atomic_load(&rb->write_ahead);

    /* If the buffer is "full", can't push. */
    if (wa - atomic_load(&rb->read) >= cap)
        return false;

    /* If there is someone writing ahead of us, we will have to spin on the
    write to WRITE, so just don't. */
    if (atomic_load(&rb->write) != wa)
        return false;

    /* If someone stole our WRITE-AHEAD slot, we can't push.
    Otherwise, take the WRITE-AHEAD slot, as it guaranteed that we will not block.
    */
    if (!atomic_compare_exchange_strong(&rb->write_ahead, &wa, wa + 1))
        return false;

    /* Alright, (WRITE == WRITE-AHEAD) && (WRITE-AHEAD - READ < cap).
    We can finally read. */
    memcpy(data + (wa & cap_mask) * value_size, item, value_size);

    /* "Increment" WRITE. */
    atomic_store(&rb->write, wa + 1);
    return true;
}

void
ring_buffer_push(struct ring_buffer* restrict rb, void const* restrict item)
{
    rb_size_t const cap = rb->capacity;
    rb_size_t const value_size = rb->value_size;
    rb_size_t const cap_mask = cap - 1u;
    char* const data = rb->data;

    /* Acquire a WRITE-AHEAD slot */
    rb_size_t const wa = atomic_fetch_add_explicit(
        &rb->write_ahead,
        1,
        memory_order_acquire
    );

    /* Ensure we will not trample over READ. */
    while (wa - atomic_load_explicit(&rb->read, memory_order_acquire) >= cap)
        ;

    /* Actually write the item in the buffer. */
    memcpy(data + (wa & cap_mask) * value_size, item, value_size);

    /* When WRITE reaches our WRITE-AHEAD, set WRITE = WRITE-AHEAD + 1. */
    while (atomic_load_explicit(&rb->write, memory_order_relaxed) != wa)
        ;
    atomic_store_explicit(&rb->write, wa + 1, memory_order_release);
}

void
ring_buffer_pop(struct ring_buffer* restrict rb, void* restrict out_item)
{
    rb_size_t const cap = rb->capacity;
    rb_size_t const value_size = rb->value_size;
    rb_size_t const cap_mask = cap - 1u;
    char const* const data = rb->data;

    /* Acquire a READ-AHEAD slot. */
    rb_size_t const ra = atomic_fetch_add_explicit(
        &rb->read_ahead,
        1,
        memory_order_acquire
    );

    /* Ensure we will not read past WRITE. */
    while (atomic_load_explicit(&rb->write, memory_order_acquire) - ra == 0)
        ;

    /* Actually read the item. */
    memcpy(out_item, data + (ra & cap_mask) * value_size, value_size);

    /* When READ reaches our READ-AHEAD, set READ = READ-AHEAD + 1. */
    while (atomic_load_explicit(&rb->read, memory_order_relaxed) != ra)
        ;
    atomic_store_explicit(&rb->read, ra + 1, memory_order_release);
}
bool
ring_buffer_maybe_pop(struct ring_buffer* restrict rb, void* restrict out_item)
{
    rb_size_t const value_size = rb->value_size;
    rb_size_t const cap_mask = rb->capacity - 1u;
    char const* const data = rb->data;

    rb_size_t ra = atomic_load(&rb->read_ahead);

    /* If the buffer is "empty", nothing to pop. */
    if (atomic_load(&rb->write) - ra == 0)
        return false;

    /* If there is someone reading ahead of us, we will have to spin on the
    write to READ, so just don't. */
    if (atomic_load(&rb->read) != ra)
        return false;

    /* If someone managed to steal our READ-AHEAD slot, we can't pop.
    Otherwise, take the READ-AHEAD slot, as it guaranteed that we will not block.
    */
    if (!atomic_compare_exchange_strong(&rb->read_ahead, &ra, ra + 1))
        return false;

    /* Alright, (READ == READ-AHEAD) && (READ-AHEAD != WRITE).
    We can finally read. */

    memcpy(out_item, data + (ra & cap_mask) * value_size, value_size);

    /* "Increment" READ. */
    /* release */
    atomic_store(&rb->read, ra + 1);

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
