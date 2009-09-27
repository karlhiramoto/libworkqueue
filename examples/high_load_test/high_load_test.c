/**
* @file high test example
* @author Karl Hiramoto <karl@hiramoto.org>
* @brief high load with lots of threads and lots of jobs
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

#define ENOMEM ERROR_NOT_ENOUGH_MEMORY
#define EBUSY ERROR_BUSY


#define sleep(x) Sleep(x*1000)
#else
#include <unistd.h>
#include <errno.h>

#endif

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
	int i;

	for (i = 0; i < 123456789; i++) {
		ret += i;
	}
	if (ret)
		prg->counter++;
}

int main(int argc, char *argv[]) {
	struct prg_ctx prg;
	int i;
	int num_jobs=512;
	int ret;
	printf("starting\n");
	prg.counter = 0;
	prg.ctx = workqueue_init(512	, 32);

	for (i = 0; i < num_jobs; i++) {
		ret = workqueue_add_work(prg.ctx, 2, 0,
			callback_func, &prg);

		if (ret >= 0) {
			/* Added job ok */
		} else if (ret == -EBUSY){
			printf("busy adding work %d of %d\n", i, num_jobs);
			sleep(1);
		} else {
			printf("Error adding job err=%d\n", ret);
			workqueue_show_status(prg.ctx, stdout);
		}
	}

	for (i = 0; i < num_jobs/2; i++) {
		ret = workqueue_add_work(prg.ctx, 5, 0,
			callback_func, &prg);

		if (ret >= 0) {
			/* Added job ok */
		} else if (ret == -EBUSY){
			printf("busy adding work %d of %d\n", i, num_jobs);
			sleep(1);
		} else {
			printf("Error adding job err=%d\n", ret);
			workqueue_show_status(prg.ctx, stdout);
		}
	}
	workqueue_show_status(prg.ctx, stdout);

	for (i = 0; i < num_jobs/2; i++) {
		ret = workqueue_add_work(prg.ctx, 1, 0,
			callback_func, &prg);

		if (ret >= 0) {
			/* Added job ok */
		} else if (ret == -EBUSY){
			printf("busy adding work %d of %d\n", i, num_jobs);
			sleep(1);
		} else {
			printf("Error adding job err=%d\n", ret);
			workqueue_show_status(prg.ctx, stdout);
		}
	}
	workqueue_show_status(prg.ctx, stdout);

	for (i = 20; i && (ret = workqueue_get_queue_len(prg.ctx)); i--) {
	  	printf("waiting for %d jobs \n", ret);
		sleep(1);
	}
	
	workqueue_destroy(prg.ctx);
	printf("count =%d \n", prg.counter);
	#ifdef WINDOWS
	system("pause");
	#endif
	return 0;
}