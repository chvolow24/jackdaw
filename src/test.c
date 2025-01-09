#include <stdio.h>
#include <stdlib.h>

void breakfn()
{
    
}

void jfree(void *obj)
{
    fprintf(stderr, "freeing %p\n", obj);
    free(obj);
}

void *jmalloc(size_t size)
{
    fprintf(stderr, "Allocing %lu bytes\n", size);
    return malloc(size);
}

void *jcalloc(size_t count, size_t size)
{
    fprintf(stderr, "Callocing %lu bytes\n", size * count);
    return calloc(count, size);
}
