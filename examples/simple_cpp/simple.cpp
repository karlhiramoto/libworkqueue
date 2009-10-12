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
#endif

#include "workqueue.h"


class prg_ctx
{
	public:
	work_queue_class *wq;
	int counter;
	prg_ctx(unsigned int queue_size, unsigned int num_worker_threads) {

		wq = new work_queue_class(queue_size, num_worker_threads);
		counter = 0;
	}
	~prg_ctx(void) {
		delete wq;
	}
};


static void callback_func(void *data)
{
	struct prg_ctx *prg = (struct prg_ctx *) data;
	int ret;

	prg->counter++;
	printf("counter=%d\n", prg->counter);
	if (prg->counter % 2)
		sleep(1);

	if ((prg->counter % 4) == 0) {
		printf("reschedule this job\n");
		ret = prg->wq->add_work(2, 1234,
			callback_func, prg);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
			prg->wq->show_status(stdout);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
}

int main(int argc, char *argv[]) {
	struct prg_ctx prg(32,1);
	int i;
	int num_jobs=6;
	int ret;
	printf("starting\n");

	for (i = 0; i < num_jobs; i++) {
		ret = prg.wq->add_work(2, 0,
			callback_func, &prg);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
	prg.wq->show_status(stdout);

	for (i = 0; i < num_jobs; i++) {
		printf("job %d is queued=%d running=%d queued_or_running=%d\n", i,
			prg.wq->job_queued(i),
			prg.wq->job_running(i),
			prg.wq->job_queued_or_running(i));
	}

	for (i = 0; i < num_jobs/2; i++) {
		ret = prg.wq->add_work(5, 0,
			callback_func, &prg);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
	prg.wq->show_status(stdout);

	for (i = 0; i < num_jobs/2; i++) {
		ret = prg.wq->add_work(1, 0,
			callback_func, &prg);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
	}
	prg.wq->show_status(stdout);

	for (i = 20; i && (ret = prg.wq->get_queue_len()); i--) {
	  	printf("waiting for %d jobs \n", ret);
		sleep(1);
	}

#ifdef WINDOWS
	system("pause");
#endif
	return 0;
}