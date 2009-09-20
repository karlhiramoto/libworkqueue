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

/*callback function format */
typedef void (*workqueue_func_t)(void *data);

/**
 * @fn struct workqueue_ctx* workqueue_init(unsigned int queue_size, unsigned int num_worker_threads);
 * @brief Initialize work context
 * @param queue_size size of queue.  Maximum number of jobs you can queue.
 * @param num_worker_threads  Number of threads to do the work.
 * @returns pointer to context data.  This data is private to the library and may not be used externally.   Free the context with workqueue_destroy
*/
struct workqueue_ctx* workqueue_init(unsigned int queue_size, unsigned int num_worker_threads);

/**
 * @fn workqueue_add_work
 * @brief Enqueue work
 * @param workqueue_ctx context
 * @param priority  0 is highest.
 * @param when_milisec  milliseconds in the future to schedule. 0 is now.
 * @param callback_fn  function pointer for callback
 * @param data  data to pass to work callback function. 
 * @returns Job ID or  -errno; -EBUSY when queues are full
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
 * @fn workqueue_destroy
 * @brief free context, releases all memory.  Any jobs in queue are dequed.
 * @brief Will wait for any currently running jobs to finish.
 * @param workqueue_ctx  context to free
 * @returns void
 */
void workqueue_destroy(struct workqueue_ctx *ctx);

#ifdef  __cplusplus
}
#endif /*cplusplus*/

#endif /*WORKQUEUE_H */
