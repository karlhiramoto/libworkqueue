#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "workqueue.h"

void callback_func(void *data)
{
	int *counter = (int*) data;
	printf("counter=%d\n",*counter);
	sleep(1);
}

int main(int argc, char *argv[]) {
	struct workqueue_ctx *ctx;
	int counter = 0;
	int i;
	int num_jobs=5;
	int ret;
// 	char buf[128];
	printf("starting\n");
	ctx = workqueue_init(32, 1);

	for (i = 0; i < num_jobs; i++) {
		ret = workqueue_add_work(ctx,1, 0,
			callback_func, &counter);

		if (ret >= 0) {
			printf("Added job %d \n", ret);
		} else {
		  	printf("Error adding job err=%d\n", ret);
		}
		

	}
	workqueue_show_status(ctx, stdout);

	for (i = 10; i && (ret = workqueue_get_queue_len(ctx)); i--) {
	  	printf("waiting for %d jobs \n", ret);
		sleep(1);
	}

	workqueue_destroy(ctx);

	return 0;
}