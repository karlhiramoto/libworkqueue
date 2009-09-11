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

void callback_func(void *data)
{
	int *counter = (int*) data;
	(*counter)++;
	printf("counter=%d\n",*counter);
	sleep(1);
}

int main(int argc, char *argv[]) {
	struct workqueue_ctx *ctx;
	int counter = 0;
	int i;
	int num_jobs=5;
	int ret;
	printf("starting\n");
	ctx = workqueue_init(32, 1);

	for (i = 0; i < num_jobs; i++) {
		ret = workqueue_add_work(ctx, 2, 0,
			callback_func, &counter);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
	workqueue_show_status(ctx, stdout);

	for (i = 0; i < num_jobs/2; i++) {
		ret = workqueue_add_work(ctx, 5, 0,
			callback_func, &counter);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
	workqueue_show_status(ctx, stdout);

	for (i = 0; i < num_jobs/2; i++) {
		ret = workqueue_add_work(ctx, 1, 0,
			callback_func, &counter);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
	workqueue_show_status(ctx, stdout);

	for (i = 20; i && (ret = workqueue_get_queue_len(ctx)); i--) {
	  	printf("waiting for %d jobs \n", ret);
		sleep(1);
	}

	workqueue_destroy(ctx);

	return 0;
}