#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

#define DEBUG_WORKQUEUE
#ifdef DEBUG_WORKQUEUE
#define DEBUG_MSG(fmt, s...) { printf("%s:%d " fmt, __FUNCTION__,__LINE__, ## s); }
#else
#define DEBUG_PRINTF(fmt, s...) 
#define DEBUG_MSG(val)
#endif

#include "workqueue.h"



struct workqueue_job
{
	int priority;
	unsigned int job_id;
	struct timespec start_time;
	workqueue_func_t func;
	void * data;
};

struct workqueue_thread
{
  	pthread_mutex_t mutex; /* used to lock this struct and thread*/
	pthread_cond_t wait_cond;
	pthread_t thread_id;   /* ID returned by pthread_create() */
	int       thread_num;  /* Application-defined thread # */
	bool      keep_running; /* while true schedule next job */
	struct workqueue_ctx *ctx; /* parent context*/
};

struct workqueue_ctx
{
	pthread_mutex_t mutex; /* used to lock this  struct */
	int num_worker_threads;
	int job_count; /* starts at 0 and goes to 2^31 then back to 0 */
	int queue_size;
	int waiting_jobs;
	struct workqueue_thread **thread; /* array of num_worker_threads */
	struct workqueue_job **queue; /* array of queue_size */
};

static int job_compare(const void *j1, const void *j2)
{
	struct workqueue_job *job1, *job2;
	
	if ( j1 == NULL && j2 == NULL)
		return 0;
	else if ( j1 == NULL) 
	  	return 1; /* j2 is not null*/
	else if (j2 == NULL)
	  	return -1; /* j1 is null */
	  
	job1 = (struct workqueue_job *)j1;
	job2 = (struct workqueue_job *)j2;

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
	/* init default wait time*/
	wait_time->tv_sec=5;
	wait_time->tv_nsec=0;

	assert(ctx);
	pthread_mutex_lock(&ctx->mutex);
	assert(ctx->queue);

	
	for(i = 0; i < ctx->queue_size; i++) {
	  	// TODO check sheduled time
		if (ctx->queue[i]) {
		  	job = ctx->queue[i];
			ctx->queue[i] = NULL;
			DEBUG_MSG("found Job %d\n",job->job_id);
			ctx->waiting_jobs--;
			break;
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
	struct timespec wait_time;
// 	int ret;
	
	while (thread->keep_running) {
		job = _workqueue_get_job(thread->ctx, &wait_time);
		if (job) {
		  	DEBUG_MSG("launching job %d\n",job->job_id);
			job->func(job->data); /* launch worker job */
			DEBUG_MSG("job %d finished\n",job->job_id);
			free(job);
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

	if(ctx->queue) {
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

	ctx->num_worker_threads = num_worker_threads;
	
	for (i = 0; i < num_worker_threads; i++) {
	  	ctx->thread[i] = thread = calloc ( 1, sizeof(struct workqueue_thread));
	  	if (!ctx->thread[i]) {
			goto free_threads;
		}
		thread->thread_num = i;
		thread->ctx = ctx;  /* point to parent */
		thread->keep_running = true;
		
		pthread_cond_init(&thread->wait_cond, NULL);
		pthread_mutex_init(&thread->mutex, NULL);
		pthread_mutex_lock(&thread->mutex); /* must be locked for pthread_cond_timedwait */
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
		int miliseconds,
		workqueue_func_t callback_fn, void *data)
{
  	int i;
	int ret;
	struct workqueue_job *job = NULL;
	
	pthread_mutex_lock(&ctx->mutex);
	
	for (i = 0; i < ctx->queue_size; i++) {
		if (ctx->queue[i])
			continue; /* used location */

		job = calloc(1, sizeof(struct workqueue_job));
		if (!job) {
		  	pthread_mutex_unlock(&ctx->mutex);
			return -ENOMEM;
		}
		job->priority = priority;
		job->data = data;
		job->func = callback_fn;
		ret = job->job_id = ctx->job_count++;
		if (ctx->job_count < 0) /* overflow case */
			ctx->job_count = 0;  
		ctx->queue[i] = job;
		ctx->waiting_jobs++;
	
		pthread_mutex_unlock(&ctx->mutex);
		return ret;
	}

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
	fprintf(fp, "%8s | %4s\n", "JobID", "Pri" );
	for (i = 0; i < ctx->queue_size; i++) {
		if (!ctx->queue[i])
			continue; /* unused location */

		fprintf(fp,"%8d | %4d \n", ctx->queue[i]->job_id, ctx->queue[i]->priority);
	}

	pthread_mutex_unlock(&ctx->mutex);

	fflush(fp);
	return 0;
}

