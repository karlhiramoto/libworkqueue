#ifndef WORKQUEUE_H
#define WORKQUEUE_H 1

#ifdef  __cplusplus
extern "C" {
#endif


#include <time.h>


struct workqueue_ctx;

typedef void (*workqueue_func_t)(void *data);

struct workqueue_ctx* workqueue_init(unsigned int queue_size, unsigned int num_worker_threads);


int workqueue_add_work(struct workqueue_ctx* ctx,
		int priority, int when_milisec,
		workqueue_func_t callback_fn, void *data);

int workqueue_show_status(struct workqueue_ctx* ctx, FILE *fp);

int workqueue_get_queue_len(struct workqueue_ctx* ctx);

void workqueue_destroy(struct workqueue_ctx *ctx);

#ifdef  __cplusplus
}
#endif

#endif