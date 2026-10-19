#ifndef JDAW_DEV_H
#define JDAW_DEV_H

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
  #define DEPRECATED [[deprecated]]
#elif defined(__GNUC__) || defined(__clang__)
  #define DEPRECATED __attribute__((deprecated))
#else
  #define DEPRECATED
#endif

#if defined(__has_c_attribute)
#  if __has_c_attribute(nodiscard)
#    define NODISCARD [[nodiscard]]
#  endif
#endif
#ifndef NODISCARD
#  if defined(__GNUC__) || defined(__clang__)
#    define NODISCARD __attribute__((__warn_unused_result__))
#  else
#    define NODISCARD
#  endif
#endif


#endif
