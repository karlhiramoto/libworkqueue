/**
* @file Workqueue.c
* @author Karl Hiramoto <karl@hiramoto.org>
* @brief Library for priority work queues.
*
* Distributed under LGPL see COPYING.LGPL
* Copyright 2009 Karl Hiramoto
*/

#define DEBUG

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
#define EBUSY ERROR_BUSY

#define ERROR_MSG(fmt, ...) { printf("%s:%d " fmt, __FUNCTION__, __LINE__, __VA_ARGS__); }

#define GET_TIME(x) _ftime_s(&x)
#define TIME_STRUCT_TYPE _timeb

#define LOCK_MUTEX(mutex)	WaitForSingleObject(mutex,INFINITE)
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
#define ERROR_MSG(fmt, s...) { fprintf(stderr, "%s:%d " fmt, __FUNCTION__,__LINE__, ## s); }

#define GET_TIME(x) clock_gettime(CLOCK_REALTIME, &x)
#define TIME_STRUCT_TYPE timespec
#define LOCK_MUTEX(mutex)	pthread_mutex_lock(mutex)
#define UNLOCK_MUTEX(x)	pthread_mutex_unlock(x)


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
	CRITICAL_SECTION  mutex;
	HANDLE            thread_id;
#else
	pthread_mutex_t mutex; /** used to lock this struct and thread*/
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
	CRITICAL_SECTION  mutex;
//	CONDITION_VARIABLE work_ready_cond;
	HANDLE work_ready_cond;
#else 
	pthread_mutex_t mutex; /** used to lock this  struct */
	pthread_cond_t work_ready_cond; /** used to signal waiting threads that new work is ready */
#endif
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
 * @param ptr1 
 * @param ptr2 
 * @return 
 */
static int job_compare(const void *ptr1, const void *ptr2)
{
  	struct workqueue_job **j1, **j2;
	struct workqueue_job *job1, *job2;
	
// 	DEBUG_MSG("p1=%p p2=%p \n", ptr1, ptr2);

	j1 = (struct workqueue_job **) ptr1;
	j2 = (struct workqueue_job **) ptr2;


	job1 = *j1;
	job2 = *j2;

// 	DEBUG_MSG("job1=%p job2=%p \n", job1, job2);
	
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
	TIME_SECP(wait_time) += 5;
	*wait_ms = 5000;

	assert(ctx);
	LOCK_MUTEX(&ctx->mutex);
	assert(ctx->queue);

	for(i = 0; thread->keep_running && i < ctx->queue_size; i++) {
	  	// TODO check sheduled time
		if (ctx->queue[i]) {
			DEBUG_MSG("job %d set wait=%u.%03lu start_time=%u.%03lu now=%u.%03lu\n", ctx->queue[i]->job_id,
				(unsigned int) TIME_SECP(wait_time), TIME_MSECP(wait_time),
				(unsigned int) TIME_SEC(ctx->queue[i]->start_time), TIME_MSEC(ctx->queue[i]->start_time),
				(unsigned int) TIME_SEC(now), TIME_MSEC(now));

			if (_time_gt(&now, &ctx->queue[i]->start_time))  {
				/* job found that must start now */
				job = ctx->queue[i];
				ctx->queue[i] = NULL;
				DEBUG_MSG("found job %d\n", job->job_id);
				ctx->waiting_jobs--;
				break;
			} else if (_time_gt(wait_time, &ctx->queue[i]->start_time)) {
				/* next job in the queue is not scheduled to be run yet */
				*wait_time = ctx->queue[i]->start_time;
				*wait_ms = time_diff_ms(wait_time, &now);
				DEBUG_MSG("waiting %lld ms\n", *wait_ms);
				DEBUG_MSG("set wait to %u.%03lu for job %d\n",
					(unsigned int) TIME_SECP(wait_time), TIME_MSECP(wait_time), ctx->queue[i]->job_id);
			} else {
				DEBUG_MSG("no other job\n", NULL);
			}
		}
	}
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

	assert(thread);
	ctx = thread->ctx;
	DEBUG_MSG("thread %d starting\n",thread->thread_num);
	
	while (thread->keep_running) {

		thread->job = _workqueue_get_job(thread, &wait_time, &wait_ms);
		if (thread->job) {
			/* there is work to do */
			DEBUG_MSG("launching job %d\n",thread->job->job_id);
			thread->job->func(thread->job->data); /* launch worker job */
			DEBUG_MSG("job %d finished\n", thread->job->job_id);
			LOCK_MUTEX(&thread->mutex);
			/* done with job free it */
			free(thread->job);
			thread->job = NULL;
			UNLOCK_MUTEX(&thread->mutex);
		} else {
			/* must be locked for pthread_cond_timedwait */
			LOCK_MUTEX(&thread->mutex);

			/* we should idle */
			DEBUG_MSG("thread %d going idle\n",thread->thread_num);
#ifdef WINDOWS
			ret = WaitForSingleObject(&ctx->work_ready_cond, (DWORD) wait_ms);

#else
			// note this wait may be a long time, if the system time changes
			ret = pthread_cond_timedwait(&ctx->work_ready_cond, &thread->mutex, &wait_time);
#endif
			UNLOCK_MUTEX(&thread->mutex);

			if (!thread->keep_running) {
			  	DEBUG_MSG("thread %d stopping\n",thread->thread_num);
			  	return NULL;
			}
#ifdef WINDOWS
			if( !ret && WAIT_TIMEOUT == GetLastError()) {
			  	DEBUG_MSG("thread %d idle timeout\n",thread->thread_num);
			  	continue; /* wait again */
			} if (!ret)  {
				// other error
			}
#else
			if (ret == ETIMEDOUT) {
			  	DEBUG_MSG("thread %d idle timeout\n",thread->thread_num);
			  	continue; /* wait again */
			} else if (ret) {
				ERROR_MSG("pthread_cond_timedwait ret =%d\n", ret);
				perror("Error waiting for thread condition");
				continue;
			}
#endif
		}
	}
	return NULL;
}

void workqueue_destroy(struct workqueue_ctx *ctx)
{
  	int i;
	DEBUG_MSG("shutting down ctx=%p\n", ctx);
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
		for (i = 0; i < ctx->num_worker_threads; i++) {
			if (ctx->thread[i]) {
			  	DEBUG_MSG("joining thread%d\n",ctx->thread[i]->thread_num);
//#ifdef WINDOWS
//				WaitForSingleObject(ctx->thread[i]->thread_id, 5000);
//				CloseHandle (ctx->thread[i]->thread_id);

//#else
				pthread_join(ctx->thread[i]->thread_id, NULL);
//#endif
				
			}
		}

		for (i = 0; i < ctx->num_worker_threads; i++) {
			if (ctx->thread[i]) {
				free(ctx->thread[i]);
			}
		}
		free(ctx->thread);
	}

	/* free any remaining jobs left in queue */
	if(ctx->queue) {
		for (i = 0; i < ctx->queue_size; i++) {
			if (ctx->queue[i]) {
				free(ctx->queue[i]);
				ctx->queue[i] = NULL;
			}
		}
		free(ctx->queue);
	}

	free(ctx);

}

struct workqueue_ctx * workqueue_init(unsigned int queue_size, unsigned int num_worker_threads)
{
  	unsigned int i;
	struct workqueue_ctx *ctx;
	struct workqueue_thread *thread;
	
	DEBUG_MSG("Starting queue_size=%d\n", queue_size);

	/* check for invalid args */
	if (!queue_size || !num_worker_threads)
	  	return NULL;

	ctx = (struct workqueue_ctx *) calloc(1, sizeof (struct workqueue_ctx));
	if (!ctx)
		return NULL;
	ctx->queue_size = queue_size;
#ifdef WINDOWS
	InitializeCriticalSection(&ctx->mutex);
#else
	pthread_mutex_init(&ctx->mutex, NULL);
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
	pthread_cond_init(&ctx->work_ready_cond, NULL);
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
		pthread_create(&thread->thread_id, NULL, _workqueue_job_scheduler, thread);
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

#else
			job->start_time.tv_sec += miliseconds / 1000;
			job->start_time.tv_nsec += (miliseconds % 1000) * 1000000;
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

	LOCK_MUTEX(&ctx->mutex);

	fprintf(fp, "Number of worker threads=%d \n", ctx->num_worker_threads);
	fprintf(fp, "Total jobs added=%d queue_size=%d waiting_jobs=%d \n", ctx->job_count, ctx->queue_size, ctx->waiting_jobs);
	fprintf(fp, "\n---------\n");
	fprintf(fp, "%3s | %8s | %4s\n", "Qi", "JobID", "Pri" );
	for (i = 0; i < ctx->queue_size; i++) {
		if (!ctx->queue[i])
			continue; /* unused location */

		fprintf(fp,"%3d | %8d | %4d \n", i, ctx->queue[i]->job_id, ctx->queue[i]->priority);
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
	for (i = 0; i < ctx->num_worker_threads && !ret; i++) {
		if (ctx->thread[i]) {
			LOCK_MUTEX(&ctx->thread[i]->mutex);
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
	int ret;

	if (!ctx)
		return -1;

	LOCK_MUTEX(&ctx->mutex);
	ret = _is_job_running(ctx, job_id);
	UNLOCK_MUTEX(&ctx->mutex);
	return ret;
}

int workqueue_job_queued_or_running(struct workqueue_ctx* ctx, int job_id)
{
	int ret;

	if (!ctx)
		return -1;

	LOCK_MUTEX(&ctx->mutex);
	ret = _is_job_queued(ctx, job_id);
	if (!ret)
		ret = _is_job_running(ctx, job_id);
	UNLOCK_MUTEX(&ctx->mutex);
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
