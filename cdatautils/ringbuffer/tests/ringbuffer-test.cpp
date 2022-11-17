#include <catch2/catch_test_macros.hpp>

#include <cdatautils/ringbuffer.h>

#include <string>
#include <optional>
#include <atomic>
#include <thread>

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

    bool
    push(T const& item)
    {
        return ring_buffer_push(_rb.get(), &item);
    }

    std::optional<T>
    pop()
    {
        T item;
        if (ring_buffer_pop(_rb.get(), &item))
            return item;
        else
            return {};
    }

    void
    push_deadlock(T const& item)
    {
        ring_buffer_deadlock_push(_rb.get(), &item);
    }

    T
    pop_deadlock()
    {
        T item;
        ring_buffer_deadlock_pop(_rb.get(), &item);
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

    rb_size_t
    size() const
    {
        return ring_buffer_size(_rb.get());
    }
    rb_size_t
    capacity() const
    {
        return ring_buffer_capacity(_rb.get());
    }
    rb_size_t
    value_size() const
    {
        return ring_buffer_value_size(_rb.get());
    }
    void
    clear() const
    {
        return ring_buffer_clear(_rb.get());
    }

private:
    std::unique_ptr<ring_buffer, ring_buffer_deleter> _rb;
};

TEST_CASE("ring buffer", "[ring_buffer]")
{
    ring_buffer_wrapper<int> rb(8);

    GIVEN("just-initialized buffer of 8")
    {
        REQUIRE(rb.value_size() == sizeof(int));
        REQUIRE(rb.capacity() == 8);

        THEN("the buffer is empty")
        {
            int dummy;
            REQUIRE(rb.size() == 0);
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
            THEN("the size is 8")
            {
                REQUIRE(rb.size() == 8);
            }
            THEN("8 items can be pop-ed, in the same order")
            {
                for (int i = 0; i < 8; ++i) {
                    REQUIRE(rb.pop() == i);
                }
                AND_THEN("a 9th item cannot be maybe_pop-ed")
                {
                    int tmp;
                    REQUIRE(rb.size() == 0);
                    REQUIRE(rb.maybe_pop(tmp) == false);
                }
            }
            THEN("8 items can be maybe_pop-ed, in the same order")
            {
                for (int i = 0; i < 8; ++i) {
                    int tmp;
                    REQUIRE(rb.maybe_pop(tmp) == true);
                    REQUIRE(tmp == i);
                }
                AND_THEN("a 9th item be maybe_pop-ed")
                {
                    int tmp;
                    REQUIRE(rb.maybe_pop(tmp) == false);
                }
            }
        }
    }
}


TEST_CASE("ring buffer SPSC", "[ring_buffer][threads]")
{
    ring_buffer_wrapper<int> rb(16);
    std::atomic_int counter = 0;

    auto producer = [&rb, &counter]() {
        for (int i = 0; i < 200'000; ++i) {
            int c = counter++;
            rb.push_deadlock(c);
        }
    };

    std::thread producer_0(producer);

    int* array = new int[200'000]{ 0 };

    for (int i = 0; i < 200'000; ++i) {
        int index = rb.pop_deadlock();
        assert(index >= 0);
        assert(index < 200'000);
        ++array[index];
    }
    producer_0.join();
    for (int i = 0; i < 200'000; ++i) {
        assert(array[i] == 1);
    }

    delete[] array;
}
TEST_CASE("ring buffer MPSC", "[ring_buffer][threads]")
{
    ring_buffer_wrapper<int> rb(16);
    std::atomic_int counter = 0;

    auto producer = [&rb, &counter]() {
        for (int i = 0; i < 10000; ++i) {
            int c = counter++;
            rb.push_deadlock(c);
        }
    };

    constexpr int c = 24;
    std::thread producers[c];

    for (int i = 0; i < c; ++i) {
        producers[i] = std::thread(producer);
    }
    int* array = new int[c * 10'000]{ 0 };

    for (int i = 0; i < c * 10'000; ++i) {
        int index = rb.pop_deadlock();
        if (index < 0 || index >= c * 10'000) {
            REQUIRE(index >= 0);
            REQUIRE(index < c * 10'000);
        }
        ++array[index];
    }
    for (int i = 0; i < c * 10'000; ++i) {
        if (array[i] != 1) {
            REQUIRE(array[i] == 1);
        }
    }
    for (int i = 0; i < c; ++i) {
        producers[i].join();
    }

    delete[] array;
}