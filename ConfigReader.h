#include <cstdio>
#include <stdexcept>
#include <cstdlib>
#include <string.h>

/* ===========================================================================
 * ConfigReader.h is a simple reader program, read data in Config.txt, so no need to hardcode
 * any config file inside Replayer.cpp
 * ===========================================================================*/

namespace ucare{

struct Config {
	char device[64];
	char traceFileName[64];	
	int device_num;
	char logFileName[64];
}; // struct TraceEvent

static Config config_read(const char *filename) {
	FILE *config_file;
	config_file = fopen(filename, "r");
	if (config_file == NULL) {
		fprintf(stderr, "error: opening file %s: %m!\n", filename);
		throw std::runtime_error("error: opening file");
	}

	Config config;
	char line[50];
	char *token;
	int field_num;

	int line_num = 1;
	int index_inline = 0;
	int device_num = 0;
 	while (fgets(line, 50, config_file) != NULL){
		token = strtok(line, " ");	//get the 1st word in line
		token = strtok(NULL, " ");	//starting with 2nd word, which is content

		switch(line_num){
			case 1:
				while( token != NULL ) {
					int len = strlen(token);
					if( token[len-1] == '\n' )
    					token[len-1] = 0;
					sprintf(config.device, "%s", token);
					device_num++;
					config.device_num = device_num;
					token = strtok(NULL, " ");
				}
				line_num++;
			break;
			case 2:
				while( token != NULL ) {
					int len = strlen(token);
					if( token[len-1] == '\n' )
    					token[len-1] = 0;
					sprintf(config.traceFileName, "%s", token);
					token = strtok(NULL, " ");
				}
				line_num++;
			break;
			case 3:
				while( token != NULL ) {
					int len = strlen(token);
					if( token[len-1] == '\n' )
    					token[len-1] = 0;
					sprintf(config.logFileName, "%s", token);
					token = strtok(NULL, " ");
				}
				line_num++;
			default:
          		// ignored
          		break;
		}
	}
	return config;
}


}	//end namespace
