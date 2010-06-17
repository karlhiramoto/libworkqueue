/**
* @file simple test example
* @author Karl Hiramoto <karl@hiramoto.org>
* @brief simple example showing how to doing a polling job that reschedules
* @brief itself to do something every X ammount of time.
*
* Example code is Distributed under Dual BSD / LGPL licence
* Copyright 2009 Karl Hiramoto
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include "workqueue.h"
#include "pthread_ex.h"
#include "ex.h"

static struct workqueue_ctx *ctx = NULL;

static struct worker_thread_ops ops = {
	.worker_constructor = ex_thread_init,
	.data = NULL,
};

void callback_func(void *data)
{
	int *counter = (int*) data;
	int ret = 0;
	ex_t ex;
	
	ex_try {
		(*counter)++;
		printf("starting callback\n");


		printf("counter=%d job ret=%d\n",*counter, ret);
		/* NOTE This kind of function to do polling every X ammount of time */

		/* reschedule myself */
		if (*counter < 80) {
			ret = workqueue_add_work(ctx, 2, 1678,
				callback_func, counter);
		} else {
			ex_throw(NULL, callback_func, data);
		}

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
			printf("Error adding job err=%d\n", ret);
		}
	} ex_catch(ex) {
		printf("caught exception at %s:%d", ex.ex_func, ex.ex_line);
		ex_rethrow;
	}
	
}

int main(int argc, char *argv[]) {
	
	int counter = 0;
	int i;
	int ret;
	printf("starting\n");
	pthread_init_ex();
	ctx = workqueue_init(32, 1, &ops);

	ret = workqueue_add_work(ctx, 2, 12000,
		callback_func, &counter);

  	printf("waiting for %d jobs \n", ret);
	workqueue_show_status(ctx, stdout);
	printf("done show status \n");
	sleep(30);

	for (i = 20; i && (ret = workqueue_get_queue_len(ctx)); i--) {
	  	printf("waiting for %d jobs \n", ret);
		sleep(1);
	}

	workqueue_destroy(ctx);

	return 0;
}