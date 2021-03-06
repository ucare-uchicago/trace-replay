#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstdarg>

/* ===========================================================================
 * Logger.h is a log recorder, put all completed IO information to log file
 * ===========================================================================*/

namespace ucare {

/** A simple logger. */
class Logger {
public:
	/** Open log to stdout. */
	Logger() : file(stdout) {}

	/** Open log to the given file. */
	Logger(std::string filename) : file(NULL) {}

	/** Redirect log to the given file. */
	void redirect(std::string filename) {
		if (file != stdout)
			fclose(file); // close previous file

		file = fopen(filename.c_str(), "w");
		if (file == NULL) {
			fprintf(stderr, "error: opening file %s: %m!\n", filename.c_str());
			throw std::runtime_error("error: opening file");
		}
	}

	/** Close log. */
	~Logger() { fclose(file); }

	/** Output log. */
	inline void printf(const char * format, ...) {
		va_list args;
		va_start(args, format);
		vfprintf(file, format, args);
		//vprintf(format, args);        
		//fflush(file);
		va_end(args);	 
	}

	/** Cast to FILE*. */
	operator FILE* () {
		return file;
	}

private:
	FILE *file; /// Log file
}; // class Logger

} // namespace ucare

