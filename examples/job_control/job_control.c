/**
* @file job_control.c
* @author Karl Hiramoto <karl@hiramoto.org>
* @brief simple example showing how to check if job is running, dequeue, cancel or kill it.
*
* Example code is Distributed under Dual BSD / LGPL licence
* Copyright 2009 Karl Hiramoto
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

#if !defined(WINDOWS)
#define WINDOWS
#endif

#pragma once
#include "windows.h"            // big fat windows lib
#define sleep(x) Sleep(x*1000)
#else
#include <unistd.h>
#include <signal.h>
#endif

#include "workqueue.h"


struct prg_ctx
{
	struct workqueue_ctx *ctx;
	int counter;
};


static void callback_func(void *data)
{
	struct prg_ctx *prg = (struct prg_ctx *) data;
	int ret;

	prg->counter++;
	printf("callback counter=%d\n", prg->counter);
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
	printf("work callback returning\n");
}

void sighandler_func(int sig)
{
	printf("signal=%d recieved\n",sig);
}


int main(int argc, char *argv[]) {
	struct prg_ctx prg;
	int i;
	int num_jobs = 3;
	int ret;
	printf("starting\n");

	signal(SIGTERM, sighandler_func);

	prg.counter = 0;
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

	for (i = 0; i < num_jobs; i++) {
		printf("job %d is queued=%d running=%d queued_or_running=%d\n", i,
			workqueue_job_queued(prg.ctx, i),
			workqueue_job_running(prg.ctx, i),
			workqueue_job_queued_or_running(prg.ctx, i));
	}

	workqueue_show_status(prg.ctx, stdout);

	for (i = 0; i < num_jobs; i++) {
		ret = workqueue_dequeue(prg.ctx, i),
		printf(" dequeue job %d  ret=%d\n", i , ret);
	}
	workqueue_show_status(prg.ctx, stdout);

	for (i = 0; i < num_jobs; i++) {
		ret = workqueue_add_work(prg.ctx, 2, 0,
			callback_func, &prg);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}

	for (i = 0; i < num_jobs*2; i++) {
		ret = workqueue_dequeue(prg.ctx, i),
		printf(" dequeue job %d  ret=%d\n", i , ret);
	}


	for (i = 20; i && (ret = workqueue_get_queue_len(prg.ctx)); i--) {
	  	printf("waiting for %d jobs \n", ret);
		sleep(1);
	}

	workqueue_destroy(prg.ctx);

#ifdef WINDOWS
	system("pause");
#endif
	return 0;
}