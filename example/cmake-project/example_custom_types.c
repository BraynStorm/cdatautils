#include <cvector.h>

#include <stdio.h>

struct vec3f
{
    float x, y, z;
};

GENERATE_VECTOR_GETTERS(struct vec3f, vec3f);

int
main()
{
    struct vector v = vector_create(struct vec3f);
    vector_push(&v, &(struct vec3f){ .x = 1.0f, .y = 2.0f, .z = 3.14f });

    struct vec3f v0 = vector_get_vec3f(&v, 0);
    printf("x=%f, y=%f, z=%f\n", v0.x, v0.y, v0.z);

    struct vec3f* pv0 = vector_ref_vec3f(&v, 0);
    printf("x=%f, y=%f, z=%f\n", pv0->x, pv0->y, pv0->z);

    if (pv0 == v.data)
        puts("No copies, just pointers!");

    vector_destroy(&v);
    return 0;
}