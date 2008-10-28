#include "NvEventLog.h"
#include "config.h"
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>



#ifdef XINETD
FILE* xinetd_log;
#endif

FILE* event_log;
double cur_time;	//in seconds

#ifdef XINETD
void NvInitService()
{
    const char* user = getenv("USER");
    char* logName = NULL;
    int logNameLen = 0;
    extern int debug_flag;

    if (user == NULL) {
        logNameLen = 20+1;
        logName = (char*) calloc(logNameLen,sizeof(char));
        strncpy(logName,"/tmp/nanovis_log.txt",logNameLen);
    }
    else {
        logNameLen = 17+1+strlen(user);
        logName = (char*) calloc(logNameLen,sizeof(char));
        strncpy(logName,"/tmp/nanovis_log_",logNameLen);
        strncat(logName,user,strlen(user));
    }

    if (!debug_flag) {
	//open log and map stderr to log file
	xinetd_log = fopen(logName, "w");
	close(2);
	dup2(fileno(xinetd_log), 2);
	dup2(2,1);
	//flush junk
	fflush(stdout);
	fflush(stderr);
    }

    // clean up malloc'd memory
    if (logName != NULL) {
        free(logName);
    }
}

void NvExitService()
{
    //close log file
    if (xinetd_log != NULL) {
        fclose(xinetd_log);
        xinetd_log = NULL;
    }
}
#endif

void NvInitEventLog()
{
    event_log = fopen("event.txt", "w");
    assert(event_log!=0);

    struct timeval time;
    gettimeofday(&time, NULL);
    cur_time = time.tv_sec*1000. + time.tv_usec/1000.;
}

void NvExitEventLog()
{
    fclose(event_log);
}

double NvGetTimeInterval()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    double new_time = time.tv_sec*1000. + time.tv_usec/1000.;

    double interval = new_time - cur_time;
    cur_time = new_time;
    return interval;
}

