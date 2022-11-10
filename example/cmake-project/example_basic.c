#include <cvector.h>

#include <assert.h>


int
main()
{
    struct vector v = vector_create(int);
    // alternative:
    // vector_init(&v, sizeof(int));

    assert(v.size == 0);
    assert(v.capacity == 0);
    assert(v.data == 0);
    assert(v.value_size == sizeof(int));

    /* Pushing values one-by-one. */
    vector_push(&v, &(int){ 5 });
    assert(v.size == 1);

    /* Pushing whole arrays. */
    int ints[] = { 1, 2, 3, 4, 5, 6, 7 };
    vector_push_array(&v, 7, ints);

    /* Reading data. */
    {
        /* By value. */
        assert(vector_get_int(&v, 0) == 5);

        /* By pointer. */
        assert(*vector_ref_int(&v, 0) == 5);

        /* Maximally generic. */
        assert(*(int*)vector_get(&v, 0) == 5);
    }

    /* Remove the last element. */
    v.size -= 1;
    // alternative:
    // vector_remove(&v, v.size - 1);

    // Remove 2 items.
    vector_remove_range(&v, 1, 3);

    /* Cleanup after ourselves. */
    vector_destroy(&v);

    return (uintptr_t)v.data | v.size | v.capacity | v.value_size; // HINT: 0
}