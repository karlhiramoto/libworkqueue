/**
* @file Workqueue.h
* @author Karl Hiramoto <karl@hiramoto.org>
* @brief public include header for priority work queues..
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


struct workqueue_ctx;

typedef void (*workqueue_func_t)(void *data);

struct workqueue_ctx* workqueue_init(unsigned int queue_size, unsigned int num_worker_threads);

/** workqueue_add_work
 * @brief Enqueue work
 * @param workqueue_ctx context
 * @returns Job ID or  -errno;
 */
int workqueue_add_work(struct workqueue_ctx* ctx,
		int priority, unsigned int when_milisec,
		workqueue_func_t callback_fn, void *data);


int workqueue_show_status(struct workqueue_ctx* ctx, FILE *fp);

/** workqueue_get_queue_len
 * @brief Get the current queue lenght
 * @param workqueue_ctx  context
 * @returns Number of jobs currently waiting in queue.
 *          Does not count job if it's currently execuing. 
 */
int workqueue_get_queue_len(struct workqueue_ctx* ctx);


/**
 * @brief free context
 * @param workqueue_ctx  context
 * @returns void
 */
void workqueue_destroy(struct workqueue_ctx *ctx);

#ifdef  __cplusplus
}
#endif /*cplusplus*/

#endif /*WORKQUEUE_H */
