#include <stdlib.h>
#include <pthread.h>

#define PTHREAD_EX_INTERNAL
#include "pthread_ex.h"
#include "ex.h"

/* context storage key */
static pthread_key_t pthread_ex_ctx_key;

/* context destructor */
static void pthread_ex_ctx_destroy(void *data)
{
	if (data != NULL)
		free(data);
	return;
}

/* callback: context fetching */
static ex_ctx_t *pthread_ex_ctx(void)
{
	return (ex_ctx_t *)
		pthread_getspecific(pthread_ex_ctx_key);
}

/* callback: termination */
static void pthread_ex_terminate(ex_t *e)
{
	pthread_exit(e->ex_value);
}
/* pthread init */
int pthread_init_ex(void)
{
	int rc;

	/* additionally create thread data key
		and override OSSP ex callbacks */
	pthread_key_create(&pthread_ex_ctx_key,
				pthread_ex_ctx_destroy);
	__ex_ctx       = pthread_ex_ctx;
	__ex_terminate = pthread_ex_terminate;

return rc;
}

/* internal thread entry wrapper information */
typedef struct {
	void *(*entry)(void *);
	void *arg;
} pthread_create_ex_t;

#if 0
/* internal thread entry wrapper */
static void *pthread_create_wrapper(void *arg)
{
	pthread_create_ex_t *wrapper;
	ex_ctx_t *ex_ctx;

	printf("in wrapper\n");
	/* create per-thread exception context */
	wrapper = (pthread_create_ex_t *)arg;
	ex_ctx = (ex_ctx_t *)malloc(sizeof(ex_ctx_t));
	EX_CTX_INITIALIZE(ex_ctx);
	pthread_setspecific(pthread_ex_ctx_key, ex_ctx);

	/* perform original operation */
	printf("Call original func\n");
	return wrapper->entry(wrapper->arg);
}

/* pthread_create() wrapper */
int pthread_create_ex(pthread_t *thread,
		const pthread_attr_t *attr,
		void *(*entry)(void *), void *arg)
{
	pthread_create_ex_t wrapper;

	/* spawn thread but execute start
		function through wrapper */
	wrapper.entry = entry;
	wrapper.arg   = arg;
	printf("creating thread\n");
	return pthread_create(thread, attr,
				pthread_create_wrapper, &wrapper);
}
#else
/* internal thread entry wrapper */
static void *pthread_create_wrapper(void *arg) {
	pthread_create_ex_t wrapper;
	ex_ctx_t *ex_ctx;
	
	/* create per-thread exception context */
	wrapper.entry = ((pthread_create_ex_t *)arg)->entry;
	wrapper.arg = ((pthread_create_ex_t *)arg)->arg;
	free (arg);
	
	ex_ctx = (ex_ctx_t *) calloc(1, sizeof(ex_ctx_t));
	EX_CTX_INITIALIZE(ex_ctx);
	pthread_setspecific(pthread_ex_ctx_key, ex_ctx);
	
	/* perform original operation */
	return wrapper.entry(wrapper.arg);
}

/* pthread_create() wrapper */
int
pthread_create_ex(pthread_t * thread,
		  const pthread_attr_t * attr,
		  void *(*entry) (void *), void *arg) {
	pthread_create_ex_t *wrapper = calloc(1, sizeof(pthread_create_ex_t));
	/* spawn thread but execute start
	function through wrapper */
	wrapper->entry = entry;
	wrapper->arg = arg;
	return pthread_create(thread, attr, pthread_create_wrapper,
			      wrapper);
}
#endif