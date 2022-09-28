#include <catch2/catch_test_macros.hpp>

#include "cvector.h"

#include <string>

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

/* Necessary wrapper so that Catch2 correctly calls vector_destroy() on each created
`vector`.
*/
struct vector_wrapper : vector
{
    ~vector_wrapper() { vector_destroy(this); }
};

TEST_CASE("vector's data is always aligned on a 16 byte boundary", "[vector]")
{
    for (int i = 0; i < 10000; ++i) {
        vector_wrapper v = {};
        vector_init(&v, rand() % 256);
        vector_reserve(&v, 1);

        uintptr_t dataptr = (uintptr_t)v.data;
        if (dataptr % 16) {
            // Reduce amount of assertions reported.
            REQUIRE((dataptr % 16) == 0);
        }
    }
}

TEST_CASE("a new vector has zero size, capacity and no data")
{
    vector_wrapper v = vector_create(int);
    REQUIRE(v.size == 0);
    REQUIRE(v.capacity == 0);
    REQUIRE(v.data == nullptr);
}

TEST_CASE("vector basics", "[vector]")
{
    GIVEN("a just-initialized vector")
    {
        vector_wrapper v = {};
        vector_init(&v, sizeof(int));
        int const old_value_size = v.value_size;

        WHEN("the vector is cleared")
        {
            vector_clear(&v);

            THEN("size remains 0")
            {
                REQUIRE(v.size == 0);
            }
            THEN("capacity remains 0")
            {
                REQUIRE(v.capacity == 0);
            }
            THEN("value_size is unchanged")
            {
                REQUIRE(v.value_size == old_value_size);
            }
            THEN("data is NULL")
            {
                REQUIRE(v.data == nullptr);
            }
        }

        WHEN("an item is pushed")
        {
            int item = 10;
            vector_push(&v, &item);

            THEN("size is 1")
            {
                REQUIRE(v.size == 1);
            }
            THEN("capacity is at least 1")
            {
                REQUIRE(v.capacity >= 1);
            }
            THEN("data is not NULL")
            {
                REQUIRE(v.data != nullptr);
            }
            THEN("value_size is unchanged")
            {
                REQUIRE(v.value_size == old_value_size);
            }
        }
        WHEN("3 items are pushed")
        {
            int item1 = 10;
            int item2 = 11;
            int item3 = 16;
            vector_push(&v, &item1);
            vector_push(&v, &item2);
            vector_push(&v, &item3);

            void* old_data = v.data;
            int old_capacity = v.capacity;

            THEN("size is 3")
            {
                REQUIRE(v.size == 3);
            }
            THEN("capacity is at least 3")
            {
                REQUIRE(v.capacity >= 3);
            }
            THEN("value_size is unchanged")
            {
                REQUIRE(v.value_size == old_value_size);
            }
            THEN("data is not NULL")
            {
                REQUIRE(v.data != nullptr);
            }
            THEN("the items appear in insertion order")
            {
                REQUIRE(vector_get_i32(&v, 0) == item1);
                REQUIRE(vector_get_i32(&v, 1) == item2);
                REQUIRE(vector_get_i32(&v, 2) == item3);
            }
            AND_WHEN("an item is inserted in the middle")
            {
                int itemMiddle = 33;
                vector_insert(&v, 1, &itemMiddle);

                THEN("the items appear in the inserted position")
                {
                    REQUIRE(vector_get_i32(&v, 1) == itemMiddle);
                }
                AND_THEN("the previous items remain unchanged")
                {
                    REQUIRE(vector_get_i32(&v, 0) == item1);
                }
                AND_THEN("the following items are moved one index over")
                {
                    REQUIRE(vector_get_i32(&v, 2) == item2);
                    REQUIRE(vector_get_i32(&v, 3) == item3);
                }
            }
            AND_WHEN("an item is inserted in the beginning")
            {
                int itemStart = 4;
                vector_insert(&v, 0, &itemStart);

                THEN("the items appear in the inserted position")
                {
                    REQUIRE(vector_get_i32(&v, 0) == itemStart);
                }
                AND_THEN("the items are moved one index over")
                {
                    REQUIRE(vector_get_i32(&v, 1) == item1);
                    REQUIRE(vector_get_i32(&v, 2) == item2);
                    REQUIRE(vector_get_i32(&v, 3) == item3);
                }
            }
            AND_WHEN("an item is inserted at index one-past-the-end (size)")
            {
                REQUIRE(v.size == 3);

                int itemEnd = 64;
                vector_insert(&v, 3, &itemEnd);

                THEN("the items appear in the inserted position")
                {
                    REQUIRE(vector_get_i32(&v, 3) == itemEnd);
                }
                AND_THEN("all items remain unchanged")
                {
                    REQUIRE(vector_get_i32(&v, 0) == item1);
                    REQUIRE(vector_get_i32(&v, 1) == item2);
                    REQUIRE(vector_get_i32(&v, 2) == item3);
                }
            }
            AND_WHEN("an item is inserted at index 'size'")
            {
                int itemEnd = 64;
                vector_insert(&v, 3, &itemEnd);

                THEN("the items appear in the inserted position")
                {
                    REQUIRE(vector_get_i32(&v, 3) == itemEnd);
                }
                AND_THEN("all items remain unchanged")
                {
                    REQUIRE(vector_get_i32(&v, 0) == item1);
                    REQUIRE(vector_get_i32(&v, 1) == item2);
                    REQUIRE(vector_get_i32(&v, 2) == item3);
                }
            }
            AND_WHEN("the vector is cleared")
            {
                vector_clear(&v);

                THEN("size is 0")
                {
                    REQUIRE(v.size == 0);
                }
                THEN("capacity remains the same")
                {
                    REQUIRE(v.capacity == old_capacity);
                }
                THEN("value_size is unchanged")
                {
                    REQUIRE(v.value_size == old_value_size);
                }
                THEN("data is not NULL")
                {
                    REQUIRE(v.data == old_data);
                }
            }
        }
    }
}

TEST_CASE("vector insertions")
{
    vector_wrapper v = vector_create(int);
    GIVEN("an empty vector")
    {
        WHEN("an array is inserted ata index 0")
        {
            int const items[] = { 6, 5, 4, 3, 2, 1 };
            int const n_items = ARRAY_COUNT(items);
            vector_insert_array(&v, 0, n_items, items);

            THEN("the size of the vector matches the size of the array")
            {
                REQUIRE(v.size == n_items);
            }
            THEN("the items of the vector match exactly the array's contents")
            {
                bool equal = 0 == memcmp(items, v.data, v.size * v.value_size);
                REQUIRE(equal == true);
            }
        }
    }

    GIVEN("a vector of 8 elements")
    {
        vector_reserve(&v, 8);
        int initial_items[] = { 100, 10, 20, 30, 40, 50, 60, 70 };
        for (int i = 0; i < v.capacity; ++i)
            vector_push(&v, &initial_items[i]);

        int const old_size = v.size;

        int const items[] = { 7, 6, 5, 4, 3, 2, 1, 55 };
        int const n_items = ARRAY_COUNT(items);

        WHEN("an array is inserted data index 0 (start)")
        {
            vector_insert_array(&v, 0, n_items, items);
            THEN("the size of the vector increases")
            {
                REQUIRE(v.size == n_items + old_size);
            }
            THEN("the start of the array matches the inserted items")
            {
                for (int i = 0; i < n_items; ++i) {
                    REQUIRE(vector_get_i32(&v, i) == items[i]);
                }
            }
            AND_THEN("the rest of the array contains the original items")
            {
                for (int i = 0; i < old_size; ++i) {
                    REQUIRE(vector_get_i32(&v, n_items + i) == initial_items[i]);
                }
            }
        }
        WHEN("an array is inserted data index 3 (middle)")
        {
            vector_insert_array(&v, 3, n_items, items);
            THEN("the size of the vector increases")
            {
                REQUIRE(v.size == n_items + old_size);
            }
            THEN("the items in index range [0, 3) are the original items")
            {
                REQUIRE(vector_get_i32(&v, 0) == initial_items[0]);
                REQUIRE(vector_get_i32(&v, 1) == initial_items[1]);
                REQUIRE(vector_get_i32(&v, 2) == initial_items[2]);
            }
            AND_THEN("the items in index range [3, 11) are the inserted items")
            {
                for (int i = 0; i < n_items; ++i) {
                    REQUIRE(vector_get_i32(&v, i + 3) == items[i]);
                }
            }
            AND_THEN("the items in index range [11, 16) are the original items")
            {
                for (int i = 0; i < 5; ++i) {
                    REQUIRE(vector_get_i32(&v, i + 11) == initial_items[i + 3]);
                }
            }
        }
        WHEN("an array is inserted data index 7 (end)")
        {
            vector_insert_array(&v, 8, n_items, items);
            THEN("the size of the vector increases")
            {
                REQUIRE(v.size == n_items + old_size);
            }
            THEN("the start of the array matches the original items")
            {
                for (int i = 0; i < old_size; ++i) {
                    REQUIRE(vector_get_i32(&v, i) == initial_items[i]);
                }
            }
            AND_THEN("the rest of the array contains the inserted items")
            {
                for (int i = 0; i < n_items; ++i) {
                    REQUIRE(vector_get_i32(&v, i + old_size) == items[i]);
                }
            }
        }
    }
}


TEST_CASE("a vector with spare capacity doesn't allocate")
{
    vector_wrapper v = {};
    vector_init(&v, sizeof(int));

    GIVEN("a vector with spare capacity")
    {
        vector_reserve(&v, 1);
        void* old_data = v.data;
        int old_cap = v.capacity;
        int old_value_size = v.value_size;

        WHEN("an item is pushed")
        {
            int item = 33;
            vector_push(&v, &item);

            THEN("capacity is unchanged")
            {
                REQUIRE(v.capacity == old_cap);
            }
            THEN("data is unchanged")
            {
                REQUIRE(v.data == old_data);
            }
        }
    }
}

TEST_CASE("a vector grows to exact capacity when reserved")
{
    vector_wrapper v = {};
    vector_init(&v, sizeof(int));

    GIVEN("an empty vector")
    {
        WHEN("items are reserved")
        {
            vector_reserve(&v, 1);
            THEN("capacity matches the reserved size")
            {
                REQUIRE(v.capacity == 1);
            }
        }
    }

    GIVEN("a full vector")
    {
        vector_reserve(&v, 4);

        int item = 423;
        vector_push(&v, &item);
        vector_push(&v, &item);
        vector_push(&v, &item);
        vector_push(&v, &item);

        WHEN("items are reserved")
        {
            vector_reserve(&v, 5);
            THEN("capacity matches the reserved size")
            {
                REQUIRE(v.capacity == 5);
            }
        }
    }
}

TEST_CASE("vector_push_sprintf")
{
#define vector_to_string(v) ::std::string((char const*)v.data, v.size)

    struct vector_wrapper v = {};
    vector_init(&v, sizeof(char));
    vector_reserve(&v, 128);

    GIVEN("empty vector")
    {
        WHEN("a format string with no replacements is pushed")
        {
            vector_push_sprintf(&v, "random text");

            THEN("the vector's content matches the literal format string")
            {
                REQUIRE(vector_to_string(v) == "random text");
            }
        }
        WHEN("fmt=%%")
        {
            vector_push_sprintf(&v, "abc%%");

            THEN("the vector's content matches the literal format string")
            {
                REQUIRE(vector_to_string(v) == "abc%");
            }
        }
        WHEN("fmt=%s")
        {
            char const* some_string = "asdf";
            vector_push_sprintf(&v, "12%s34", some_string);

            THEN("the vector's content is appended the formatted text")
            {
                REQUIRE(vector_to_string(v) == "12asdf34");
            }
        }
        WHEN("fmt=%i")
        {
            AND_WHEN("negative")
            {
                vector_push_sprintf(&v, "a%ib", 14);
                REQUIRE(vector_to_string(v) == "a14b");
            }

            AND_WHEN("negative")
            {
                vector_push_sprintf(&v, "a%ib", -14);
                REQUIRE(vector_to_string(v) == "a-14b");
            }
        }
        WHEN("fmt=%u")
        {
            vector_push_sprintf(&v, "a%ub", (uint32_t)3000000000);
            REQUIRE(vector_to_string(v) == "a3000000000b");
        }
        WHEN("fmt=%li")
        {
            vector_push_sprintf(&v, "a%lib", (int64_t)3000000000);
            REQUIRE(vector_to_string(v) == "a3000000000b");
        }
        WHEN("fmt=%lu")
        {
            // Slight less than UINT64_MAX
            vector_push_sprintf(&v, "a%lib", (uint64_t)9223372036854775800LLU);
            REQUIRE(vector_to_string(v) == "a9223372036854775800b");
        }
    }
#undef vector_to_string
}

TEST_CASE("vector with no spare capacity doubles its capacity")
{
    vector_wrapper v = {};
    vector_init(&v, sizeof(int));

    GIVEN("a vector with no spare capacity")
    {
        int item = 10;
        vector_reserve(&v, 10);
        REQUIRE(v.capacity == 10);
        for (int i = 0; i < 10; ++i)
            vector_push(&v, &item);
        REQUIRE(v.capacity == 10);

        THEN("the capacity doubles")
        {
            int old_cap = v.capacity;
            WHEN("a new element is inserted in the beginning")
            {
                vector_insert(&v, 0, &item);
                REQUIRE(v.capacity == old_cap * 2);
            }
            WHEN("a new element is inserted int the middle")
            {
                vector_insert(&v, 4, &item);
                REQUIRE(v.capacity == old_cap * 2);
            }
            WHEN("a new element is inserted at the end")
            {
                vector_insert(&v, v.size, &item);
                REQUIRE(v.capacity == old_cap * 2);
            }
        }
    }
}

TEST_CASE("modifying vector's value_size", "[!nonportable][vector]")
{
    GIVEN("a vector of two uint32s")
    {
        struct vector_wrapper v = {};
        vector_init(&v, sizeof(uint32_t));

        uint32_t int1 = 0xAABBCCDD;
        uint32_t int2 = 0xEEFF0011;
        vector_push(&v, &int1);
        vector_push(&v, &int2);

        REQUIRE(vector_get_int(&v, 0) == int1);
        REQUIRE(vector_get_int(&v, 1) == int2);

        WHEN("value_size is set to that of uint16_t")
        {
            v.value_size = sizeof(uint16_t);

            THEN("unicorns happen")
            {
                REQUIRE(vector_get_u16(&v, 0) == (uint16_t)int1);
                REQUIRE(vector_get_u16(&v, 1) == (int1 >> 16));
                REQUIRE(vector_get_u16(&v, 2) == (uint16_t)int2);
                REQUIRE(vector_get_u16(&v, 3) == (int2 >> 16));
            }
        }
    }
}