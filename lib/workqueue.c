/**
* @file Workqueue.c
* @author Karl Hiramoto <karl@hiramoto.org>
* @brief Library for priority work queues.
*
* Distributed under LGPL see COPYING.LGPL
* Copyright 2009 Karl Hiramoto
*/


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

// #define DEBUG
#ifdef DEBUG
#define DEBUG_MSG(fmt, s...) { printf("%s:%d " fmt, __FUNCTION__,__LINE__, ## s); }
#else
#define DEBUG_MSG(fmt, s...)
#endif

#include "workqueue.h"


#define ERROR_MSG(fmt, s...) { fprintf(stderr, "%s:%d " fmt, __FUNCTION__,__LINE__, ## s); }

/**
* @struct workqueue_job
* @brief Every scheduled or running job has a instance of this struct
* @brief This data is private to the library
*/
struct workqueue_job
{
	int priority; /** Job priority.  Lower number is higher priority as on all UNIX OS's */
	unsigned int job_id; /** Job ID 1st job is 1 then goes up */
	struct timespec start_time; /** Time the job was started */
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
  	pthread_mutex_t mutex; /** used to lock this struct and thread*/
	pthread_t thread_id;   /** ID returned by pthread_create() */
	int       thread_num;  /** Application-defined thread # */
	bool      keep_running; /** while true schedule next job */
	struct workqueue_ctx *ctx; /** parent context*/
};

/**
* @struct workqueue_ctx
* @brief This the the context of the library. Multiple contexts are permited.
* @brief This data is private to the library.
*/

struct workqueue_ctx
{
	pthread_mutex_t mutex; /** used to lock this  struct */
	pthread_cond_t work_ready_cond; /** used to signal waiting threads that new work is ready */
	int num_worker_threads; /** Number of worker threads this context has */
	int job_count; /** starts at 0 and goes to 2^31 then back to 0 */
	int queue_size; /** max number of jobs that can be queued */
	int waiting_jobs; /** current number of jobs in queue */
	struct workqueue_thread **thread; /** array of num_worker_threads */
	struct workqueue_job **queue; /** array of queue_size */
};


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

	/* check jobs for scheduled time */
	if (job1->start_time.tv_sec < job2->start_time.tv_sec)
	  	return -1;
	else if (job1->start_time.tv_sec > job2->start_time.tv_sec)
	  	return 1;

	if (job1->start_time.tv_nsec < job2->start_time.tv_nsec)
	  	return -1;
	else if (job1->start_time.tv_nsec > job2->start_time.tv_nsec)
	  	return 1;

	if (job1->job_id < job2->job_id)
	  	return -1;
	else if (job1->job_id > job2->job_id)
	  	return 1;

	return 0;
}
// dequeu the next job that needes to run
struct workqueue_job*  _workqueue_get_job(struct workqueue_ctx* ctx, struct timespec *wait_time)
{
	int i;
	struct workqueue_job *job = NULL;
	struct timespec now;
	/* init default wait time*/
	clock_gettime(CLOCK_REALTIME, &now);
	*wait_time = now;
	wait_time->tv_sec+=5;

	assert(ctx);
	pthread_mutex_lock(&ctx->mutex);
	assert(ctx->queue);
 	
	
	for(i = 0; i < ctx->queue_size; i++) {
	  	// TODO check sheduled time
		if (ctx->queue[i]) {
			DEBUG_MSG("job %d set wait=%u.%09lu start_time=%u.%09lu now=%u.%09lu\n", ctx->queue[i]->job_id,
				(unsigned int) wait_time->tv_sec, wait_time->tv_nsec,
				(unsigned int) ctx->queue[i]->start_time.tv_sec, ctx->queue[i]->start_time.tv_nsec,
				(unsigned int) now.tv_sec, now.tv_nsec);

			if (now.tv_sec > ctx->queue[i]->start_time.tv_sec ||
			(now.tv_sec == ctx->queue[i]->start_time.tv_sec &&
			now.tv_nsec > ctx->queue[i]->start_time.tv_nsec) )  {
				job = ctx->queue[i];
				ctx->queue[i] = NULL;
				DEBUG_MSG("found job %d\n", job->job_id);
				ctx->waiting_jobs--;
				break;
			} else if (wait_time->tv_sec > ctx->queue[i]->start_time.tv_sec ||
				(wait_time->tv_sec == ctx->queue[i]->start_time.tv_sec &&
				wait_time->tv_nsec > ctx->queue[i]->start_time.tv_nsec)) {
				
				*wait_time = ctx->queue[i]->start_time;
				DEBUG_MSG("set wait to %u.%lu for job %d\n",
					(unsigned int) wait_time->tv_sec, wait_time->tv_nsec, ctx->queue[i]->job_id);
			} else {
				DEBUG_MSG("no other job\n");
			}
		}
	}
	pthread_mutex_unlock(&ctx->mutex);
	return job;
}

// return number of jobs waiting in queue
int workqueue_get_queue_len(struct workqueue_ctx* ctx)
{
  	int ret;
	assert(ctx);
	
	pthread_mutex_lock(&ctx->mutex);
	ret = ctx->waiting_jobs;
	pthread_mutex_unlock(&ctx->mutex);
	
	return ret;
}

static void * _workqueue_job_scheduler(void *data)
{
  	struct workqueue_thread *thread = (struct workqueue_thread *) data;
	struct workqueue_job *job = NULL;
	struct workqueue_ctx *ctx;
	struct timespec wait_time = { .tv_sec=5,  .tv_nsec=0 };
 	int ret;

	assert(thread);
	ctx = thread->ctx;
	DEBUG_MSG("thread %d starting\n",thread->thread_num);
	
	while (thread->keep_running) {

		job = _workqueue_get_job(thread->ctx, &wait_time);
		if (job) {
		  	/* there is work to do */
		  	DEBUG_MSG("launching job %d\n",job->job_id);
			job->func(job->data); /* launch worker job */
			DEBUG_MSG("job %d finished\n",job->job_id);
			free(job);
		} else {
		  	/* we should idle */
			DEBUG_MSG("thread %d going idle\n",thread->thread_num);
		  	pthread_mutex_lock(&thread->mutex); /* must be locked for pthread_cond_timedwait */
			DEBUG_MSG("thread %d waiting\n",thread->thread_num);

			// note this wait may be a long time, if the system time changes
			ret = pthread_cond_timedwait(&ctx->work_ready_cond, &thread->mutex, &wait_time);

			pthread_mutex_unlock(&thread->mutex);
			
			if (!thread->keep_running) {
			  	DEBUG_MSG("thread %d stopping\n",thread->thread_num);
			  	return NULL;
			}
			
			if (ret == ETIMEDOUT) {
			  	DEBUG_MSG("thread %d idle timeout\n",thread->thread_num);
			  	continue; /* wait again */
			} else if (ret) {
				perror("Error waiting for thread condition");
				continue;
			}

		}
	}
	return NULL;
}

void workqueue_destroy(struct workqueue_ctx *ctx)
{
  	int i;
	DEBUG_MSG("shutting down\n");
	if (ctx->thread) {
		DEBUG_MSG("shutting down %d workers\n", ctx->num_worker_threads);
		for (i = 0; i < ctx->num_worker_threads; i++) {
			if (ctx->thread[i]) {
				ctx->thread[i]->keep_running = false;
			}
		}

		/* send signal to unblock threads */
		pthread_cond_broadcast(&ctx->work_ready_cond);
		for (i = 0; i < ctx->num_worker_threads; i++) {
			if (ctx->thread[i]) {
			  	DEBUG_MSG("joining thread%d\n",ctx->thread[i]->thread_num);
				pthread_join(ctx->thread[i]->thread_id, NULL);
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
  	int i;
	struct workqueue_ctx *ctx;
	struct workqueue_thread *thread;
	
	/* check for invalid args */
	if (!queue_size || !num_worker_threads)
	  	return NULL;

	ctx = calloc(1, sizeof (struct workqueue_ctx));
	if (!ctx)
		return NULL;
	ctx->queue_size = queue_size;
	pthread_mutex_init(&ctx->mutex, NULL);
	
	/* Allocate pointers for queue */
	ctx->queue = calloc( queue_size + 1, sizeof(struct workqueue_job *));
	if (!ctx->queue) {
		goto free_ctx;
	}

	/* Allocate pointers for threads */
	ctx->thread = calloc( num_worker_threads + 1, sizeof(struct workqueue_thread *));
	if (!ctx->thread) {
	  	goto free_queue;
	}

	pthread_cond_init(&ctx->work_ready_cond, NULL);
	ctx->num_worker_threads = num_worker_threads;
	
	for (i = 0; i < num_worker_threads; i++) {
	  	ctx->thread[i] = thread = calloc ( 1, sizeof(struct workqueue_thread));
	  	if (!ctx->thread[i]) {
			goto free_threads;
		}
		thread->thread_num = i;
		thread->ctx = ctx;  /* point to parent */
		thread->keep_running = true;
		

		pthread_mutex_init(&thread->mutex, NULL);
		pthread_create(&thread->thread_id, NULL, _workqueue_job_scheduler, thread);
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
	struct timespec sched_time;

	/* get current time time*/
	clock_gettime(CLOCK_REALTIME, &sched_time);

	pthread_mutex_lock(&ctx->mutex);

	for (i = 0; i < ctx->queue_size; i++) {
		if (ctx->queue[i])
			continue; /* used location */

		/* found free spot in queue to put job, so allocate memory for it. */
		job = calloc(1, sizeof(struct workqueue_job));
		if (!job) {
			pthread_mutex_unlock(&ctx->mutex);
			return -ENOMEM;
		}
		if (miliseconds) {
			/* get current time time*/
			clock_gettime(CLOCK_REALTIME, &job->start_time);
			/* add time */
			job->start_time.tv_sec += miliseconds / 1000;
			job->start_time.tv_nsec += (miliseconds % 1000) * 1000000;
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

		if (pthread_cond_signal(&ctx->work_ready_cond)) {
			ERROR_MSG("invalid condition\n");
		}

		pthread_mutex_unlock(&ctx->mutex);
		return ret;
	}
	
	/* queues are full */
	DEBUG_MSG("Queues are full\n");
	
	pthread_mutex_unlock(&ctx->mutex);
	/* no room in queue */
	return -EBUSY;
}

int workqueue_show_status(struct workqueue_ctx* ctx, FILE *fp)
{
	int i;
	
	pthread_mutex_lock(&ctx->mutex);
	fprintf(fp, "Number of worker threads=%d \n", ctx->num_worker_threads);
	fprintf(fp, "Total jobs added=%d queue_size=%d waiting_jobs=%d \n", ctx->job_count, ctx->queue_size, ctx->waiting_jobs);
	fprintf(fp, "\n---------\n");
	fprintf(fp, "%3s | %8s | %4s\n", "Qi", "JobID", "Pri" );
	for (i = 0; i < ctx->queue_size; i++) {
		if (!ctx->queue[i])
			continue; /* unused location */

		fprintf(fp,"%3d | %8d | %4d \n", i, ctx->queue[i]->job_id, ctx->queue[i]->priority);
	}

	pthread_mutex_unlock(&ctx->mutex);

	fflush(fp);
	return 0;
}

