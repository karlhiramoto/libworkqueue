/**
* @file simple test example
* @author Karl Hiramoto <karl@hiramoto.org>
* @brief simple example showing how to enqueue worker jobs
*
* Example code is Distributed under Dual BSD / LGPL licence
* Copyright 2009 Karl Hiramoto
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "workqueue.h"


struct prg_ctx
{
	struct workqueue_ctx *ctx;
	int counter;
};

void callback_func(void *data)
{
	struct prg_ctx *prg = (struct prg_ctx *) data;
	int ret;

	prg->counter++;
	printf("counter=%d\n", prg->counter);
	if (prg->counter % 2)
		sleep(1);

	if ((prg->counter % 4) == 0) {
		printf("reschedule this job\n");
		ret = workqueue_add_work(prg->ctx, 2, 1234,
			callback_func, prg);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
			workqueue_show_status(prg->ctx, stdout);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
}

int main(int argc, char *argv[]) {
	struct prg_ctx prg = { .counter = 0};
	int i;
	int num_jobs=5;
	int ret;
	printf("starting\n");
	prg.ctx = workqueue_init(32, 1);

	for (i = 0; i < num_jobs; i++) {
		ret = workqueue_add_work(prg.ctx, 2, 0,
			callback_func, &prg);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
	workqueue_show_status(prg.ctx, stdout);

	for (i = 0; i < num_jobs/2; i++) {
		ret = workqueue_add_work(prg.ctx, 5, 0,
			callback_func, &prg);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
	workqueue_show_status(prg.ctx, stdout);

	for (i = 0; i < num_jobs/2; i++) {
		ret = workqueue_add_work(prg.ctx, 1, 0,
			callback_func, &prg);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
	workqueue_show_status(prg.ctx, stdout);

	for (i = 20; i && (ret = workqueue_get_queue_len(prg.ctx)); i--) {
	  	printf("waiting for %d jobs \n", ret);
		sleep(1);
	}

	workqueue_destroy(prg.ctx);

	return 0;
}