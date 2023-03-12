#include <catch2/catch_test_macros.hpp>

#include <cdatautils/text.h>

#include <string>

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

/* Necessary wrapper so that Catch2 correctly calls vector_destroy() on each created
`vector`.
*/
struct text_wrapper : text
{
    text_wrapper(text const& other)
      : text()
    {
        data = other.data;
        size = other.size;
    }
    ~text_wrapper() { text_destroy(this); }
};

TEST_CASE("conversions")
{
    char const* original = "asdfASDF";
    text_wrapper t = text_from_utf8_z(original);

    REQUIRE(t.size == 8);
    REQUIRE(memcmp(t.data, original, t.size) == 0);

    char* t2 = text_to_utf8_z(t);
    REQUIRE(strcmp(t2, original) == 0);
    free(t2);
}
TEST_CASE("conversions - empty string")
{
    char const* original = "";
    text_wrapper t = text_from_utf8_z(original);

    REQUIRE(t.size == 0);
    REQUIRE(t.data == 0);

    char* t2 = text_to_utf8_z(t);
    REQUIRE(t2 != nullptr);
    REQUIRE(strcmp(t2, original) == 0);
    free(t2);
}