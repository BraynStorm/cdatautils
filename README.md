# cdatautils - collection of C data structures.

cdatautils/vector.h - a simple vector implementation in C.

TODO: extend this document


# Example usage:
```c
#include <cdatautils/cvector.h>

int
main()
{
    struct vector v = vector_create(int);

    vector_push(&v, &(int){ 10 });
    vector_push(&v, &(int){ 20 });
    vector_push(&v, &(int){ 30 });

    int ints[] = { 1, 2, 3, 4, 5 };
    vector_push_array(&v, ints, sizeof(ints) / sizeof(ints[0]));

    for (int i = 0; i < v.size; ++i) {
        printf("%d\n", vector_get_int(&v, i));
    }

    /* Outputs:
       10
       20
       30
       1
       2
       3
       4
       5
    */

    /* Cleanup (not necessary, since main will exit on the next line) */
    vector_destroy(&v);
    return 0;
}
```