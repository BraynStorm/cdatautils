/* benchmark tries to declspec(__dllimport) its methods, even though it is a static
 * library.
 */
#define BENCHMARK_EXPORT
#include <benchmark/benchmark.h>

#include <random>

#include <cdatautils/ringbuffer.h>


void
bm_ring_buffer_gotos(benchmark::State& state)
{
    int capacity = state.range(0);
    std::mt19937 mt(state.range(1));
    std::uniform_int_distribution<unsigned> dist(0, capacity - 1);

    int head = 0;
    int tail = capacity;
    unsigned* data = new unsigned[capacity]{ 0 };

    for (int i = 0; i < capacity; ++i) {
        data[i] = dist(mt);
    }

    for (auto _ : state) {
        unsigned sum = 0;

        head = dist(mt);
        tail = dist(mt);

        {
            int _old_tail = tail;
            int _tail = -1;

        loop:;
            for (; head < tail; ++head) {
                sum += data[head];
            }

            if (head > tail) {
                _tail = tail;
                tail = capacity;
                goto loop;
            } else if (_tail == _old_tail) {
                head = 0;
                tail = _tail;
                _tail = -1;
                goto loop;
            }
        }

        data[0] = sum;
        benchmark::DoNotOptimize(sum);
        benchmark::DoNotOptimize(data);
    }

    delete[] data;
}
void
bm_ring_buffer_two_loops(benchmark::State& state)
{
    int capacity = state.range(0);
    std::mt19937 mt(state.range(1));
    std::uniform_int_distribution<unsigned> dist(0, capacity - 1);
    int head = 0;
    int tail = 0;
    unsigned* data = new unsigned[capacity]{ 0 };

    for (int i = 0; i < capacity; ++i) {
        data[i] = dist(mt);
    }

    for (auto _ : state) {
        unsigned sum = 0;

        head = dist(mt);
        tail = dist(mt);

        if (head > tail) {
            for (; head < capacity; ++head) {
                sum += data[head];
            }
            head = 0;
        }
        for (; head < tail; ++head) {
            sum += data[head];
        }
        head = tail;


        data[0] = sum;
        benchmark::DoNotOptimize(sum);
        benchmark::DoNotOptimize(data);
    }

    delete[] data;
}

void
bm_ring_buffer_rotate(benchmark::State& state)
{
    int capacity = state.range(0);
    std::mt19937 mt(state.range(1));
    std::uniform_int_distribution<unsigned> dist(0, capacity - 1);
    int head = 0;
    int tail = 0;
    unsigned* data = new unsigned[capacity]{ 0 };

    for (int i = 0; i < capacity; ++i) {
        data[i] = dist(mt);
    }

    for (auto _ : state) {
        unsigned sum = 0;

        head = dist(mt);
        tail = dist(mt);

        {
            if (head > tail) {
                std::rotate(data, data + tail, data + capacity);
            }
            for (; head < tail; ++head) {
                sum += data[head];
            }
        }

        data[0] = sum;
        benchmark::DoNotOptimize(sum);
        benchmark::DoNotOptimize(data);
    }

    delete[] data;
}

void
bm_ring_buffer_scratch_space(benchmark::State& state)
{
    int capacity = state.range(0);
    std::mt19937 mt(state.range(1));
    std::uniform_int_distribution<unsigned> dist(0, capacity - 1);
    int head = 0;
    int tail = 0;
    unsigned* data = new unsigned[capacity]{ 0 };
    unsigned* tmp = new unsigned[capacity]{ 0 };

    for (int i = 0; i < capacity; ++i) {
        data[i] = dist(mt);
    }

    for (auto _ : state) {
        unsigned sum = 0;

        head = dist(mt);
        tail = dist(mt);

        {
            int size;
            if (head > tail) {
                memcpy(tmp, data + head, (capacity - head) * sizeof(data[0]));
                memcpy(tmp + capacity - head, data, tail * sizeof(data[0]));
                size = capacity - head + tail;
            } else {
                size = tail - head;
            }
            for (int i = 0; i < size; ++i) {
                sum += tmp[i];
            }
            head = tail;
        }

        data[0] = sum;
        benchmark::DoNotOptimize(sum);
        benchmark::DoNotOptimize(data);
    }

    delete[] tmp;
    delete[] data;
}

#define ARGS                    \
    ArgsProduct({               \
        /* ring buffer sizes */ \
        {                       \
            1u << 8u,           \
            1u << 10u,          \
            1u << 12u,          \
            1u << 16u,          \
            1u << 20u,          \
            1u << 24u,          \
        }, /* seeds */          \
        {                       \
            0x1337,             \
            0xdeadbeef,         \
        },                      \
    });                         \
    /*  */


// BENCHMARK(bm_ring_buffer_scratch_space)->ARGS;
// BENCHMARK(bm_ring_buffer_rotate)->ARGS;
// BENCHMARK(bm_ring_buffer_gotos)->ARGS;
// BENCHMARK(bm_ring_buffer_two_loops)->ARGS;

void
bm_ring_buffer_single_thread_deadlock(benchmark::State& state)
{
    using uint = unsigned int;

    struct ring_buffer* rb;
    ring_buffer_init(&rb, state.range(0), sizeof(uint));

    uint write_item = 0;
    for (auto _ : state) {
        for (int i = 0; i < state.range(1); ++i) {
            ring_buffer_deadlock_push(rb, &write_item);
            ++write_item;

            uint read_item = 0;
            ring_buffer_deadlock_pop(rb, &read_item);
            benchmark::DoNotOptimize(read_item);
        }
        state.SetBytesProcessed(
            state.bytes_processed() + state.range(1) * sizeof(uint)
        );
    }
}
void
bm_ring_buffer_single_thread(benchmark::State& state)
{
    using uint = unsigned int;

    struct ring_buffer* rb;
    ring_buffer_init(&rb, state.range(0), sizeof(uint));

    uint write_item = 0;
    for (auto _ : state) {
        for (int i = 0; i < state.range(1); ++i) {
            ring_buffer_push(rb, &write_item);
            ++write_item;

            uint read_item = 0;
            ring_buffer_pop(rb, &read_item);

            benchmark::DoNotOptimize(read_item);
        }
        state.SetBytesProcessed(
            state.bytes_processed() + state.range(1) * sizeof(uint)
        );
    }
}
void
bm_ring_buffer_single_thread_maybe(benchmark::State& state)
{
    using uint = unsigned int;

    struct ring_buffer* rb;
    ring_buffer_init(&rb, state.range(0), sizeof(uint));

    uint write_item = 0;
    for (auto _ : state) {
        for (int i = 0; i < state.range(1); ++i) {
            ring_buffer_maybe_push(rb, &write_item);
            ++write_item;

            uint read_item = 0;
            ring_buffer_maybe_pop(rb, &read_item);
            benchmark::DoNotOptimize(read_item);
        }
        state.SetBytesProcessed(
            state.bytes_processed() + state.range(1) * sizeof(uint)
        );
    }
}
void
bm_ring_buffer_multithread(benchmark::State& state)
{
    static struct ring_buffer* rb;
    if (state.thread_index() == 0) {
        ring_buffer_init(&rb, state.range(0), sizeof(uint64_t));
    }

    if (state.thread_index() % 2) {
        // Reader.

        uint64_t value;
        for (auto _ : state) {
            for (int i = 0; i < 128; ++i)
                ring_buffer_pop(rb, &value);
        }
        benchmark::DoNotOptimize(rb);
        benchmark::DoNotOptimize(value);
    } else {
        // Writer.

        uint64_t value = 1;
        for (auto _ : state) {
            for (int i = 0; i < 128; ++i) {
                ring_buffer_push(rb, &value);
                ++value;
            }
        }
        benchmark::DoNotOptimize(rb);
        benchmark::DoNotOptimize(value);
    }

    if (state.thread_index() == 0) {
        ring_buffer_destroy(rb);
    }
}
void
bm_ring_buffer_multithread_maybe(benchmark::State& state)
{
    static struct ring_buffer* rb;
    if (state.thread_index() == 0) {
        ring_buffer_init(&rb, state.range(), sizeof(uint64_t));
    }

    if (state.thread_index() % 2) {
        // Reader.

        uint64_t value;
        for (auto _ : state) {
            ring_buffer_maybe_pop(rb, &value);
        }
        benchmark::DoNotOptimize(value);
    } else {
        // Writer.

        uint64_t value = 1;
        for (auto _ : state) {
            ring_buffer_maybe_push(rb, &value);
            ++value;
        }
    }

    if (state.thread_index() == 0) {
        ring_buffer_destroy(rb);
    }
}


void
decorate_ring_buffer_single_thread(benchmark::internal::Benchmark* bm)
{
    bm->ComputeStatistics(
          "max",
          [](std::vector<double> const& v) -> double {
              return *(std::max_element(std::begin(v), std::end(v)));
          },
          benchmark::StatisticUnit::kTime
    )
        ->ArgsProduct({
            { 64 },
            { 16, 64 },
        })
        ->MinTime(2);
    // ->Iterations(1000000);
}
void
decorate_ring_buffer_multi_thread(benchmark::internal::Benchmark* bm)
{
    bm->ArgsProduct({
                        { 64 },
                        { 32, 64 },
                    })
        ->Iterations(100000);
}
BENCHMARK(bm_ring_buffer_single_thread_deadlock)
    ->Apply(decorate_ring_buffer_single_thread);
BENCHMARK(bm_ring_buffer_single_thread)->Apply(decorate_ring_buffer_single_thread);
BENCHMARK(bm_ring_buffer_single_thread_maybe)
    ->Apply(decorate_ring_buffer_single_thread);

BENCHMARK_MAIN();