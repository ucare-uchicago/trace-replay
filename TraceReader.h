
#include <cstdio>
#include <stdexcept>
#include <cstdlib>
#include <string.h>

/* ===========================================================================
 * TraceReader.h is a simple reader, take trace event from trace file and store in 
 * TraceEvent struct, TraceEvent is reused in Replayer to save memory
 * ===========================================================================*/
/** An I/O trace event. */
namespace ucare {

struct TraceEvent {
	double time;
	size_t bcount; // count in block
	size_t size; // count in bytes
	size_t blkno; // offset in block
	size_t offset; // offset in bytes
	int    flags;
}; // struct TraceEvent

/** A simple trace reader. */
class TraceReader {
public:
	/** Open trace. */
	TraceReader(const char *filename) {
		input = fopen(filename, "r");
		if (input == NULL) {
			fprintf(stderr, "error: opening file %s: %m!\n", filename);
			throw std::runtime_error("error: opening file");
		}
		isFirstLine = true;
		baseTime = 0;
	}

	/** Close trace. */
	~TraceReader() { fclose(input); }

	/** Read trace. */
    bool read(TraceEvent& event) {
    	char line[50];
    	char *tok;
    	int field_num;
    	if (fgets(line, 50, input) == NULL)
      		return 0;
    	// first ignored field is disk num; second is IO type (normal, sheltered, cleanup)
    	for (tok = strtok(line, " "), field_num=0; tok!=NULL; tok = strtok(NULL, " "), ++field_num) {
      		switch (field_num) {
        		case 0:
			if(isFirstLine == true){
				isFirstLine = false;
				baseTime = strtod(tok, NULL);
				event.time = 0;
			}else
          			event.time = strtod(tok, NULL) - baseTime;
          		break;
        		case 2:
          		event.blkno = strtol(tok, NULL, 10);
          		break;
        		case 3:
          		event.bcount = strtol(tok, NULL, 10);
          		break;
        		case 4:
          		event.flags = strtol(tok, NULL, 10); // this is just integer
          		break;
        		default:
          		// ignored
          		break;
      		}
    	}
    	//printf("%lf %ld %ld %d\n", event.time, event.blkno, event.bcount, event.flags);
    	return 1;
	}

private:
	FILE *input; /// Trace file
	bool isFirstLine;
	double baseTime;
}; // class TraceReader
} // namespace ucare
