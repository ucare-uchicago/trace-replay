#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <iostream>
#include <queue>
#include <cstdio>
#include "IOutil.h"
#include <aio.h>
#include "ConfigReader.h"

using namespace std;
using namespace chrono;
using namespace ucare;

/* ===========================================================================
 * Replayer.cpp is just a simple centralized trace replayer, take trace event from after
 * preprocessed trace file, and submit IO request, record in log
 * ===========================================================================*/

//#define MEM_ALIGN             512  // Memory alignment
#define BYTE_PER_BLOCK 512 
#define LARGEST_REQUEST_SIZE 10000  // Largest request size in blocks
#define BLOCK_PER_DRIVE	3800000	//2GB blocks number


 int main(int argc, char *argv[])
 {
	//read config file, put all information into struct config
	Config config;
	config = config_read("Config.txt");

	//reading trace
	TraceReader trace(config.traceFileName); 

	//initialize message queue
	queue<TraceEvent> request_queue;

	//read trace
	printf("Start reading trace\n");
	TraceEvent event;
	while (trace.read(event)) {
		event.time = event.time * 1000; // to microseconds
		event.size = event.bcount * BYTE_PER_BLOCK;
		event.offset = (event.blkno % BLOCK_PER_DRIVE) * BYTE_PER_BLOCK;
		request_queue.push(event);
	}
	
	// initialize device

	int fd = open(config.device, O_DIRECT | O_SYNC | O_RDWR); 
	if (fd < 0) {
		fprintf(stderr, "Value of errno: %d\n", errno);
        	printf("Cannot open\n");
        	exit(1);
	}


	//init log file
	logger.redirect(config.logFileName);



	// this is shared write buffer, read buffer is per read request thread inside function
	char *buf;
	if (posix_memalign((void**)&buf, MEM_ALIGN, LARGEST_REQUEST_SIZE * BYTE_PER_BLOCK)) {
		fprintf(stderr, "Error allocating buffer\n");
		return 1;
	}

	//generate  data
	int j;
        for(j=0;j<LARGEST_REQUEST_SIZE * BYTE_PER_BLOCK;j++){
            //Generate random alphabets to write to file
            buf[j]=(char)(rand()%26+65);
        }


	replayer_aio_init();

	//play IO
	Timer timeBegin;	//start time

	Timer timer;
	while(!request_queue.empty()){	
		TraceEvent event =  request_queue.front();
		request_queue.pop();
		long currentTime = timer.elapsedTime();
		long nextIoTime = event.time;
		while (currentTime < nextIoTime) { // we're early
			currentTime = timer.elapsedTime(); // busy-waiting
		}
		cout << "current time: " << currentTime <<endl;
		cout << "event time: " << event.time <<endl;
		performAIO(fd,buf, event);
	}

	printf("All IO submitted after: %ld s\n", timeBegin.elapsedTime<seconds>());

	while (completeCount != submitCount /*&& numWait++ < 300*/) {
		printf("I/O completed:%d but submitted:%d\n", 
			completeCount, submitCount);
		sleep(3);
	}
	if (completeCount != submitCount)
		fprintf(stderr, "Warning I/O completed:%d but submitted:%d\n", 
			completeCount, submitCount);

	printf("All IO completed after: %ld s\n", timeBegin.elapsedTime<seconds>());

	printf("Done\n");
	return 0;

}

