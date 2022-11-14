#include <catch2/catch_test_macros.hpp>

#include <cdatautils/ringbuffer.h>

#include <string>

/* Necessary wrappers so that Catch2 correctly calls destructors.
 */

struct ring_buffer_deleter
{
    void
    operator()(ring_buffer* rb)
    {
        ring_buffer_destroy(rb);
    }
};
template<class T>
struct ring_buffer_wrapper
{
    ring_buffer_wrapper(rb_size_t capacity)
    {
        ring_buffer* rb = 0;
        ring_buffer_init(&rb, capacity, sizeof(T));
        _rb.reset(rb);
    }

    void
    push(T const& item)
    {
        ring_buffer_push(_rb.get(), &item);
    }

    T
    pop()
    {
        T item;
        ring_buffer_pop(_rb.get(), &item);
        return item;
    }

    bool
    maybe_pop(T& out_item)
    {
        return ring_buffer_maybe_pop(_rb.get(), &out_item);
    }

    bool
    maybe_push(T const& item)
    {
        return ring_buffer_maybe_push(_rb.get(), &item);
    }

private:
    std::unique_ptr<ring_buffer, ring_buffer_deleter> _rb;
};

TEST_CASE("ring buffer initial state is empty", "[ring_buffer]")
{
    ring_buffer_wrapper<int> rb(8);

    GIVEN("just-initialized buffer of 8")
    {
        WHEN("the buffer is empty")
        {
            int dummy;
            REQUIRE(rb.maybe_pop(dummy) == false);
        }
        WHEN("8 items are pushed")
        {
            for (int i = 0; i < 8; ++i)
                REQUIRE(rb.maybe_push(i) == true);

            THEN("a 9th item cannot be pushed")
            {
                REQUIRE(rb.maybe_push(9) == false);
            }
            AND_THEN("8 items can be read, in the same order")
            {
                for (int i = 0; i < 8; ++i) {
                    int tmp;
                    REQUIRE(rb.maybe_pop(tmp) == true);
                    REQUIRE(tmp == i);
                }
                AND_THEN("a 9th item cannot be read")
                {
                    int tmp;
                    REQUIRE(rb.maybe_pop(tmp) == false);
                }
            }
        }
    }
}