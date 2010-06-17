#ifndef __PTHREAD_EX_H__
#define __PTHREAD_EX_H__

#include <pthread.h>
#if 0
int pthread_init_ex   (void);
int pthread_create_ex (pthread_t *, const pthread_attr_t *,
                                 void *(*)(void *), void *);

#ifndef PTHREAD_EX_INTERNAL
#define pthread_init   pthread_init_ex
#define pthread_create pthread_create_ex
#endif
#endif

void ex_thread_init(void *arg);

#endif /* __PTHREAD_EX_H__ */
