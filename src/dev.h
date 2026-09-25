#ifndef JDAW_DEV_H
#define JDAW_DEV_H

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
  #define DEPRECATED [[deprecated]]
#elif defined(__GNUC__) || defined(__clang__)
  #define DEPRECATED __attribute__((deprecated))
#else
  #define DEPRECATED
#endif

#endif
