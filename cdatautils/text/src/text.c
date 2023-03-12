#include <cdatautils/text.h>

#include <stdlib.h>
#include <string.h>

static int64_t
get_size(text_t t)
{
    uint64_t ptr = (uint64_t)t.data;
    if ((ptr & 0xFF00000000000000UL) == 0) {
        if (ptr) {
            return t.size;
        } else {
            /* NOTE:
                In case of an empty string.
            */
            return 0;
        }
    } else {
        if (t.chars[15] == 0)
            return (int64_t)strlen(t.chars);
        else
            return 16;
    }
}


text_t
text_from_utf8_z(char const* string_z)
{
    return text_from_utf8(string_z, (int64_t)strlen(string_z));
}
text_t
text_from_utf8(char const* string, int64_t size)
{
    text_t text = { .data = 0, .size = size };

    if (!size) {
        return text;
    }

    text.data = malloc((size_t)size);
    memcpy(text.data, string, (size_t)size);

    return text;
}
char*
text_to_utf8_z(text_t text)
{
    size_t size = (size_t)text.size;
    char* bytes = calloc(size + 1, 1);
    memcpy(bytes, text.data, size);
    return bytes;
}


void
text_destroy(text_t* text)
{
    free(text->data);
    text->size = 0;
    text->data = 0;
}
