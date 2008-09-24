
/*
 * ----------------------------------------------------------------------
 * proxypymol.c
 *
 *      This module creates the Tcl interface to the pymol server.  It acts as
 *      a go-between establishing communication between a molvisviewer widget
 *      and the pymol server. The communication protocol from the molvisviewer
 *      widget is the Tcl language.  Commands are then relayed to the pymol
 *      server.  Responses from the pymol server are translated into Tcl
 *      commands * and send to the molvisviewer widget. For example, resulting
 *      image rendered offscreen is returned as BMP-formatted image data.
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/times.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <getopt.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <tcl.h>

#undef INLINE
#ifdef __GNUC__
#  define INLINE __inline__
#else
#  define INLINE
#endif

#define FALSE 0
#define TRUE  1

#define IO_TIMEOUT (30000)
#define KEEPSTATS	1
#define CVT2SECS(x)  ((double)(x).tv_sec) + ((double)(x).tv_usec * 1.0e-6)

typedef struct {
    pid_t child;		/* Child process. */
    size_t nFrames;		/* # of frames sent to client. */
    size_t nBytes;		/* # of bytes for all frames. */
    size_t nCommands;		/* # of commands executed */
    double cmdTime;		/* Elasped time spend executing commands. */
    struct timeval start;	/* Start of elapsed time. */
} Stats;

static Stats stats;

static FILE *flog;
static int debug = 1;


typedef struct {
    char *data;
    int   used;
    int   allocated;
} DyBuffer;

typedef struct {
    int p_stdin;
    int p_stdout;
    int p_stderr;
    int c_stdin;
    int c_stdout;
    DyBuffer image;
    int need_update;
    int can_update;
    int immediate_update;
    int sync;
    int labels;
    int frame;
    int rock_offset;
    int cacheid;
    int invalidate_cache;
    int error;
    int status;
} PymolProxy;

static void
trace TCL_VARARGS_DEF(char *, arg1)
{
    if (debug) {
        char *format;
        va_list args;

        format = TCL_VARARGS_START(char *, arg1, args);
        vfprintf(flog, format, args);
        fprintf(flog, "\n");
        fflush(flog);
    }
}

#if KEEPSTATS

static int
WriteStats(const char *who, int code) 
{
    double start, finish;
    pid_t pid;
    char buf[BUFSIZ];
    Tcl_DString ds;

    {
	struct timeval tv;

	/* Get ending time.  */
	gettimeofday(&tv, NULL);
	finish = CVT2SECS(tv);
	tv = stats.start;
	start = CVT2SECS(tv);
    }
    /* 
     * Session information:
     *   1. Start date of session in seconds.
     *   2. Process ID
     *	 3. Number of frames returned.
     *	 4. Number of bytes total returned (in frames).
     *	 5. Total elapsed time of all commands.
     *   6. Total elapsed time of session.
     *	 7. Exit code of pymol server.
     *   8. User time.  
     *	 9. System time.
     *  10. Maximum resident size.
     */ 
    pid = getpid();
    Tcl_DStringInit(&ds);
    
    sprintf(buf, "<session server=\"%s\" ", who);
    Tcl_DStringAppend(&ds, buf, -1);

    strcpy(buf, ctime(&stats.start.tv_sec));

    buf[strlen(buf) - 1] = '\0';
    Tcl_DStringAppend(&ds, "date=\"", -1);
    Tcl_DStringAppend(&ds, buf, -1);
    Tcl_DStringAppend(&ds, "\" ", -1);

    sprintf(buf, "date_secs=\"%ld\" ", stats.start.tv_sec);
    Tcl_DStringAppend(&ds, buf, -1);

    sprintf(buf, "pid=\"%d\" ", pid);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "num_frames=\"%lu\" ", (unsigned long int)stats.nFrames);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "frame_bytes=\"%lu\" ", (unsigned long int)stats.nBytes);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "num_commands=\"%lu\" ", (unsigned long int)stats.nCommands);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "cmd_time=\"%g\" ", stats.cmdTime);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "session_time=\"%g\" ", finish - start);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "status=\"%d\" ", code);
    Tcl_DStringAppend(&ds, buf, -1);
    {
	long clocksPerSec = sysconf(_SC_CLK_TCK);
	double clockRes = 1.0 / clocksPerSec;
	struct tms tms;

	memset(&tms, 0, sizeof(tms));
	times(&tms);
	sprintf(buf, "utime=\"%g\" ", tms.tms_utime * clockRes);
	Tcl_DStringAppend(&ds, buf, -1);
	sprintf(buf, "stime=\"%g\" ", tms.tms_stime * clockRes);
	Tcl_DStringAppend(&ds, buf, -1);
	sprintf(buf, "cutime=\"%g\" ", tms.tms_cutime * clockRes);
	Tcl_DStringAppend(&ds, buf, -1);
	sprintf(buf, "cstime=\"%g\" ", tms.tms_cstime * clockRes);
	Tcl_DStringAppend(&ds, buf, -1);
    }
    Tcl_DStringAppend(&ds, "/>\n", -1);

    {
	int f;
	ssize_t length;
	int result;

#define STATSDIR	"/var/tmp/visservers"
#define STATSFILE	STATSDIR "/" "data.xml"
	if (access(STATSDIR, X_OK) != 0) {
	    mkdir(STATSDIR, 0770);
	}
	length = Tcl_DStringLength(&ds);
	f = open(STATSFILE, O_APPEND | O_CREAT | O_WRONLY, 0600);
	result = FALSE;
	if (f < 0) {
	    goto error;
	}
	if (write(f, Tcl_DStringValue(&ds), length) != length) {
	    goto error;
	}
	result = TRUE;
 error:
	if (f >= 0) {
	    close(f);
	}
	Tcl_DStringFree(&ds);
	return result;
    }
}
#endif

static void
DoExit(int code)
{
    char fileName[200];
#if KEEPSTATS
    WriteStats("pymolproxy", code);
#endif
    sprintf(fileName, "/tmp/pymol%d.pdb", getpid());
    unlink(fileName);
    exit(code);
}

static int
ExecuteCommand(Tcl_Interp *interp, Tcl_DString *dsPtr) 
{
    struct timeval tv;
    double start, finish;
    int result;

    gettimeofday(&tv, NULL);
    start = CVT2SECS(tv);

    result = Tcl_Eval(interp, Tcl_DStringValue(dsPtr));
    trace("Executed (%s)", Tcl_DStringValue(dsPtr));
    Tcl_DStringSetLength(dsPtr, 0);

    gettimeofday(&tv, NULL);
    finish = CVT2SECS(tv);

    stats.cmdTime += finish - start;
    stats.nCommands++;
    return result;
}


INLINE static void
dyBufferInit(DyBuffer *buffer)
{
    buffer->data = NULL;
    buffer->used = 0;
    buffer->allocated = 0;
}

INLINE static void
dyBufferFree(DyBuffer *buffer)
{
    assert(buffer != NULL);
    free(buffer->data);
    dyBufferInit(buffer);
}

void
dyBufferSetLength(DyBuffer *buffer, int length)
{
    assert(buffer != NULL);
    if (length == 0) {
        dyBufferFree(buffer);
    } else if (length > buffer->used) {
        char *newdata;
        
        newdata = realloc(buffer->data, length);
        if (newdata != NULL) {
            buffer->data = newdata;
            buffer->used = length;
            buffer->allocated = length;
        }
    } else {
        buffer->used = length;
    }
}

static void
dyBufferAppend(DyBuffer *buffer, const char *data, int length)
{
    int offset;

    assert(buffer != NULL);
    offset = buffer->used;
    dyBufferSetLength(buffer, offset + length);
    memcpy(buffer->data + offset, data, length);
}

static int
bwrite(int sock, char *buffer, int size)
{
    int result;
    int total = 0;
    int left = size;

    trace("bwrite: want to write %d bytes\n", size);
    while(1) {
        result = write(sock,buffer+total,left);

        if (result <= 0)
            break;

        total += result;
        left -= result;

        if (total == size)
            break;
    }
    trace("bwrite: wrote %d bytes\n", total);
    return(total);
}

static int
bread(int sock, char *buffer, int size)
{
    int result, total, left;

    for( total = 0, left = size; left > 0; left -= result) {
	result = read(sock,buffer+total,left);
	
	if (result > 0) {
	    total += result;
	    continue;
	}
        
	if ((result < 0) && (errno != EAGAIN) && (errno != EINTR)) { 
	    trace("pymolproxy: Error reading sock(%d), %d/%s\n", 
		  sock, errno,strerror(errno));
	    break;
	}
	
	result = 0;
    }
    return total;
}

#ifdef notdef

static int 
bflush(int sock, char *buffer, int size, int bytes)
{
    int bsize;

    while(bytes) {
	if (bytes > size)
	    bsize = size;
	else
	    bsize = bytes;
	
	bsize = bread(sock,buffer,bsize);
        
	bytes -= bsize;     
    }
}

#undef timersub
static void
timersub(struct timeval *a, struct timeval *b, struct timeval *result)
{
    result->tv_sec = a->tv_sec - b->tv_sec;
    result->tv_usec = a->tv_usec - b->tv_usec;

    while(result->tv_usec < 0) {
        result->tv_sec -= 1;
        result->tv_usec += 1000000;
    }
}

#endif

static void
timersub_ms(struct timeval *a, struct timeval *b, int *result)
{
    struct timeval tmp;

    tmp.tv_sec = a->tv_sec - b->tv_sec;
    tmp.tv_usec = a->tv_usec - b->tv_usec;

    while(tmp.tv_usec < 0) {
        tmp.tv_sec -= 1;
        tmp.tv_usec += 1000000;
    }

    *result = (tmp.tv_sec * 1000) + (tmp.tv_usec / 1000);
}

#undef timeradd
static void
timeradd (struct timeval *a, struct timeval *b, struct timeval *result)
{
    result->tv_sec = a->tv_sec + b->tv_sec;
    result->tv_usec = a->tv_usec + b->tv_usec;

    while (result->tv_usec >= 1000000) {
        result->tv_sec += 1;
        result->tv_usec -= 1000000;
    }
}

static void
timerset_ms(struct timeval *result, int timeout)
{
    result->tv_sec = timeout / 1000;
    result->tv_usec = (timeout % 1000) * 1000;
}

static int
getline(int sock, char *buffer, int size, long timeout)
{
    int pos = 0, status, timeo;
    struct timeval now, end, tmo;
    struct pollfd ufd;

    gettimeofday(&now,NULL);
    timerset_ms(&tmo, timeout);
    timeradd(&now,&tmo,&end);

    ufd.fd = sock;
    ufd.events = POLLIN;

    size--;

    while(pos < size) {
        if (timeout > 0) {
            gettimeofday(&now,NULL);
            timersub_ms(&now,&end,&timeo);
        } 
        else
            timeo = -1;

        status = poll(&ufd, 1, timeo);

        if (status > 0)
            status = read(sock,&buffer[pos],1);

        if ( (status < 0) && ( (errno == EINTR) || (errno == EAGAIN) ) )
            continue; /* try again, if interrupted/blocking */

        if (status <= 0)
            break;
                        
        if (buffer[pos] == '\n') {
            pos++;
            break;
        }
        pos++;
    }

    buffer[pos]=0;

    return(pos);
}

static int
waitForString(PymolProxy *pymol, char *string, char *buffer, int length)
{
    int sock;

    assert(buffer != NULL);
    if (pymol->status != TCL_OK)
        return pymol->status;

    sock = pymol->p_stdout;
    trace("want to match (%s)\n", string);
    while(1) {
        if (getline(sock,buffer,length,IO_TIMEOUT) == 0) {
            pymol->error = 2;
            pymol->status = TCL_ERROR;
            break;
        }
        trace("stdout-u>read(%s)", buffer);
        if (strncmp(buffer, string, strlen(string)) == 0) {
            trace("stdout-e> %s",buffer);
            pymol->error = 0;
            pymol->status = TCL_OK;
            break;
        }
    }

    return pymol->status;
}

INLINE static int
clear_error(PymolProxy *pymol)
{
    pymol->error = 0;
    pymol->status = TCL_OK;
    return pymol->status;
}

static int
send_expect(PymolProxy *pymol, char *expect, char *cmd)
{
    if (pymol->error) {
        return(TCL_ERROR);
    }
    trace("to-pymol>(%s)", cmd);
    write(pymol->p_stdin, cmd, strlen(cmd));
    if (waitForString(pymol, expect, cmd, 800)) {
        trace("pymolproxy: Timeout reading data [%s]\n",cmd);
        pymol->error = 1;
        pymol->status = TCL_ERROR;
        return pymol->status;
    }
    return( pymol->status );
}

static int
sendf(PymolProxy *pymol, char *format, ...)
{
    va_list ap;
    char buffer[800];

    if (pymol->error)
        return(TCL_ERROR);

    va_start(ap, format);
    vsnprintf(buffer, 800, format, ap);
    va_end(ap);
    trace("to-pymol>(%s)", buffer);
    write(pymol->p_stdin, buffer, strlen(buffer));

    if (waitForString(pymol, "PyMOL>", buffer, 800)) {
        trace("pymolproxy: Timeout reading data [%s]\n",buffer);
        pymol->error = 1;
        pymol->status = TCL_ERROR;
        return pymol->status;
    }

    return( pymol->status );
}

static int
BallNStickCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    int ghost = 0, defer = 0, push = 0, arg;
    const char *model = "all";
    PymolProxy *pymol = (PymolProxy *) cdata;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0)
            push = 1;
        else if (strcmp(argv[arg],"-ghost") == 0)
            ghost = 1;
        else if (strcmp(argv[arg],"-normal") == 0)
            ghost = 0;
        else if (strcmp(argv[arg],"-model") == 0) {
            if (++arg < argc)
                model = argv[arg];
        }
        else
            model = argv[arg];
    }

    pymol->invalidate_cache = 1;
    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

    sendf(pymol, "hide everything,%s\n",model);
    sendf(pymol, "set stick_color,white,%s\n",model);
        
    if (ghost)
        sendf(pymol, "set stick_radius,0.1,%s\n",model);
    else
        sendf(pymol, "set stick_radius,0.14,%s\n",model);

    sendf(pymol, "set sphere_scale=0.25,%s\n", model);

    if (ghost) {
        sendf(pymol, "set sphere_transparency,0.75,%s\n", model);
        sendf(pymol, "set stick_transparency,0.75,%s\n", model);
    }
    else {
        sendf(pymol, "set sphere_transparency,0,%s\n", model);
        sendf(pymol, "set stick_transparency,0,%s\n", model);
    }

    sendf(pymol, "show sticks,%s\n",model);
    sendf(pymol, "show spheres,%s\n",model);

    if (pymol->labels)
        sendf(pymol, "show labels,%s\n", model);

    return( pymol->status );
}

static int
SpheresCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    int defer = 0, ghost = 0, push = 0, arg;
    const char *model = "all";
    PymolProxy *pymol = (PymolProxy *) cdata;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0)
            push = 1;
        else if (strcmp(argv[arg],"-ghost") == 0)
            ghost = 1;
        else if (strcmp(argv[arg],"-normal") == 0)
            ghost = 0;
        else if (strcmp(argv[arg],"-model") == 0) {
            if (++arg < argc)
                model = argv[arg];
        }
        else
            model = argv[arg];
    }

    pymol->invalidate_cache = 1;
    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

    sendf(pymol, "hide everything, %s\n", model);
    sendf(pymol, "set sphere_scale,0.41,%s\n", model);
    //sendf(pymol, "set sphere_quality,2,%s\n", model);
    sendf(pymol, "set ambient,.2,%s\n", model);

    if (ghost)
        sendf(pymol, "set sphere_transparency,.75,%s\n", model);
    else
        sendf(pymol, "set sphere_transparency,0,%s\n", model);

    sendf(pymol, "show spheres,%s\n", model);

    if (pymol->labels)
        sendf(pymol, "show labels,%s\n", model);

    return pymol->status;
}

static int
LinesCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    int ghost = 0, defer = 0, push = 0, arg;
    const char *model = "all";
    PymolProxy *pymol = (PymolProxy *) cdata;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0)
            push = 1;
        else if (strcmp(argv[arg],"-ghost") == 0)
            ghost = 1;
        else if (strcmp(argv[arg],"-normal") == 0)
            ghost = 0;
        else if (strcmp(argv[arg],"-model") == 0) {
            if (++arg < argc)
                model = argv[arg];
        }
        else
            model = argv[arg];
    }

    pymol->invalidate_cache = 1;
    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

    sendf(pymol, "hide everything,%s\n",model);

    if (ghost)
        sendf(pymol, "set line_width,.25,%s\n",model);
    else
        sendf(pymol, "set line_width,1,%s\n",model);

    sendf(pymol, "show lines,%s\n",model);

    if (pymol->labels)
        sendf(pymol, "show labels,%s\n",model);

    return pymol->status;
}

static int
DisableCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *pymol = (PymolProxy *) cdata;
    const char *model = "all";
    int arg, defer = 0, push = 0;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {

        if (strcmp(argv[arg], "-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg], "-push") == 0 )
            push = 1;
        else
            model = argv[arg];
        
    }

    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;
    pymol->invalidate_cache = 1;

    sendf( pymol, "disable %s\n", model);

    return pymol->status;
}

static int
EnableCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *pymol = (PymolProxy *) cdata;
    const char *model = "all";
    int arg, defer = 0, push = 0;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
                
        if (strcmp(argv[arg],"-defer") == 0)
            defer = 1;
        else if (strcmp(argv[arg], "-push") == 0 )
            push = 1;
        else
            model = argv[arg];

    }

    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;
    pymol->invalidate_cache = 1;

    sendf( pymol, "enable %s\n", model);

    return pymol->status;
}

static int
VMouseCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *pymol = (PymolProxy *) cdata;
    int arg, defer = 0, push = 0, varg = 1;
    int arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0, arg5 = 0;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "-defer") == 0)
            defer = 1;
        else if (strcmp(argv[arg], "-push") == 0)
            push = 1;
        else if (varg == 1) {
            arg1 = atoi(argv[arg]);
            varg++;
        }
        else if (varg == 2) {
            arg2 = atoi(argv[arg]);
            varg++;
        }
        else if (varg == 3) {
            arg3 = atoi(argv[arg]);
            varg++;
        }
        else if (varg == 4) {
            arg4 = atoi(argv[arg]);
            varg++;
        }
        else if (varg == 5) {
            arg5 = atoi(argv[arg]);
            varg++;
        }
    }

    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;
    pymol->invalidate_cache = 1;

    sendf(pymol, "vmouse %d,%d,%d,%d,%d\n", arg1,arg2,arg3,arg4,arg5);

    return pymol->status;
}

static int
RawCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *pymol = (PymolProxy *) cdata;
    DyBuffer buffer;
    int arg, defer = 0, push = 0;
    const char *cmd;
    clear_error(pymol);

    dyBufferInit(&buffer);

    cmd = NULL;
    for(arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "-defer") == 0)
            defer = 1;
        else if (strcmp(argv[arg], "-push") == 0)
            push = 1;
        else {
            cmd = argv[arg];
        }
    }

    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;
    pymol->invalidate_cache = 1;

    sendf(pymol,"%s\n", cmd);
    dyBufferFree(&buffer);

    return pymol->status;
}

static int
LabelCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *pymol = (PymolProxy *) cdata;
    int state = 1;
    int arg, push = 0, defer = 0;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0 )
            push = 1;
        else if (strcmp(argv[arg],"on") == 0 )
            state = 1;
        else if (strcmp(argv[arg],"off") == 0 )
            state = 0;
        else if (strcmp(argv[arg],"toggle") == 0 )
            state =  !pymol->labels;
    }

    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;
    pymol->invalidate_cache = 1;

    if (state) {
        sendf(pymol, "set label_color,white,all\n");
        sendf(pymol, "set label_size,14,all\n");
        sendf(pymol, "label all,\"%%s%%s\" %% (ID,name)\n");
    }
    else
        sendf(pymol, "label all\n");

    pymol->labels = state;

    return pymol->status;
}

static int
FrameCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *pymol = (PymolProxy *) cdata;
    int frame = 0;
    int arg, push = 0, defer = 0;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0 )
            push = 1;
        else
            frame = atoi(argv[arg]);
    }
                
    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

    pymol->frame = frame;

    sendf(pymol,"frame %d\n", frame);
        
    return pymol->status;
}

static int
ResetCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *pymol = (PymolProxy *) cdata;
    int arg, push = 0, defer = 0;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0 )
            push = 1;
    }
                
    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;
    pymol->invalidate_cache = 1;
        
    sendf(pymol, "reset\n");
    sendf(pymol, "zoom buffer=2\n");

    return pymol->status;
}

static int
RockCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *pymol = (PymolProxy *) cdata;
    float y = 0.0;
    int arg, push = 0, defer = 0;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0 )
            push = 1;
        else
            y = atof( argv[arg] );
    }
                
    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

    sendf(pymol,"turn y, %f\n", y - pymol->rock_offset);

    pymol->rock_offset = y;

    return pymol->status;
}

static int
ViewportCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *pymol = (PymolProxy *) cdata;
    int width = 640, height = 480;
    int defer = 0, push = 0, arg, varg = 1;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 ) 
            defer = 1;
        else if ( strcmp(argv[arg], "-push") == 0 )
            push = 1;
        else if (varg == 1) {
            width = atoi(argv[arg]);
            height = width;
            varg++;
        }
        else if (varg == 2) {
            height = atoi(argv[arg]);
            varg++;
        }
    }

    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;
    pymol->invalidate_cache = 1;

    sendf(pymol, "viewport %d,%d\n", width, height);

    //usleep(205000); // .2s delay for pymol to update its geometry *HACK ALERT*
        
    return pymol->status;
}

static int
LoadPDB2Cmd(ClientData cdata, Tcl_Interp *interp, int argc, 
	      const char *argv[])
{
    const char *pdbdata, *name;
    PymolProxy *pymol = (PymolProxy *) cdata;
    int state = 1;
    int arg, defer = 0, push = 0, varg = 1;
    
    if (pymol == NULL)
	return(TCL_ERROR);
    clear_error(pymol);
    pdbdata = name = NULL;	/* Suppress compiler warning. */
    for(arg = 1; arg < argc; arg++) {
	if ( strcmp(argv[arg],"-defer") == 0 )
	    defer = 1;
	else if (strcmp(argv[arg],"-push") == 0)
	    push = 1;
        else if (varg == 1) {
	    pdbdata = argv[arg];
	    varg++;
	} else if (varg == 2) {
	    name = argv[arg];
	    varg++;
	} else if (varg == 3) {
	    state = atoi( argv[arg] );
	    varg++;
	}
    }
    
    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

    {
	char fileName[200];
	FILE *f;
	size_t nBytes, nWritten;

	sprintf(fileName, "/tmp/pymol%d.pdb", getpid());
	f = fopen(fileName, "w");
	trace("pymolproxy: open file %s as %x\n", fileName, f);
	if (f == NULL) {
	    trace("pymolproxy: failed to open %s %d\n", fileName, errno);
	    perror("pymolproxy");
	}
	nBytes = strlen(pdbdata);
	nWritten = fwrite(pdbdata, sizeof(char), nBytes, f);
	if (nBytes != nWritten) {
	    trace("pymolproxy: short write %d wanted %d bytes\n", nWritten,
		  nBytes);
	    perror("pymolproxy");
	}
	fclose(f);
	sendf(pymol, "load %s, %s, %d\n", fileName, name, state);
    }
    sendf(pymol, "zoom buffer=2\n");
    return(pymol->status);
}

static int
LoadPDBCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    const char *pdbdata, *name;
    PymolProxy *pymol = (PymolProxy *) cdata;
    int state = 1;
    int arg, defer = 0, push = 0, varg = 1;

    clear_error(pymol);

    pdbdata = name = NULL;	/* Suppress compiler warning. */
    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0)
            push = 1;
        else if (varg == 1) {
            pdbdata = argv[arg];
            varg++;
        }
        else if (varg == 2) {
            name = argv[arg];
            varg++;
        }
        else if (varg == 3) {
            state = atoi( argv[arg] );
            varg++;
        }
    }

    pymol->need_update = !defer || push;
    pymol->immediate_update |= push;

    {
        int count;
        const char *p;
        char *q, *newdata;

        count = 0;
        for (p = pdbdata; *p != '\0'; p++) {
            if (*p == '\n') {
                count++;
            }
            count++;
        }
        
        q = newdata = malloc(count + 100);
        strcpy(newdata, "cmd.read_pdbstr(\"\"\"\\\n");
        q = newdata + strlen(newdata);
        for (p = pdbdata; *p != '\0'; p++, q++) {
            if (*p == '\n') {
                *q++ = '\\';
            } 
            *q = *p;
        }
        sprintf(q, "\\\n\"\"\",\"%s\",%d)\n", name, state);
        {
            char expect[800];

            sprintf(expect, "PyMOL>\"\"\",\"%s\",%d)\n", name, state);
            send_expect(pymol, expect, newdata);
        }
        free(newdata);
    }
    sendf(pymol, "zoom buffer=2\n");

    return pymol->status;
}


static int
RotateCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    double turnx = 0.0;
    double turny = 0.0;
    double turnz = 0.0;
    PymolProxy *pymol = (PymolProxy *) cdata;
    int defer = 0, push = 0, arg, varg = 1;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
	if (strcmp(argv[arg],"-defer") == 0) {
	    defer = 1; 
	} else if (strcmp(argv[arg],"-push") == 0) {
	    push = 1;
	} else  if (varg == 1) {
	    turnx = atof(argv[arg]);
	    varg++;
	} else if (varg == 2) {
	    turny = atof(argv[arg]);
	    varg++;
	} else if (varg == 3) {
	    turnz = atof(argv[arg]);
	    varg++;
	}
    } 
    pymol->need_update = !defer || push;
    pymol->immediate_update  |= push;
    pymol->invalidate_cache = 1;

    if (turnx != 0.0)
        sendf(pymol,"turn x, %f\n", turnx);
        
    if (turny != 0.0)
        sendf(pymol,"turn y, %f\n", turny);
        
    if (turnz != 0.0) 
        sendf(pymol,"turn z, %f\n", turnz);

    return pymol->status;
}

static int
ZoomCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    double factor = 0.0;
    double zmove = 0.0;
    PymolProxy *pymol = (PymolProxy *) cdata;
    int defer = 0, push = 0, arg, varg = 1;

    clear_error(pymol);

    for(arg = 1; arg < argc; arg++) {
	if (strcmp(argv[arg],"-defer") == 0)
	    defer = 1;
	else if (strcmp(argv[arg],"-push") == 0)
	    push = 1;
	else if (varg == 1) {
	    factor = atof(argv[arg]);
	    varg++;
	}
    }
    zmove = factor * -75;
 
    pymol->need_update = !defer || push;
    pymol->immediate_update  |= push;
    pymol->invalidate_cache = 1;

    if (zmove != 0.0)
        sendf(pymol,"move z, %f\n", factor);

    return pymol->status;
}

static int
PNGCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    char buffer[800];
    unsigned int nBytes=0;
    float samples = 0.0;
    PymolProxy *pymol = (PymolProxy *) cdata;

    clear_error(pymol);

    if (pymol->invalidate_cache)
        pymol->cacheid++;

    pymol->need_update = 0;
    pymol->immediate_update = 0;
    pymol->invalidate_cache = 0;

    sendf(pymol,"png -\n");

    waitForString(pymol, "image follows: ", buffer, 800);

    sscanf(buffer, "image follows: %d %f\n", &nBytes, &samples);
 
    write(3, &samples, sizeof(samples));
  
    dyBufferSetLength(&pymol->image, nBytes);

    bread(pymol->p_stdout, pymol->image.data, pymol->image.used);

    waitForString(pymol, " ScenePNG", buffer,800);

    if ((nBytes > 0) && (pymol->image.used == nBytes)) {
        sprintf(buffer, "nv>image %d %d %d %d\n", 
                nBytes, pymol->cacheid, pymol->frame, pymol->rock_offset);
        trace("to-molvis> %s", buffer);
        write(pymol->c_stdin, buffer, strlen(buffer));
        bwrite(pymol->c_stdin, pymol->image.data, nBytes);
	stats.nFrames++;
	stats.nBytes += nBytes;
    }
    return pymol->status;
}

static int
BMPCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    char buffer[800];
    unsigned int nBytes=0;
    float samples = 0.0;
    PymolProxy *pymol = (PymolProxy *) cdata;

    clear_error(pymol);

    if (pymol->invalidate_cache)
        pymol->cacheid++;

    pymol->need_update = 0;
    pymol->immediate_update = 0;
    pymol->invalidate_cache = 0;

    sendf(pymol,"bmp -\n");

    waitForString(pymol, "image follows: ", buffer, 800);

    sscanf(buffer, "image follows: %d %f\n", &nBytes, &samples);
    write(3,&samples,sizeof(samples));

    dyBufferSetLength(&pymol->image, nBytes);

    bread(pymol->p_stdout, pymol->image.data, pymol->image.used);

    if ((nBytes > 0) && (pymol->image.used == nBytes)) {
        sprintf(buffer, "nv>image %d %d %d %d\n", 
                nBytes, pymol->cacheid, pymol->frame, pymol->rock_offset);
        write(pymol->c_stdin, buffer, strlen(buffer));
        trace("to-molvis buffer=%s\n", buffer);
        bwrite(pymol->c_stdin, pymol->image.data, nBytes);
	stats.nFrames++;
	stats.nBytes += nBytes;
    }
    return pymol->status;
}
        
static int 
ProxyInit(int c_in, int c_out, char *const *argv)
{
    int flags, status, result = 0;
    int pairIn[2];
    int pairOut[2];
    int pairErr[2];
    Tcl_Interp *interp;
    Tcl_DString cmdbuffer;
    DyBuffer dybuffer, dybuffer2;
    struct pollfd ufd[3];
    int pid;
    PymolProxy pymol;
    struct timeval now, end;
    int timeout;

    /* Create three pipes for communication with the external application. One
     * each for the applications's: stdin, stdout, and stderr  */

    if (pipe(pairIn) == -1)
        return(-1);

    if (pipe(pairOut) == -1) {
        close(pairIn[0]);
        close(pairIn[1]);
        return(-1);
    }

    if (pipe(pairErr) == -1) {
        close(pairIn[0]);
        close(pairIn[1]);
        close(pairOut[0]);
        close(pairOut[1]);
        return(-1);
    }

    /* Fork the new process.  Connect i/o to the new socket.  */

    pid = fork();
        
    if (pid < 0) {
        fprintf(stderr, "can't fork process: %s\n", strerror(errno));
        return(-3);
    }
    if (pid == 0) {
        int fd;

        /* Child process */
        
        /* 
         * Create a new process group, so we can later kill this process and
         * all its children without affecting the process that created this
         * one.
         */
        setpgid(pid, 0); 
        
        /* Redirect stdin, stdout, and stderr to pipes before execing */ 
        dup2(pairIn[0] ,0);  // stdin
        dup2(pairOut[1],1);  // stdout
        dup2(pairErr[1],2);  // stderr
        
        for(fd = 3; fd < FD_SETSIZE; fd++)  /* close all other descriptors  */
            close(fd);
        
        execvp(argv[0], argv);
        trace("pymolproxy: Failed to start pyMol %s\n", argv[0]);
	exit(-1);
    }
    stats.child = pid;

    /* close opposite end of pipe, these now belong to the child process        */
    close(pairIn[0]);
    close(pairOut[1]);
    close(pairErr[1]);

    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE (ie if nanoscale terminates)

    pymol.p_stdin = pairIn[1];
    pymol.p_stdout = pairOut[0];
    pymol.p_stderr = pairErr[0];
    pymol.c_stdin  = c_in;
    pymol.c_stdout = c_out;
    pymol.labels = 0;
    pymol.need_update = 0;
    pymol.can_update = 1;
    pymol.immediate_update = 0;
    pymol.sync = 0;
    pymol.frame = 1;
    pymol.rock_offset = 0;
    pymol.cacheid = 0;
    pymol.invalidate_cache = 0;

    ufd[0].fd = pymol.c_stdout;
    ufd[0].events = POLLIN | POLLHUP; /* ensure catching EOF */
    ufd[1].fd = pymol.p_stdout;
    ufd[1].events = POLLIN | POLLHUP;
    ufd[2].fd = pymol.p_stderr;
    ufd[2].events = POLLIN | POLLHUP;

    flags = fcntl(pymol.p_stdout, F_GETFL);
    fcntl(pymol.p_stdout, F_SETFL, flags|O_NONBLOCK);

    interp = Tcl_CreateInterp();
    Tcl_MakeSafe(interp);

    Tcl_DStringInit(&cmdbuffer);
    dyBufferInit(&pymol.image);
    dyBufferInit(&dybuffer);
    dyBufferInit(&dybuffer2);

    Tcl_CreateCommand(interp, "bmp",     BMPCmd,        &pymol, NULL);
    Tcl_CreateCommand(interp, "png",     PNGCmd,        &pymol, NULL);
    Tcl_CreateCommand(interp, "screen",  ViewportCmd,   &pymol, NULL);
    Tcl_CreateCommand(interp, "viewport",ViewportCmd,   &pymol, NULL);
    Tcl_CreateCommand(interp, "rotate",  RotateCmd,     &pymol, NULL);
    Tcl_CreateCommand(interp, "zoom",    ZoomCmd,       &pymol, NULL);
    Tcl_CreateCommand(interp, "loadpdb.old", LoadPDBCmd,    &pymol, NULL);
    Tcl_CreateCommand(interp, "loadpdb", LoadPDB2Cmd,  &pymol, NULL);
    Tcl_CreateCommand(interp, "ballnstick",BallNStickCmd, &pymol, NULL);
    Tcl_CreateCommand(interp, "spheres", SpheresCmd,    &pymol, NULL);
    Tcl_CreateCommand(interp, "lines",   LinesCmd,      &pymol, NULL);
    Tcl_CreateCommand(interp, "raw",     RawCmd,        &pymol, NULL);
    Tcl_CreateCommand(interp, "label",   LabelCmd,      &pymol, NULL);
    Tcl_CreateCommand(interp, "reset",   ResetCmd,      &pymol, NULL);
    Tcl_CreateCommand(interp, "rock",    RockCmd,       &pymol, NULL);
    Tcl_CreateCommand(interp, "frame",   FrameCmd,      &pymol, NULL);
    Tcl_CreateCommand(interp, "vmouse",  VMouseCmd,     &pymol, NULL);
    Tcl_CreateCommand(interp, "disable", DisableCmd,    &pymol, NULL);
    Tcl_CreateCommand(interp, "enable",  EnableCmd,     &pymol, NULL);

    // Main Proxy Loop
    //  accept tcl commands from socket
    //  translate them into pyMol commands, and issue them to child proccess
    //  send images back

    gettimeofday(&end, NULL);
    stats.start = end;
    while(1) {
	char ch;
	
	gettimeofday(&now,NULL);
	
	if ( (!pymol.need_update) )
	    timeout = -1;
	else if ((now.tv_sec > end.tv_sec) || ( (now.tv_sec == end.tv_sec) && (now.tv_usec >= end.tv_usec)) )
	    timeout = 0; 
	else {
	    timeout = (end.tv_sec - now.tv_sec) * 1000;
	    
	    if (end.tv_usec > now.tv_usec)
		timeout += (end.tv_usec - now.tv_usec) / 1000;
	    else
		timeout += (((1000000 + end.tv_usec) - now.tv_usec) / 1000) - 1000;
	    
	}
	
	if (!pymol.immediate_update)
	    status = poll(ufd, 3, timeout);
	else
	    status = 0;
	
	if ( status < 0 ) {
	    trace("pymolproxy: POLL ERROR: status = %d, errno = %d, %s \n", status,errno,strerror(errno));
	} else if (status > 0) {
	    gettimeofday(&now,NULL);
	    
	    if (ufd[0].revents) { /* Client Stdout Connection: command input */
		if (read(ufd[0].fd,&ch,1) <= 0) {
		    if (errno != EINTR) {
			trace("pymolproxy: lost client connection (%d).\n", errno);
			break;
		    }
		} else {
		    Tcl_DStringAppend(&cmdbuffer, &ch, 1);
		    
		    if (ch == '\n' && Tcl_CommandComplete(Tcl_DStringValue(&cmdbuffer))) {
			int result;

			result = ExecuteCommand(interp, &cmdbuffer);
			if (timeout == 0) status = 0; // send update
		    }
		}
	    }

	    if (ufd[1].revents ) { /* pyMol Stdout Connection: pymol (unexpected) output */
		if (read(ufd[1].fd, &ch, 1) <= 0) {
		    if (errno != EINTR) {
			trace("pymolproxy: lost connection (stdout) to pymol server\n");
			break;
		    }
		} else {
		    dyBufferAppend(&dybuffer, &ch, 1);
		    
		    if (ch == '\n') {
			ch = 0;
			dyBufferAppend(&dybuffer, &ch, 1);
			trace("STDOUT>%s",dybuffer.data);
			dyBufferSetLength(&dybuffer,0);
		    }
		}
	    }
	    
	    if (ufd[2].revents) { /* pyMol Stderr Connection: pymol standard error output */
		if (read(ufd[2].fd, &ch, 1) <= 0) {
		    if (errno != EINTR) { 
			trace("pymolproxy: lost connection (stderr) to pymol server\n");
			break;
		    }
		} else {
		    dyBufferAppend(&dybuffer2, &ch, 1);
		    
		    if (ch == '\n') {
			ch = 0;
			dyBufferAppend(&dybuffer2, &ch, 1);
			trace("stderr>%s", dybuffer2.data);
			dyBufferSetLength(&dybuffer2,0);
		    }
		}
	    }
	}
	
	if (status == 0) {
	    gettimeofday(&now,NULL);
	    
	    if (pymol.need_update && pymol.can_update)
		Tcl_Eval(interp, "bmp -\n");
	    
	    end.tv_sec = now.tv_sec;
	    end.tv_usec = now.tv_usec + 150000;
	    
	    if (end.tv_usec >= 1000000) {
		end.tv_sec++;
		end.tv_usec -= 1000000;
	    }
	    
	}
	
    }
    
    status = waitpid(pid, &result, WNOHANG);
    if (status == -1) {
        trace("pymolproxy: error waiting on pymol server to exit (%d)\n", errno);
    } else if (status == 0) {
        trace("pymolproxy: attempting to SIGTERM pymol server\n");
        kill(-pid, SIGTERM); // kill process group
        alarm(5);
        status = waitpid(pid, &result, 0);
        alarm(0);

        while ((status == -1) && (errno == EINTR)) {
	    trace("pymolproxy: Attempting to SIGKILL process.\n");
	    kill(-pid, SIGKILL); // kill process group
	    alarm(10);
	    status = waitpid(pid, &result, 0);
	    alarm(0); 
	}
    }
    
    trace("pymolproxy: pymol server process ended (%d)\n", result);
    
    dyBufferFree(&pymol.image);
    
    Tcl_DeleteInterp(interp);
    DoExit(result);
    return 0;
}

#ifdef STANDALONE

int
main(int argc, char *argv[])
{
    flog = stderr;
    if (debug) {
	char fileName[200];
	sprintf(fileName, "/tmp/pymolproxy%d.log", getpid());
        flog = fopen(fileName, "w");
    }    
    ProxyInit(fileno(stdout), fileno(stdin), argv + 1);
    return 0;
}

#endif

