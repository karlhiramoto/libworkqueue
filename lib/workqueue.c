/**
* @file Workqueue.c
* @author Karl Hiramoto <karl@hiramoto.org>
* @brief Library for priority work queues.
*
* Distributed under LGPL see COPYING.LGPL
* Copyright 2009 Karl Hiramoto
*/

// #define DEBUG

#include <stdlib.h>
#include <stdio.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)


#pragma once
#include "windows.h"            // big fat windows lib

#include <sys/timeb.h>
#include <time.h>
#include "win32/stdbool.h"

#if !defined(WINDOWS)
#define WINDOWS
#endif

#ifdef DEBUG
#define DEBUG_MSG(fmt, ...) { printf("%s:%d " fmt, __FUNCTION__, __LINE__, __VA_ARGS__); }
#else
#define DEBUG_MSG(fmt, ...)
#endif

#define assert(x)
#define ENOMEM ERROR_NOT_ENOUGH_MEMORY
#define EBUSY  ERROR_BUSY
#define ENOENT ERROR_NOT_FOUND

#define ERROR_MSG(fmt, ...) { printf("%s:%d " fmt, __FUNCTION__, __LINE__, __VA_ARGS__); }

#define GET_TIME(x) _ftime_s(&x)
#define TIME_STRUCT_TYPE _timeb

#define LOCK_MUTEX(mutex)	WaitForSingleObject(mutex,INFINITE)
#define TRY_LOCK_MUTEX(mutex, r)  r = WaitForSingleObject(mutex,0);
#define UNLOCK_MUTEX(x)	ReleaseMutex(x)
#define pthread_join(A,B) \
  ((WaitForSingleObject((A), INFINITE) != WAIT_OBJECT_0) || !CloseHandle(A))


/* if time X is > y*/
//#define TIME_GT(x,y) (x.time > y->time || (x.time == y->time && x.millitm > y->millitm) )
#define TIME_SEC(x) x.time
#define TIME_SECP(x) x->time
#define TIME_MSEC(x) x.millitm
#define TIME_MSECP(x) x->millitm



#else // on a unix/linux system
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <signal.h>

#ifdef DEBUG
#define DEBUG_MSG(fmt, s...) { printf("%s:%d " fmt, __FUNCTION__,__LINE__, ## s); }
#else
#define DEBUG_MSG(fmt, s...)
#endif
#define ERROR_MSG(fmt, s...) { fprintf(stderr, "ERROR %s:%d " fmt, __FUNCTION__,__LINE__, ## s); }

#define GET_TIME(x) clock_gettime(CLOCK_REALTIME, &x)
#define TIME_STRUCT_TYPE timespec
// #define LOCK_MUTEX(mutex)  pthread_mutex_lock(mutex)
// #define UNLOCK_MUTEX(x)	pthread_mutex_unlock(x)
#define LOCK_MUTEX(mutex)  { DEBUG_MSG("LOCKING " #mutex " get %s:%d\n" , __FUNCTION__,__LINE__); pthread_mutex_lock(mutex); DEBUG_MSG("LOCKED " #mutex " got %s:%d \n" , __FUNCTION__,__LINE__); }

#define TRY_LOCK_MUTEX(mutex, r)  { DEBUG_MSG("TRYLOCK " #mutex " get %s:%d\n" , __FUNCTION__,__LINE__); r = pthread_mutex_trylock(mutex); DEBUG_MSG ("TRYLOCK " #mutex " got %s:%d r=%d \n" , __FUNCTION__,__LINE__,r); }

#define UNLOCK_MUTEX(x)	 { DEBUG_MSG("UNLOCK " #x " %s:%d\n" , __FUNCTION__,__LINE__); pthread_mutex_unlock(x); }

#define TIME_SEC(x) x.tv_sec
#define TIME_SECP(x) x->tv_sec
#define TIME_MSEC(x) (x.tv_nsec / 1000000)
#define TIME_MSECP(x) (x->tv_nsec / 1000000)

#endif


#include "workqueue.h"



/**
* @struct workqueue_job
* @brief Every scheduled or running job has a instance of this struct
* @brief This data is private to the library
*/
struct workqueue_job
{
	int priority; /** Job priority.  Lower number is higher priority as on all UNIX OS's */
	unsigned int job_id; /** Job ID 1st job is 1 then goes up */
	struct TIME_STRUCT_TYPE start_time;   /** Time the job was started */
	workqueue_func_t func; /** Callback function pointer*/
	void * data; /** Data to pass to callback pointer */
};

/**
* @struct workqueue_thread
* @brief Every worker thread has an instance of this struct.
* @brief This data is private to the library.
*/
struct workqueue_thread
{

#ifdef WINDOWS
	CRITICAL_SECTION  mutex;  /** used to lock this struct and thread*/
	CRITICAL_SECTION  job_mutex;  /** locked while job is running */
	HANDLE            thread_id; /* handle returned by CreateThread() */
#else
	pthread_mutex_t mutex; /** used to lock this struct and thread*/
	pthread_mutex_t job_mutex; /** locked while job is running */
	pthread_t thread_id;   /** ID returned by pthread_create() */
#endif
	int       thread_num;  /** Application-defined thread # */
	bool      keep_running; /** while true schedule next job */
	struct workqueue_ctx *ctx; /** parent context*/
	struct workqueue_job *job; /** Currently running job or NULL if no job*/
};

/**
* @struct workqueue_ctx
* @brief This the the context of the library. Multiple contexts are permited.
* @brief This data is private to the library.
*/

struct workqueue_ctx
{
#ifdef WINDOWS
	CRITICAL_SECTION  mutex; /** used to lock this struct */
	HANDLE work_ready_cond; /** used to signal waiting threads that new work is ready */
	CRITICAL_SECTION  cond_mutex; /** lock condition. TODO NOT needed on windows? */
#else
	pthread_mutex_t mutex; /** used to lock this struct */
	pthread_cond_t work_ready_cond; /** used to signal waiting threads that new work is ready */
	pthread_mutex_t cond_mutex; /** used to lock condition variable*/
#endif
	struct worker_thread_ops *ops; /** worker init cleanup functions */
	int num_worker_threads; /** Number of worker threads this context has */
	int job_count; /** starts at 0 and goes to 2^31 then back to 0 */
	int queue_size; /** max number of jobs that can be queued */
	int waiting_jobs; /** current number of jobs in queue */
	struct workqueue_thread **thread; /** array of num_worker_threads */
	struct workqueue_job **queue; /** array of queue_size */
};

/* if time X is > y*/
static int _time_gt (struct TIME_STRUCT_TYPE *x, struct TIME_STRUCT_TYPE *y)
{
#ifdef WINDOWS
	if (x->time > y->time || (x->time == y->time && x->millitm > y->millitm) )
		return 1;

#else
	if (x->tv_sec > y->tv_sec || (x->tv_sec == y->tv_sec && x->tv_nsec > y->tv_nsec) )
		return 1;
#endif
	return 0;
}

/**
 * @brief Compare function for qsort()
 * @param ptr1 pointer to a workqueue_job
 * @param ptr2 pointer to a workqueue_job
 * @return 0 if jobs equal or both null. -1 if ptr1 < ptr2
 */
static int job_compare(const void *ptr1, const void *ptr2)
{
  	struct workqueue_job **j1, **j2;
	struct workqueue_job *job1, *job2;

// 	DEBUG_MSG("p1=%p p2=%p \n", ptr1, ptr2);

	/* dereference job pointers*/
	j1 = (struct workqueue_job **) ptr1;
	j2 = (struct workqueue_job **) ptr2;
	job1 = *j1;
	job2 = *j2;

// 	DEBUG_MSG("job1=%p job2=%p \n", job1, job2);

	/* check for null pointers*/
	if ( job1 == NULL && job2 == NULL)
		return 0;
	else if ( job1 == NULL)
	  	return 1; /* j2 is not null*/
	else if (job2 == NULL)
	  	return -1; /* j1 is null */

// 	DEBUG_MSG("pri1=%d pri2=%d id1=%d id2=%d\n",job1->priority, job2->priority, job1->job_id, job2->job_id);

	/* check jobs for priority order */
	if (job1->priority < job2->priority)
		return -1;
	else if (job1->priority > job2->priority)
		return 1;
#ifdef WINDOWS
//FIXME
#else
	/* check jobs for scheduled time */
	if (job1->start_time.tv_sec < job2->start_time.tv_sec)
	  	return -1;
	else if (job1->start_time.tv_sec > job2->start_time.tv_sec)
	  	return 1;

	/* seconds must be equal so compare nanoseconds */
	if (job1->start_time.tv_nsec < job2->start_time.tv_nsec)
	  	return -1;
	else if (job1->start_time.tv_nsec > job2->start_time.tv_nsec)
	  	return 1;
#endif

	if (job1->job_id < job2->job_id)
	  	return -1;
	else if (job1->job_id > job2->job_id)
	  	return 1;

	return 0;
}

/*time x-y */
static long long time_diff_ms(struct TIME_STRUCT_TYPE *x,
				  struct TIME_STRUCT_TYPE *y)
{
	long long diff;
	diff = (TIME_SECP(x) - TIME_SECP(y))*1000;

	diff += TIME_MSECP(x) - TIME_MSECP(y);
	return diff;
}

// dequeue the next job that needes to run in this thread
static struct workqueue_job*  _workqueue_get_job(struct workqueue_thread *thread,
			struct TIME_STRUCT_TYPE *wait_time,
			long long *wait_ms)
{
	int i;
	struct workqueue_job *job = NULL;
	struct workqueue_ctx *ctx = thread->ctx;
	struct TIME_STRUCT_TYPE now;

	if (!thread->keep_running)
		return NULL;

	/* init default wait time*/
	GET_TIME(now);
	*wait_time = now;
	TIME_SECP(wait_time) += 9999;  // if no work wait for 9999 sec
	*wait_ms = 5000;

	assert(ctx);
	DEBUG_MSG("thread %d locking ctx\n",thread->thread_num);
	LOCK_MUTEX(&ctx->mutex);
	DEBUG_MSG("thread %d got lock\n",thread->thread_num);
	assert(ctx->queue);

	/* for each queued job item while keep_running
	 serach for a job and break out if we find one */
	for(i = 0; thread->keep_running && i < ctx->queue_size; i++) {

		/* if queue pointer not null there is a job in this slot */
		if (ctx->queue[i]) {

			DEBUG_MSG("job %d set wait=%u.%03lu start_time=%u.%03lu now=%u.%03lu\n", ctx->queue[i]->job_id,
				(unsigned int) TIME_SECP(wait_time), TIME_MSECP(wait_time),
				(unsigned int) TIME_SEC(ctx->queue[i]->start_time), TIME_MSEC(ctx->queue[i]->start_time),
				(unsigned int) TIME_SEC(now), TIME_MSEC(now));

			/* check scheduled time */
			if (_time_gt(&now, &ctx->queue[i]->start_time))  {
				/* job found that must start now */
				job = ctx->queue[i];
				ctx->queue[i] = NULL;
				DEBUG_MSG("found job %d\n", job->job_id);
				ctx->waiting_jobs--;
				break; /* found job ready to run */
			} else if (_time_gt(wait_time, &ctx->queue[i]->start_time)) {
				/* next job in the queue is not scheduled to be run yet */

				/* calculate time thread should sleep for */
				*wait_time = ctx->queue[i]->start_time;
				*wait_ms = time_diff_ms(&ctx->queue[i]->start_time, &now);
				DEBUG_MSG("waiting %lld ms\n", *wait_ms);
				DEBUG_MSG("set wait to %u.%03lu for job %d\n",
					(unsigned int) TIME_SECP(wait_time), TIME_MSECP(wait_time), ctx->queue[i]->job_id);
			} else {
				DEBUG_MSG("no other job\n", NULL);
			}
		}
	}


	DEBUG_MSG("thread %d unlocking ctx job=%p wait=%u.%03lu\n",
			thread->thread_num, job,
			(unsigned int) TIME_SECP(wait_time), TIME_MSECP(wait_time));

	UNLOCK_MUTEX(&ctx->mutex);
	return job;
}

// return number of jobs waiting in queue
int workqueue_get_queue_len(struct workqueue_ctx* ctx)
{
	int ret;
	LOCK_MUTEX(&ctx->mutex);
	ret = ctx->waiting_jobs;
	UNLOCK_MUTEX(&ctx->mutex);
	return ret;
}

static void * _workqueue_job_scheduler(void *data)
{
  	struct workqueue_thread *thread = (struct workqueue_thread *) data;
	struct workqueue_ctx *ctx;
	struct TIME_STRUCT_TYPE wait_time;
	int ret;
	long long wait_ms = 1000;

	DEBUG_MSG("starting data=%p\n", data);
	assert(thread);
	ctx = thread->ctx;
	DEBUG_MSG("thread %d starting\n",thread->thread_num);

	if (ctx->ops && ctx->ops->worker_constructor) {
		DEBUG_MSG("thread %d calling constructor\n",thread->thread_num);
		ctx->ops->worker_constructor(ctx->ops->data);
	}
	LOCK_MUTEX(&thread->mutex);

	while (thread->keep_running) {

		DEBUG_MSG("thread %d looking for work \n",thread->thread_num);
		thread->job = _workqueue_get_job(thread, &wait_time, &wait_ms);

		/* is there a job that needs to be run now */
		if (thread->job) {
			/* there is work to do */
			DEBUG_MSG("launching job %d\n",thread->job->job_id);

			/* keep job_mutex locked while running to test if running */
			LOCK_MUTEX(&thread->job_mutex);
			/* mantain unlocked while running so we can check state.
			  in workqueue_job_running() */
			UNLOCK_MUTEX(&thread->mutex);

			/* launch worker job */
			thread->job->func(thread->job->data);

			DEBUG_MSG("job %d finished\n", thread->job->job_id);
			/* done with job free it */
			free(thread->job);
			thread->job = NULL;

			UNLOCK_MUTEX(&thread->job_mutex);
			LOCK_MUTEX(&thread->mutex);

		} else {
			/* wait until we are signaled that there is new work, or until the wait time is up */


			/* we should idle */

			UNLOCK_MUTEX(&thread->mutex);

#ifdef WINDOWS
			DEBUG_MSG("thread %d going idle\n",thread->thread_num);
			ret = WaitForSingleObject(&ctx->work_ready_cond, (DWORD) wait_ms);

#else
			DEBUG_MSG("thread %d going idle.  %d sec; %ld nsec\n",
					thread->thread_num, (int) wait_time.tv_sec, wait_time.tv_nsec);
			// note this wait may be a long time, if the system time changes
			LOCK_MUTEX(&ctx->cond_mutex);
			ret = pthread_cond_timedwait(&ctx->work_ready_cond, &ctx->cond_mutex, &wait_time);
			UNLOCK_MUTEX(&ctx->cond_mutex);
#endif

			LOCK_MUTEX(&thread->mutex);

			if (!thread->keep_running) {
			  	DEBUG_MSG("thread %d stopping\n",thread->thread_num);
				break;
			}
#ifdef WINDOWS
			/* check windows error cases*/
			if( !ret && WAIT_TIMEOUT == GetLastError()) {
			  	DEBUG_MSG("thread %d idle timeout\n",thread->thread_num);
			  	continue; /* wait again */
			} if (!ret)  {
				// other error
			}
#else

			if (ret == ETIMEDOUT) {
			  	DEBUG_MSG("thread %d idle timeout lock\n",thread->thread_num);
			  	continue; /* wait again */
			} else if (ret == EINVAL) {
				ERROR_MSG("thread %d pthread_cond_timedwait EINVAL\n", thread->thread_num);
				usleep(wait_ms); // wait 1000th of time wait
			} else if (ret) {
				ERROR_MSG("thread %d pthread_cond_timedwait ret =%d\n", thread->thread_num, ret);
			}

#endif
		}
	}

	UNLOCK_MUTEX(&thread->mutex);

	if (ctx->ops && ctx->ops->worker_destructor) {
		DEBUG_MSG("thread %d calling destructor\n",thread->thread_num);
		ctx->ops->worker_destructor(ctx->ops->data);
	}
	return NULL;
}

static int _empty_queue(struct workqueue_ctx *ctx)
{
	int count = 0;
	int i;
	/* free any remaining jobs left in queue */
	if(ctx->queue) {
		for (i = 0; i < ctx->queue_size; i++) {
			if (ctx->queue[i]) {
				free(ctx->queue[i]);
				ctx->queue[i] = NULL;
				count++;
			}
		}
	}
	return count;
}

void workqueue_destroy(struct workqueue_ctx *ctx)
{
	int i;

	DEBUG_MSG("shutting down ctx=%p\n", ctx);
	LOCK_MUTEX(&ctx->mutex);
	if (ctx->thread) {
		DEBUG_MSG("shutting down %d workers\n", ctx->num_worker_threads);
		for (i = 0; i < ctx->num_worker_threads; i++) {
			if (ctx->thread[i]) {
				ctx->thread[i]->keep_running = false;
			}
		}

#ifdef WINDOWS
		SetEvent(&ctx->work_ready_cond);
#else
		/* send signal to unblock threads */
		pthread_cond_broadcast(&ctx->work_ready_cond);

#endif
		/* Unlock incase the worker callback is makeing its own calls to workqueue_* to avoid deadlock */
		UNLOCK_MUTEX(&ctx->mutex);
		for (i = 0; i < ctx->num_worker_threads; i++) {
			if (ctx->thread[i]) {

			  	DEBUG_MSG("joining thread %d\n",ctx->thread[i]->thread_num);
				pthread_join(ctx->thread[i]->thread_id, NULL);

			}
		}
		LOCK_MUTEX(&ctx->mutex);

		/* for each worker thread, clean up it's context */
		for (i = 0; i < ctx->num_worker_threads; i++) {
			if (ctx->thread[i]) {
				free(ctx->thread[i]);
				ctx->thread[i] = NULL;
			}
		}
		free(ctx->thread); /* free pointer list */
	}


	_empty_queue(ctx);
	free(ctx->queue);
	ctx->queue = NULL;

	UNLOCK_MUTEX(&ctx->mutex);

	#ifdef WINDOWS
	//FIXME test this
	#else
	pthread_mutex_destroy(&ctx->mutex);
	pthread_mutex_destroy(&ctx->cond_mutex);
	#endif

	free(ctx);

}

static struct workqueue_ctx *
__workqueue_init(struct workqueue_ctx *ctx, unsigned int queue_size, unsigned int num_worker_threads)
{
	unsigned int i;
	struct workqueue_thread *thread;
	int ret = 0;

	if (!ctx)
		return NULL;
	ctx->queue_size = queue_size;
	#ifdef WINDOWS
	InitializeCriticalSection(&ctx->mutex);
	InitializeCriticalSection(&ctx->cond_mutex);
	#else
	ret = pthread_mutex_init(&ctx->mutex, NULL);
	if (ret)
		ERROR_MSG("pthread_mutex_init failed ret=%d\n", ret);

	ret = pthread_mutex_init(&ctx->cond_mutex, NULL);
	if (ret)
		ERROR_MSG("pthread_mutex_init failed ret=%d\n", ret);

	#endif
	/* Allocate pointers for queue */
	ctx->queue = (struct workqueue_job **) calloc( queue_size + 1, sizeof(struct workqueue_job *));
	if (!ctx->queue) {
		goto free_ctx;
	}

	/* Allocate pointers for threads */
	ctx->thread = (struct workqueue_thread **) calloc( num_worker_threads + 1, sizeof(struct workqueue_thread *));
	if (!ctx->thread)
		goto free_queue;

	#ifdef WINDOWS
	// Condition variable only work on vista and newer
	//	InitializeConditionVariable(&ctx->work_ready_cond);

	ctx->work_ready_cond = CreateEvent (NULL,  // no security
					    FALSE, // auto-reset event
					    FALSE, // non-signaled initially
					    NULL); // unnamed


	#else
	ret = pthread_cond_init(&ctx->work_ready_cond, NULL);
	if (ret)
		ERROR_MSG("pthread_cond_init failed ret=%d\n", ret);
	#endif
	ctx->num_worker_threads = num_worker_threads;

	for (i = 0; i < num_worker_threads; i++) {
		ctx->thread[i] = thread = (struct workqueue_thread *) calloc ( 1, sizeof(struct workqueue_thread));
		if (!ctx->thread[i]) {
			goto free_threads;
		}
		thread->thread_num = i;
		thread->ctx = ctx;  /* point to parent */
		thread->keep_running = true;
		#ifdef WINDOWS
		InitializeCriticalSection(&thread->mutex);

		thread->thread_id = CreateThread(
		NULL,       // default security attributes
						 0,          // default stack size
						 (LPTHREAD_START_ROUTINE) _workqueue_job_scheduler,
						 thread,       // data passed to thread
						 0,          // default creation flags
						 NULL); // receive thread identifier

		#else
		pthread_mutex_init(&thread->mutex, NULL);
		if (ret)
			ERROR_MSG("pthread_mutex_init failed ret=%d\n", ret);

		ret = pthread_create(&thread->thread_id, NULL, _workqueue_job_scheduler, thread);

		if (ret)
			ERROR_MSG("pthread_create failed ret=%d\n", ret);

		#endif
	}

	return ctx;

	/* error cases to clean up for*/
	free_threads:

	for (i = 0; i < num_worker_threads; i++) {
		if (ctx->thread[i]) {
			free(ctx->thread[i]);
		}
	}

	free_queue:
	free(ctx->queue);

	free_ctx:
	free(ctx);
	fprintf(stderr, "Error initializing\n");
	return NULL;
}

struct workqueue_ctx * workqueue_init(unsigned int queue_size,
	unsigned int num_worker_threads, struct worker_thread_ops *ops)
{
	struct workqueue_ctx *ctx;

	DEBUG_MSG("Starting queue_size=%d\n", queue_size);

	/* check for invalid args */
	if (!queue_size || !num_worker_threads)
	  	return NULL;

	ctx = (struct workqueue_ctx *) calloc(1, sizeof (struct workqueue_ctx));
	ctx->ops = ops;
	return __workqueue_init(ctx, queue_size, num_worker_threads);
}

#if 0

struct workqueue_ctx * workqueue_init_pth_wapper(unsigned int queue_size, unsigned int num_worker_threads,
	 int (*pthread_create_wrapper)(pthread_t *, const pthread_attr_t *,
	       void *(*)(void *), void *))
{
	struct workqueue_ctx *ctx;

	DEBUG_MSG("Starting queue_size=%d\n", queue_size);

	/* check for invalid args */
	if (!queue_size || !num_worker_threads)
		return NULL;

	ctx = (struct workqueue_ctx *) calloc(1, sizeof (struct workqueue_ctx));

	DEBUG_MSG("Set wrapper=%p\n", pthread_create_wrapper);
	ctx->pthread_create_wrapper = pthread_create_wrapper;

	return __workqueue_init(ctx, queue_size, num_worker_threads);
}

#endif

int workqueue_add_work(struct workqueue_ctx* ctx, int priority,
		unsigned int miliseconds,
		workqueue_func_t callback_fn, void *data)
{
	int i;
	int ret;
	struct workqueue_job *job = NULL;
	struct TIME_STRUCT_TYPE sched_time;

	/* get current time time*/
	GET_TIME(sched_time);

	LOCK_MUTEX(&ctx->mutex);

	for (i = 0; i < ctx->queue_size; i++) {
		if (ctx->queue[i])
			continue; /* used location */

		/* found free spot in queue to put job, so allocate memory for it. */
		job = (struct workqueue_job *) calloc(1, sizeof(struct workqueue_job));
		if (!job) {
			UNLOCK_MUTEX(&ctx->mutex);
			return -ENOMEM;
		}
		if (miliseconds) {
			/* get current time time*/
			GET_TIME(job->start_time);
			/* add time */
#ifdef WINDOWS
			job->start_time.time += miliseconds / 1000;
			job->start_time.millitm += miliseconds % 1000;
			// if milili sec overflow carry over to a second.
			if (job->start_time.millitm > 1000) {
				job->start_time.millitm -= 1000;
				job->start_time.time += 1;
			}
#else
			job->start_time.tv_sec += miliseconds / 1000;
			job->start_time.tv_nsec += (miliseconds % 1000) * 1000000;
			// if nano sec overflow carry over to a second.
			if (job->start_time.tv_nsec > 1000000000) {
				job->start_time.tv_nsec -= 1000000000;
				job->start_time.tv_sec += 1;
			}
			DEBUG_MSG("Start time sec=%d  nsec=%ld\n",
				(int) job->start_time.tv_sec, job->start_time.tv_nsec);
#endif
		} /* else the start_time will be 0, so start ASAP */

		job->priority = priority;
		job->data = data;
		job->func = callback_fn;
		ret = job->job_id = ctx->job_count++;
		if (ctx->job_count < 0) /* overflow case */
			ctx->job_count = 0;
		ctx->queue[i] = job;
		ctx->waiting_jobs++;

		DEBUG_MSG("Adding job %p to q %p id=%d pri=%d waiting=%d cb=%p data=%p\n",
			job, ctx->queue[i], job->job_id, job->priority,
			 ctx->waiting_jobs, job->func, job->data );

		qsort(ctx->queue, ctx->queue_size,
		      sizeof(struct workqueue_job *), /* size of pointer to sort */
		      job_compare);
#ifdef WINDOWS
		//WakeConditionVariable(&ctx->work_ready_cond);
		PulseEvent(ctx->work_ready_cond);
#else
		if (pthread_cond_signal(&ctx->work_ready_cond)) {
			ERROR_MSG("invalid condition\n");
		}
#endif

		DEBUG_MSG("unlock mutex\n", NULL);
		UNLOCK_MUTEX(&ctx->mutex);
		return ret;
	}

	/* queues are full */
	DEBUG_MSG("Queues are full\n", NULL);

	UNLOCK_MUTEX(&ctx->mutex);
	/* no room in queue */
	return -EBUSY;
}

int workqueue_show_status(struct workqueue_ctx* ctx, FILE *fp)
{
	int i;
	long long time_ms;
	struct TIME_STRUCT_TYPE now_time;
	GET_TIME(now_time);

	LOCK_MUTEX(&ctx->mutex);
	fprintf(fp, "Number of worker threads=%d \n", ctx->num_worker_threads);
	fprintf(fp, "Total jobs added=%d queue_size=%d waiting_jobs=%d \n", ctx->job_count, ctx->queue_size, ctx->waiting_jobs);
	fprintf(fp, "\n");
	fprintf(fp, "%3s | %8s | %4s | %6s ms\n", "Qi", "JobID", "Pri", "Time" );
	fprintf(fp, "---------------------------------\n");
	for (i = 0; i < ctx->queue_size; i++) {
		if (!ctx->queue[i])
			continue; /* unused location */

		if (TIME_SEC(ctx->queue[i]->start_time) ||
			TIME_MSEC(ctx->queue[i]->start_time)) {
			// has been scheduled for a time in the future.
			time_ms = time_diff_ms(&ctx->queue[i]->start_time, &now_time);
		} else {
			// will run ASAP
			time_ms = 0;
		}

		fprintf(fp,"%3d | %8d | %4d | %6lld ms\n", i,
			ctx->queue[i]->job_id,
			ctx->queue[i]->priority, time_ms);
	}

	UNLOCK_MUTEX(&ctx->mutex);

	fflush(fp);
	return 0;
}

static int _is_job_queued(struct workqueue_ctx* ctx, int job_id)
{
	int i;

	if(ctx->queue) {
		for (i = 0; i < ctx->queue_size; i++) {
			if (ctx->queue[i] && ctx->queue[i]->job_id == job_id)
				return 1;
		}
	}
	return 0;
}

static int _is_job_running(struct workqueue_ctx* ctx, int job_id)
{
	int i;
	int ret = 0;
	int rc;
	for (i = 0; i < ctx->num_worker_threads && !ret; i++) {
		if (ctx->thread[i]) {
			TRY_LOCK_MUTEX(&ctx->thread[i]->mutex, rc);
#ifdef WINDOWS
			if (rc)
				return -EBUSY;
#else
			if (rc == EBUSY)
				return -EBUSY;
#endif
			if (ctx->thread[i]->job && ctx->thread[i]->job->job_id == job_id) {
				ret = 1;
			}
			UNLOCK_MUTEX(&ctx->thread[i]->mutex);
		}
	}

	return ret;
}

int workqueue_job_queued(struct workqueue_ctx* ctx, int job_id)
{
	int ret;

	if (!ctx)
		return -1;

	LOCK_MUTEX(&ctx->mutex);
	ret = _is_job_queued(ctx, job_id);
	UNLOCK_MUTEX(&ctx->mutex);
	return ret;
}

int workqueue_job_running(struct workqueue_ctx* ctx, int job_id)
{
	int ret = 0;

	if (!ctx)
		return -1;


	do {
		LOCK_MUTEX(&ctx->mutex);
		ret = _is_job_running(ctx, job_id);

		UNLOCK_MUTEX(&ctx->mutex);
	} while (ret == -EBUSY);


	return ret;
}

int workqueue_job_queued_or_running(struct workqueue_ctx* ctx, int job_id)
{
	int ret;

	if (!ctx)
		return -1;

	LOCK_MUTEX(&ctx->mutex);
	ret = _is_job_queued(ctx, job_id);
	UNLOCK_MUTEX(&ctx->mutex);

	if (!ret) {
		do {
			LOCK_MUTEX(&ctx->mutex);
			ret = _is_job_running(ctx, job_id);

			UNLOCK_MUTEX(&ctx->mutex);
		} while (ret == -EBUSY);
	}



	return ret;
}

/* private function. NOTE ctx must be locked by caller to avoid race with other dequeue*/
static int _dequeue(struct workqueue_ctx* ctx, int job_id)
{
	int i;

	if(ctx->queue) {
		for (i = 0; i < ctx->queue_size; i++) {
			if (ctx->queue[i] && ctx->queue[i]->job_id == job_id) {
				free(ctx->queue[i]);
				ctx->queue[i] = NULL;
				ctx->waiting_jobs--;
				return 0;
			}
		}
	}
	return -ENOENT;
}

#if 0  /* This probally can't be reliably done.  At best we could send a signal */
static int _kill_job(struct workqueue_ctx* ctx, int job_id)
{
	int i;
	int ret = -ENOENT;

	for (i = 0; i < ctx->num_worker_threads; i++) {
		if (ctx->thread[i]) {
			LOCK_MUTEX(&ctx->thread[i]->mutex);
			if (ctx->thread[i]->job && ctx->thread[i]->job->job_id == job_id) {
				/* FOUND*/
#ifdef WINDOWS
			TerminateThread(thread->thread_id, 0);

			thread->thread_id = CreateThread(
				NULL,       // default security attributes
				0,          // default stack size
				(LPTHREAD_START_ROUTINE) _workqueue_job_scheduler,
				thread,       // data passed to thread
				0,          // default creation flags
				NULL); // receive thread identifier

#else /*POSIX */
				if ( (ret = pthread_kill(ctx->thread[i]->thread_id , SIGTERM)) )
					ERROR_MSG("pthread_kill err = %d\n,", ret);

				/* recreate worker thread */
				pthread_create(&ctx->thread[i]->thread_id, NULL,
					_workqueue_job_scheduler, ctx->thread[i]);
#endif

				UNLOCK_MUTEX(&ctx->thread[i]->mutex);
				return 0;
			}
			UNLOCK_MUTEX(&ctx->thread[i]->mutex);
		}
	}


	return -ENOENT;
}

int workqueue_kill(struct workqueue_ctx* ctx, int job_id)
{
	int ret;
	LOCK_MUTEX(&ctx->mutex);

	ret = _kill_job(ctx, job_id);

	UNLOCK_MUTEX(&ctx->mutex);
	return ret;
}

int workqueue_cancel(struct workqueue_ctx* ctx, int job_id)
{

	int ret;
	LOCK_MUTEX(&ctx->mutex);
	ret = _dequeue(ctx, job_id);

	if (ret == -ENOENT)  {
		/* kill any running thread that has this job id */
		ret = _kill_job(ctx, job_id);
	}
	UNLOCK_MUTEX(&ctx->mutex);
	return ret;
}
#endif

int workqueue_dequeue(struct workqueue_ctx* ctx, int job_id)
{
	int ret;
	LOCK_MUTEX(&ctx->mutex);
	ret = _dequeue(ctx, job_id);

	UNLOCK_MUTEX(&ctx->mutex);
	return ret;
}


int workqueue_empty(struct workqueue_ctx *ctx)
{
	int count;
	LOCK_MUTEX(&ctx->mutex);

	count = _empty_queue(ctx);

	UNLOCK_MUTEX(&ctx->mutex);
	return count;
}

int workqueue_empty_wait(struct workqueue_ctx *ctx)
{
	int count;
	int i;
	int num_workers = ctx->num_worker_threads;

	LOCK_MUTEX(&ctx->mutex);
	count = _empty_queue(ctx);
	num_workers = ctx->num_worker_threads;
	UNLOCK_MUTEX(&ctx->mutex);



	for (i = 0; i < num_workers; i++) {
		if (ctx->thread[i]) {

			LOCK_MUTEX(&ctx->thread[i]->mutex);
			LOCK_MUTEX(&ctx->thread[i]->job_mutex);
			if (ctx->thread[i]->job) {
				ERROR_MSG("no job should be running\n");
			}
			UNLOCK_MUTEX(&ctx->thread[i]->job_mutex);
			UNLOCK_MUTEX(&ctx->thread[i]->mutex);

		}
	}




	return count;
}