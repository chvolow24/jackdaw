#ifndef JDAW_ATOMIC_H
#define JDAW_ATOMIC_H

#include <stdatomic.h>


#define astrr(dst, new) atomic_store_explicit(dst, new, memory_order_relaxed)
#define aldr(dst) atomic_load_explicit(dst, memory_order_relaxed)


#endif
