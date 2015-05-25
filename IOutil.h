#include "TraceReader.h"
#include "Timer.h"
#include <aio.h>
#include "Logger.h"

/* ===========================================================================
 * IOutil.h is IO toolkit, I put all posix aio related method inside, including initialize,
 * IOsubmit and callback
 * ===========================================================================*/

using namespace ucare;

namespace ucare {

#define MEM_ALIGN             512  // Memory alignment
#define USE_GLOBAL_BUFF	true	//if you don't want to use global buffer, simply change to false

static int submitCount = 0; // count IO submitted

static int completeCount = 0;

static Logger logger;

// Add our objects into aiocb
struct AioCB : public aiocb {
	TraceEvent event; // The IoEvent performed for this callback
	Timer::Timepoint beginTime; // The time right before we submit the IO
	int submitCount; // The submit count right before we submit the IO
	int completeCount; // The complete count right before we submit the IO
}; // struct AioCB


// Callback when IO's completed
static void IOCompleted(sigval_t sigval) {
	//cbtracker.insert(pthread_self());

	auto request = (AioCB *)sigval.sival_ptr;
	long latency = Timer::elapsedTimeSince(request->beginTime); // time completed

	++completeCount; // keep track number of request completed
	
	int error = aio_error(request);
	if (error) {
		if(error != ECANCELED)
			fprintf(stderr, "Error completing i/o:%d\n", error);
		//delete request;
		return;
	}
	
	int count = aio_return(request);
	if (count < (int)request->aio_nbytes) { // does this happen with device? 
		fprintf(stderr, "Warning I/O completed:%db but requested:%ldb\n", 
			count, request->aio_nbytes);
	}

	// log result (note: we can use shared log now because the IO's done)
	TraceEvent const& io = request->event;
	fprintf(logger, "%ld,%ld,%ld,%d,%ld,%lf,%d,%d,%d,%d\n", 
		(size_t)io.time, io.blkno, io.bcount, io.flags, latency, (double)io.size/latency,
		submitCount, completeCount, request->submitCount, request->completeCount);
}


// Perform IO in async manner
static AioCB * performAIO(int fd, void* buf, TraceEvent const& io) {
	// create and initialize cb with 0! 
	auto cb = new AioCB(); // keep cb alive till callback's done!
	
	cb->aio_fildes = fd;
	cb->aio_nbytes = io.size;
	cb->aio_offset = io.offset;

	cb->aio_sigevent.sigev_notify = SIGEV_THREAD;
	cb->aio_sigevent.sigev_notify_function = IOCompleted;
	cb->aio_sigevent.sigev_value.sival_ptr = cb;

	//write and read different buffer
	if(!USE_GLOBAL_BUFF){
		char *buf_new;
		if (posix_memalign((void**)&buf_new, MEM_ALIGN, io.size)) {
			fprintf(stderr, "Error allocating buffer\n");
		}
		cb->aio_buf = buf_new;
	}else{
		cb->aio_buf = buf;
	}

	cb->event = io;	
	cb->submitCount = submitCount;
	cb->completeCount = completeCount;
	cb->beginTime = Timer::now(); // mark the time right before we submit IO
	
	// submit IO
	int error = 0;
	if (io.flags == 0) // output
		error = aio_write(cb);
	else if (io.flags == 1) // input
		error = aio_read(cb);

	if (error) {
		fprintf(stderr, "Error performing i/o: %m time:%ld size:%ld offset:%ld\n", 
			(size_t)io.time, cb->aio_nbytes, cb->aio_offset);
		return NULL;
	}

	++submitCount; // keep track number of request submitted

	return cb;
}

static void replayer_aio_init(){
	aioinit aioParam = {0};

	//two thread for each device is better
	aioParam.aio_threads = 20;
	aioParam.aio_num = 2048;
	aioParam.aio_idle_time = 1;	
	aio_init(&aioParam);
}
}	//end namespace
