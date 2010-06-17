/**
* @file workqueue.h
* @brief public include header for priority work queues..
* @author Karl Hiramoto <karl@hiramoto.org>
*
* Distributed under LGPL see COPYING.LGPL
* Copyright 2009 Karl Hiramoto
*/

#ifndef WORKQUEUE_H
#define WORKQUEUE_H 1

#ifdef  __cplusplus
extern "C" {
#endif


#include <time.h>

/* forward declaration for struct workqueue_ctx*/
struct workqueue_ctx;

/** callback function format for each work task */
typedef void (*workqueue_func_t)(void *data);

struct worker_thread_ops
{
	/**
	* Optional callback to init/allocate any private data.
	* This is called once on initialization of each worker thread
	*/
	void (*worker_constructor)(void *data);
	
	/**
	* Optional callback to free/cleanup any private data, allocated by
	* worker_create this will be called before the worker thread exits
	* this is only called when workqueue_destroy() is called
	*/
	void (*worker_destructor)(void *data);
	
	/** optional data that may be passed to constructor/destructor */
	void *data;
};

/**
 * @fn struct workqueue_ctx* workqueue_init(unsigned int queue_size, unsigned int num_worker_threads);
 * @brief Initialize work context
 * @param queue_size size of queue.  Maximum number of jobs you can queue.
 * @param num_worker_threads  Number of threads to do the work.
 * @param worker_thread_ops optional callbacks to initialize and cleanup worker threads
 *        if this is NULL, no special init/cleanup is done.
 * @returns pointer to context data.  This data is private to the library and may not be used externally.   Free the context with workqueue_destroy
*/

struct workqueue_ctx * workqueue_init(unsigned int queue_size, unsigned int num_worker_threads,
		struct worker_thread_ops *ops);

/**
 * @fn workqueue_add_work
 * @brief Enqueue work
 * @param workqueue_ctx context
 * @param priority  0 is highest.
 * @param when_milisec  milliseconds in the future to schedule. 0 is now.
 * @param callback_fn  function pointer for callback
 * @param data  data to pass to work callback function. 
 * @returns positive Job ID or negative  -errno; -EBUSY when queues are full
 */
int workqueue_add_work(struct workqueue_ctx* ctx,
		int priority, unsigned int when_milisec,
		workqueue_func_t callback_fn, void *data);

/**
 * @fn workqueue_show_status
 * @brief Print out status, usefull for debug
 * @param workqueue_ctx context
 * @param fp  file pointer of where to print may be stdout or stderr, or file if you want.
 * @returns 0 for OK or -errno
*/
int workqueue_show_status(struct workqueue_ctx* ctx, FILE *fp);

/**
 * @fn workqueue_get_queue_len
 * @brief Get the current queue length
 * @param workqueue_ctx  context
 * @returns Number of jobs currently waiting in queue.
 *          Does not count job if it's currently execuing. 
 */
int workqueue_get_queue_len(struct workqueue_ctx* ctx);

/**
 * @fn workqueue_job_queued
 * @brief Check if job is currently queued
 * @param workqueue_ctx  context
 * @returns 1 if Job is queued. 0 if not queued. or -error number.
 */
int workqueue_job_queued(struct workqueue_ctx* ctx, int job_id);

/**
 * @fn workqueue_job_running
 * @brief Check if job is running
 * @param workqueue_ctx  context
 * @returns 1 if Job is running. 0 if not running. or -error number.
 */
int workqueue_job_running(struct workqueue_ctx* ctx, int job_id);

/**
 * @fn workqueue_job_queued_or_running
 * @brief Check if job is queued or running
 * @param workqueue_ctx  context
 * @returns 1 if Job is running or in queue. 0 if not running or in queue. or -error number.
 */

int workqueue_job_queued_or_running(struct workqueue_ctx* ctx, int job_id);


/**
 * @fn workqueue_dequeue
 * @brief If this job in queued but not yet running, dequeue it. cancel the work.
 * @param workqueue_ctx  context
 * @param job_id  ID returned from workqueue_add_work
 * @returns 0 for OK or -error number.
 */
int workqueue_dequeue(struct workqueue_ctx* ctx, int job_id);

/**
 * @fn workqueue_destroy
 * @brief free context, releases all memory.  Any jobs in queue are dequed.
 * @brief Will wait for any currently running jobs to finish.
 * @param workqueue_ctx  context to free
 * @returns void
 */
void workqueue_destroy(struct workqueue_ctx *ctx);

/**
 * @fn workqueue_empty_wait
 * @brief Empty and reset the queue.  Cancel all queued work
 * @param workqueue_ctx  context to free
 * @returns number of entries removed  or -errno
 */
int workqueue_empty(struct workqueue_ctx *ctx);

/**
 * @fn workqueue_empty_wait
 * @brief Empty and reset the queue
 * @brief Will wait for any currently running jobs to finish.
 * @param workqueue_ctx  context to free
 * @returns number of entries removed  or -errno
 */
int workqueue_empty_wait(struct workqueue_ctx *ctx);


/**
* @fn workqueue_init
* @brief to be used instead of workqueue_init, to be used to install a pthread_create wrapper
* @returns number of entries removed  or -errno
*/


#ifdef  __cplusplus
}

/* C++ wrapper of C functions */
class work_queue_class
{
	private:
		struct workqueue_ctx *ctx;
	public:
		work_queue_class(unsigned int queue_size, unsigned int num_worker_threads) {
			ctx = workqueue_init(queue_size, num_worker_threads, NULL);
		}

		~work_queue_class(void) {
			workqueue_destroy(ctx);
		}

		int dequeue(int job_id) {
			return workqueue_dequeue(ctx, job_id);
		}

		int add_work(int priority, unsigned int when_milisec,
				workqueue_func_t callback_fn, void *data) {

			return workqueue_add_work(ctx,
				priority, when_milisec,	callback_fn, data);
		}

		int show_status(FILE *fp) {
			return workqueue_show_status(ctx, fp);
		}

		int get_queue_len(void) {
			return workqueue_get_queue_len(ctx);
		}

		int job_queued(int job_id) {
			return workqueue_job_queued(ctx, job_id);
		}

		int job_running(int job_id) {
			return workqueue_job_running(ctx, job_id);
		}

		int job_queued_or_running(int job_id) {
			return workqueue_job_queued_or_running(ctx, job_id);
		}

		int empty(void) {
			return workqueue_empty(ctx);
		}

		int empty_wait(void) {
			return workqueue_empty_wait(ctx);
		}
};

#endif /*cplusplus*/

#endif /*WORKQUEUE_H */
