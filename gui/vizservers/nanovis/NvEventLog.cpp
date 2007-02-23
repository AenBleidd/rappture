#include "NvEventLog.h"
#include "config.h"
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>



#ifdef XINETD
FILE* xinetd_log;
#endif

FILE* event_log;
double cur_time;	//in seconds

#ifdef XINETD
void NvInitService()
{
    //open log and map stderr to log file
    xinetd_log = fopen("/tmp/log.txt", "w");
    close(2);
    dup2(fileno(xinetd_log), 2);
    dup2(2,1);

    //flush junk
    fflush(stdout);
    fflush(stderr);
}

void NvExitService()
{
    //close log file
    fclose(xinetd_log);
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

