* libWorkQueue

This library is a priority work queue scheduler.

Possible applications of this library is prioritizing, queuing, and/or scheduling:

* Queue, parallelize, and/or schedule transactions to a synchronous database API.

* Hardware I/O to a device where commands must be queued.

* Periodic work that must be scheduled every X period for example:
	* A software watchdog to check that your system is OK.
	* A periodic check of network I/O
	* a cron job scheduler
	
* If you have many threads that spend most of the time sleeping or blocked,
	use libworkqueue to reduce the number of threads and to queue the jobs.
	The result may be a lower memory footprint by a lower number of threads.

* If you have other threads and wich to serialize operations you may use libworkqueue
	to make a FIFO or priority queue. 

* A thread pool.  Send work tasks to the pool.


**** Linux/UNIX ****
Help on Configure options 
	./autogen.sh
	./configure --help

Dependencies to compile:
	* C compiler
	* libtool, autoconf, automake (gnu auto tools)

To compile library:
	./autogen.sh
	./configure
	make

To compile examples:
	make examples

To install:
	make install

To build doxygen documentation graphvis is required:
	make doxygen-doc

* MS windows 
	Tested on Windows XP SP3 with MS Visual C++ 2008 Express Edition.  Load solution libworkqueue.sln
