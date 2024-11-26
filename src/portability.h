#ifndef JDAW_PORTABILITY_H
#define JDAW_PORTABILITY_H

#if defined(HAVE_THREAD_LOCAL)
    #define JDAW_THREAD_LOCAL thread_local
#elsif defined(HAVE__THREAD_LOCAL)
    #define JDAW_THREAD_LOCAL _Thread_local
#elsif defined(HAVE___THREAD)
    #define JDAW_THREAD_LOCAL __thread
#else
    #define JDAW_THREAD_LOCAL
#endif

#endif
