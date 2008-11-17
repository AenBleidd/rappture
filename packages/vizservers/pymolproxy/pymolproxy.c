
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
static int debug = TRUE;

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
        fprintf(flog, "pymolproxy: ");
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

static void
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

    trace("bwrite: want to write %d bytes.", size);
    while(1) {
        result = write(sock,buffer+total,left);

        if (result <= 0)
            break;

        total += result;
        left -= result;

        if (total == size)
            break;
    }
    trace("bwrite: wrote %d bytes.", total);
    return total;
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
	    trace("Error reading sock(%d), %d/%s.", sock, errno, 
		  strerror(errno));
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

INLINE static void
clear_error(PymolProxy *proxyPtr)
{
    proxyPtr->error = 0;
    proxyPtr->status = TCL_OK;
}

static int
getline(int sock, char *buffer, int size, int timeout)
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

        if (status <= 0) {
	    trace("getline: status=%d, errno=%d", status, errno);
            break;
	}
        if (buffer[pos] == '\n') {
            pos++;
            break;
        }
        pos++;
    }

    buffer[pos]=0;
    return pos;
}

static int
Expect(PymolProxy *proxyPtr, char *match, char *buffer, int length)
{
    int sock;
    char c;
    size_t mlen;

    assert(buffer != NULL);
    if (proxyPtr->status != TCL_OK)
        return proxyPtr->status;

    sock = proxyPtr->p_stdout;
    trace("Expect(%s)", match);
    c = match[0];
    mlen = strlen(match);
    for (;;) {
        if (getline(sock, buffer, length, IO_TIMEOUT) == 0) {
            proxyPtr->error = 2;
            proxyPtr->status = TCL_ERROR;
            break;
        }
        trace("stdout-u>read(%s)", buffer);
        if ((c == buffer[0]) && (strncmp(buffer, match, mlen) == 0)) {
            trace("stdout-e> %s", buffer);
	    clear_error(proxyPtr);
            break;
        }
    }

    return proxyPtr->status;
}

static int
Send(PymolProxy *proxyPtr, char *format, ...)
{
    va_list ap;
    char iobuffer[BUFSIZ];

    if (proxyPtr->error) {
        return TCL_ERROR;
    }
    va_start(ap, format);
    vsnprintf(iobuffer, 800, format, ap);
    va_end(ap);

    trace("to-pymol>(%s)", iobuffer);

    /* Write the command out to the server. */
    write(proxyPtr->p_stdin, iobuffer, strlen(iobuffer));

    /* Now wait for the "PyMOL>" prompt. */
    if (Expect(proxyPtr, "PyMOL>", iobuffer, BUFSIZ)) {
        trace("timeout reading data (iobuffer=%s)", iobuffer);
        proxyPtr->error = 1;
        proxyPtr->status = TCL_ERROR;
        return proxyPtr->status;
    }
    return  proxyPtr->status;
}

static int
BallNStickCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	      const char *argv[])
{
    float radius, transparency;
    int ghost = 0, defer = 0, push = 0, arg;
    const char *model = "all";
    PymolProxy *proxyPtr = clientData;

    clear_error(proxyPtr);

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

    proxyPtr->invalidate_cache = 1;
    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;

    Send(proxyPtr, "hide everything,%s\n",model);
    Send(proxyPtr, "set stick_color,white,%s\n",model);
    if (ghost) {
	radius = 0.1f;
	transparency = 0.75f;
    } else {
	radius = 0.14f;
	transparency = 0.0f;
    }
    Send(proxyPtr, "set stick_radius,%g,%s\n", radius, model);
    Send(proxyPtr, "set sphere_scale=0.25,%s\n", model);
    Send(proxyPtr, "set sphere_transparency,%g,%s\n", transparency, model);
    Send(proxyPtr, "set stick_transparency,%g,%s\n", transparency, model);
    Send(proxyPtr, "show sticks,%s\n",model);
    Send(proxyPtr, "show spheres,%s\n",model);

    if (proxyPtr->labels)
        Send(proxyPtr, "show labels,%s\n", model);

    return proxyPtr->status;
}

static int
SpheresCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    int defer = 0, ghost = 0, push = 0, arg;
    const char *model = "all";
    PymolProxy *proxyPtr = clientData;

    clear_error(proxyPtr);

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

    proxyPtr->invalidate_cache = 1;
    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;

    Send(proxyPtr, "hide everything, %s\n", model);
    Send(proxyPtr, "set sphere_scale,0.41,%s\n", model);
    //Send(proxyPtr, "set sphere_quality,2,%s\n", model);
    Send(proxyPtr, "set ambient,.2,%s\n", model);

    if (ghost)
        Send(proxyPtr, "set sphere_transparency,.75,%s\n", model);
    else
        Send(proxyPtr, "set sphere_transparency,0,%s\n", model);

    Send(proxyPtr, "show spheres,%s\n", model);

    if (proxyPtr->labels)
        Send(proxyPtr, "show labels,%s\n", model);

    return proxyPtr->status;
}

static int
LinesCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	 const char *argv[])
{
    int ghost = 0, defer = 0, push = 0, arg;
    const char *model = "all";
    PymolProxy *proxyPtr = clientData;
    float lineWidth;

    clear_error(proxyPtr);

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

    proxyPtr->invalidate_cache = 1;
    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;

    Send(proxyPtr, "hide everything,%s\n",model);

    lineWidth = (ghost) ? 0.25f : 1.0f;
    Send(proxyPtr, "set line_width,%g,%s\n", lineWidth, model);
    Send(proxyPtr, "show lines,%s\n",model);
    if (proxyPtr->labels)
        Send(proxyPtr, "show labels,%s\n",model);

    return proxyPtr->status;
}

static int
DisableCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    const char *model = "all";
    int arg, defer = 0, push = 0;

    clear_error(proxyPtr);

    for(arg = 1; arg < argc; arg++) {

        if (strcmp(argv[arg], "-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg], "-push") == 0 )
            push = 1;
        else
            model = argv[arg];
        
    }

    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;
    proxyPtr->invalidate_cache = 1;

    Send( proxyPtr, "disable %s\n", model);

    return proxyPtr->status;
}

static int
EnableCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	  const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    const char *model = "all";
    int arg, defer = 0, push = 0;

    clear_error(proxyPtr);

    for(arg = 1; arg < argc; arg++) {
                
        if (strcmp(argv[arg],"-defer") == 0)
            defer = 1;
        else if (strcmp(argv[arg], "-push") == 0 )
            push = 1;
        else
            model = argv[arg];

    }

    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;
    proxyPtr->invalidate_cache = 1;

    Send( proxyPtr, "enable %s\n", model);

    return proxyPtr->status;
}

static int
VMouseCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	  const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    int arg, defer = 0, push = 0, varg = 1;
    int arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0, arg5 = 0;

    clear_error(proxyPtr);

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

    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;
    proxyPtr->invalidate_cache = 1;

    Send(proxyPtr, "vmouse %d,%d,%d,%d,%d\n", arg1, arg2, arg3, 
		    arg4, arg5);

    return proxyPtr->status;
}

static int
RawCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    DyBuffer buffer;
    int arg, defer = 0, push = 0;
    const char *cmd;
    clear_error(proxyPtr);

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

    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;
    proxyPtr->invalidate_cache = 1;

    Send(proxyPtr,"%s\n", cmd);
    dyBufferFree(&buffer);

    return proxyPtr->status;
}

static int
LabelCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	 const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    int state = 1;
    int arg, push = 0, defer = 0;

    clear_error(proxyPtr);

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
            state =  !proxyPtr->labels;
    }

    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;
    proxyPtr->invalidate_cache = 1;

    if (state) {
        Send(proxyPtr, "set label_color,white,all\n");
        Send(proxyPtr, "set label_size,14,all\n");
        Send(proxyPtr, "label all,\"%%s%%s\" %% (ID,name)\n");
    }
    else
        Send(proxyPtr, "label all\n");

    proxyPtr->labels = state;

    return proxyPtr->status;
}

static int
FrameCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	 const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    int frame = 0;
    int arg, push = 0, defer = 0;

    clear_error(proxyPtr);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0 )
            push = 1;
        else
            frame = atoi(argv[arg]);
    }
                
    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;

    proxyPtr->frame = frame;

    Send(proxyPtr,"frame %d\n", frame);
        
    return proxyPtr->status;
}

static int
ResetCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	 const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    int arg, push = 0, defer = 0;

    clear_error(proxyPtr);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0 )
            push = 1;
    }
                
    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;
    proxyPtr->invalidate_cache = 1;
        
    Send(proxyPtr, "reset\n");
    Send(proxyPtr, "zoom buffer=2\n");

    return proxyPtr->status;
}

static int
RockCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    float y = 0.0;
    int arg, push = 0, defer = 0;

    clear_error(proxyPtr);

    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0 )
            push = 1;
        else
            y = atof( argv[arg] );
    }
                
    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;

    Send(proxyPtr,"turn y, %f\n", y - proxyPtr->rock_offset);

    proxyPtr->rock_offset = y;

    return proxyPtr->status;
}

static int
ViewportCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	    const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    int width = 640, height = 480;
    int defer = 0, push = 0, arg, varg = 1;

    clear_error(proxyPtr);

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

    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;
    proxyPtr->invalidate_cache = 1;

    Send(proxyPtr, "viewport %d,%d\n", width, height);

    //usleep(205000); // .2s delay for pymol to update its geometry *HACK ALERT*
        
    return proxyPtr->status;
}

static int
LoadPDBCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    const char *pdbdata, *name;
    PymolProxy *proxyPtr = clientData;
    int state = 1;
    int arg, defer = 0, push = 0, varg = 1;
    
    if (proxyPtr == NULL)
	return TCL_ERROR;
    clear_error(proxyPtr);
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
    
    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update |= push;

    {
	char fileName[200];
	FILE *f;
	ssize_t nBytes, nWritten;

	sprintf(fileName, "/tmp/pymol%d.pdb", getpid());
	f = fopen(fileName, "w");
	if (f == NULL) {
	    trace("can't open `%s': %s", fileName, strerror(errno));
	    perror("pymolproxy");
	}
	nBytes = strlen(pdbdata);
	nWritten = fwrite(pdbdata, sizeof(char), nBytes, f);
	if (nBytes != nWritten) {
	    trace("short write %d wanted %d bytes", nWritten, nBytes);
	    perror("pymolproxy");
	}
	fclose(f);
	Send(proxyPtr, "load %s, %s, %d\n", fileName, name, state);
    }
    Send(proxyPtr, "zoom buffer=2\n");
    return proxyPtr->status;
}

static int
RotateCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	  const char *argv[])
{
    double turnx = 0.0;
    double turny = 0.0;
    double turnz = 0.0;
    PymolProxy *proxyPtr = clientData;
    int defer = 0, push = 0, arg, varg = 1;

    clear_error(proxyPtr);

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
    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update  |= push;
    proxyPtr->invalidate_cache = 1;

    if (turnx != 0.0)
        Send(proxyPtr,"turn x, %f\n", turnx);
        
    if (turny != 0.0)
        Send(proxyPtr,"turn y, %f\n", turny);
        
    if (turnz != 0.0) 
        Send(proxyPtr,"turn z, %f\n", turnz);

    return proxyPtr->status;
}

static int
ZoomCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	const char *argv[])
{
    double factor = 0.0;
    PymolProxy *proxyPtr = clientData;
    int defer = 0, push = 0, arg, varg = 1;

    clear_error(proxyPtr);

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
    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update  |= push;
    proxyPtr->invalidate_cache = 1;
    if (factor != 0.0) {
        Send(proxyPtr,"move z,%f\n", factor);
    }
    return proxyPtr->status;
}

static int
PanCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    double x, y;
    int i;
    int defer, push;

    clear_error(proxyPtr);
    defer = push = FALSE;
    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i],"-defer") == 0) {
	    defer = 1;
	} else if (strcmp(argv[i],"-push") == 0) {
	    push = 1;
	} else { 
	    break;
	}
    }
    if ((Tcl_GetDouble(interp, argv[i], &x) != TCL_OK) ||
	(Tcl_GetDouble(interp, argv[i+1], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    proxyPtr->need_update = !defer || push;
    proxyPtr->immediate_update  |= push;
    proxyPtr->invalidate_cache = 1;
    if (x != 0.0) {
        Send(proxyPtr,"move x,%f\n", x * 0.05);
    }	
    if (y != 0.0) {
        Send(proxyPtr,"move y,%f\n", -y * 0.05);
    }	
    return proxyPtr->status;
}

static int
PNGCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    char buffer[800];
    unsigned int nBytes=0;
    float samples = 10.0;
    PymolProxy *proxyPtr = clientData;

    clear_error(proxyPtr);

    if (proxyPtr->invalidate_cache)
        proxyPtr->cacheid++;

    proxyPtr->need_update = 0;
    proxyPtr->immediate_update = 0;
    proxyPtr->invalidate_cache = 0;

    Send(proxyPtr,"png -\n");

    Expect(proxyPtr, "image follows: ", buffer, 800);

    sscanf(buffer, "image follows: %d\n", &nBytes);
 
    write(3, &samples, sizeof(samples));
  
    dyBufferSetLength(&proxyPtr->image, nBytes);

    bread(proxyPtr->p_stdout, proxyPtr->image.data, proxyPtr->image.used);

    Expect(proxyPtr, " ScenePNG", buffer,800);

    if ((nBytes > 0) && (proxyPtr->image.used == nBytes)) {
        sprintf(buffer, "nv>image %d %d %d %d\n", 
                nBytes, proxyPtr->cacheid, proxyPtr->frame, proxyPtr->rock_offset);
        trace("to-molvis> %s", buffer);
        write(proxyPtr->c_stdin, buffer, strlen(buffer));
        bwrite(proxyPtr->c_stdin, proxyPtr->image.data, nBytes);
	stats.nFrames++;
	stats.nBytes += nBytes;
    }
    return proxyPtr->status;
}

static int
BMPCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    char buffer[800];
    unsigned int nBytes=0;
    float samples = 10.0;
    PymolProxy *proxyPtr = clientData;

    clear_error(proxyPtr);

    if (proxyPtr->invalidate_cache)
        proxyPtr->cacheid++;

    proxyPtr->need_update = 0;
    proxyPtr->immediate_update = 0;
    proxyPtr->invalidate_cache = 0;

    Send(proxyPtr,"bmp -\n");

    Expect(proxyPtr, "image follows: ", buffer, 800);

    sscanf(buffer, "image follows: %d\n", &nBytes);
    write(3,&samples,sizeof(samples));

    dyBufferSetLength(&proxyPtr->image, nBytes);

    bread(proxyPtr->p_stdout, proxyPtr->image.data, proxyPtr->image.used);

    if ((nBytes > 0) && (proxyPtr->image.used == nBytes)) {
        sprintf(buffer, "nv>image %d %d %d %d\n", 
                nBytes, proxyPtr->cacheid, proxyPtr->frame, proxyPtr->rock_offset);
        write(proxyPtr->c_stdin, buffer, strlen(buffer));
        trace("to-molvis buffer=%s", buffer);
        bwrite(proxyPtr->c_stdin, proxyPtr->image.data, nBytes);
	stats.nFrames++;
	stats.nBytes += nBytes;
    }
    return proxyPtr->status;
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
    PymolProxy proxy;
    struct timeval now, end;
    int timeout;

    /* Create three pipes for communication with the external application. One
     * each for the applications's: stdin, stdout, and stderr  */

    if (pipe(pairIn) == -1)
        return -1;

    if (pipe(pairOut) == -1) {
        close(pairIn[0]);
        close(pairIn[1]);
        return -1;
    }

    if (pipe(pairErr) == -1) {
        close(pairIn[0]);
        close(pairIn[1]);
        close(pairOut[0]);
        close(pairOut[1]);
        return -1;
    }

    /* Fork the new process.  Connect i/o to the new socket.  */

    pid = fork();
        
    if (pid < 0) {
        fprintf(stderr, "can't fork process: %s\n", strerror(errno));
        return -3;
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
        trace("Failed to start pymol `%s'", argv[0]);
	exit(-1);
    }
    stats.child = pid;

    /* close opposite end of pipe, these now belong to the child process        */
    close(pairIn[0]);
    close(pairOut[1]);
    close(pairErr[1]);

    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE (ie if nanoscale terminates)

    proxy.p_stdin = pairIn[1];
    proxy.p_stdout = pairOut[0];
    proxy.p_stderr = pairErr[0];
    proxy.c_stdin  = c_in;
    proxy.c_stdout = c_out;
    proxy.labels = 0;
    proxy.need_update = 0;
    proxy.can_update = 1;
    proxy.immediate_update = 0;
    proxy.sync = 0;
    proxy.frame = 1;
    proxy.rock_offset = 0;
    proxy.cacheid = 0;
    proxy.invalidate_cache = 0;

    ufd[0].fd = proxy.c_stdout;
    ufd[0].events = POLLIN | POLLHUP; /* ensure catching EOF */
    ufd[1].fd = proxy.p_stdout;
    ufd[1].events = POLLIN | POLLHUP;
    ufd[2].fd = proxy.p_stderr;
    ufd[2].events = POLLIN | POLLHUP;

    flags = fcntl(proxy.p_stdout, F_GETFL);
    fcntl(proxy.p_stdout, F_SETFL, flags|O_NONBLOCK);

    interp = Tcl_CreateInterp();
    Tcl_MakeSafe(interp);

    Tcl_DStringInit(&cmdbuffer);
    dyBufferInit(&proxy.image);
    dyBufferInit(&dybuffer);
    dyBufferInit(&dybuffer2);

    Tcl_CreateCommand(interp, "bmp",     BMPCmd,        &proxy, NULL);
    Tcl_CreateCommand(interp, "png",     PNGCmd,        &proxy, NULL);
    Tcl_CreateCommand(interp, "screen",  ViewportCmd,   &proxy, NULL);
    Tcl_CreateCommand(interp, "viewport",ViewportCmd,   &proxy, NULL);
    Tcl_CreateCommand(interp, "rotate",  RotateCmd,     &proxy, NULL);
    Tcl_CreateCommand(interp, "zoom",    ZoomCmd,       &proxy, NULL);
    Tcl_CreateCommand(interp, "loadpdb", LoadPDBCmd,    &proxy, NULL);
    Tcl_CreateCommand(interp, "ballnstick",BallNStickCmd, &proxy, NULL);
    Tcl_CreateCommand(interp, "spheres", SpheresCmd,    &proxy, NULL);
    Tcl_CreateCommand(interp, "lines",   LinesCmd,      &proxy, NULL);
    Tcl_CreateCommand(interp, "raw",     RawCmd,        &proxy, NULL);
    Tcl_CreateCommand(interp, "label",   LabelCmd,      &proxy, NULL);
    Tcl_CreateCommand(interp, "reset",   ResetCmd,      &proxy, NULL);
    Tcl_CreateCommand(interp, "rock",    RockCmd,       &proxy, NULL);
    Tcl_CreateCommand(interp, "frame",   FrameCmd,      &proxy, NULL);
    Tcl_CreateCommand(interp, "vmouse",  VMouseCmd,     &proxy, NULL);
    Tcl_CreateCommand(interp, "disable", DisableCmd,    &proxy, NULL);
    Tcl_CreateCommand(interp, "enable",  EnableCmd,     &proxy, NULL);
    Tcl_CreateCommand(interp, "pan",     PanCmd,        &proxy, NULL);

    // Main Proxy Loop
    //  accept tcl commands from socket
    //  translate them into pyMol commands, and issue them to child proccess
    //  send images back

    gettimeofday(&end, NULL);
    stats.start = end;
    while(1) {
	gettimeofday(&now,NULL);
	
	if ( (!proxy.need_update) )
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
	
	status = 0;
	errno = 0;
	if (!proxy.immediate_update) {
	    status = poll(ufd, 3, timeout);
	}
	if ( status < 0 ) {
	    trace("POLL ERROR: status = %d: %s", status, strerror(errno));
	} else if (status > 0) {

	    gettimeofday(&now,NULL);
	    if (ufd[0].revents & POLLIN) { 
		/* Client Stdout Connection: command input */
		ssize_t nRead;
		char ch;

		nRead = read(ufd[0].fd, &ch, 1);
		if (nRead <= 0) {
		    trace("unexpected read error from client (stdout): %s",
			  strerror(errno));
		    if (errno != EINTR) {
			trace("lost client connection: %s", strerror(errno));
			break;
		    }
		} else {
		    Tcl_DStringAppend(&cmdbuffer, &ch, 1);
		    
		    if (ch == '\n' && 
			Tcl_CommandComplete(Tcl_DStringValue(&cmdbuffer))) {
			int result;

			result = ExecuteCommand(interp, &cmdbuffer);
			if (timeout == 0) {
			    status = 0; // send update
			}
		    }
		}
	    } else if (ufd[0].revents & POLLHUP) {
		trace("received hangup on stdout from client.");
		break;
	    }

	    if (ufd[1].revents & POLLIN) { 
		ssize_t nRead;
		char ch;

		/* This is supposed to suck up all the extra output from the
		 * pymol server output after we've matched the command
		 * prompt.  */

		/* pyMol Stdout Connection: pymol (unexpected) output */
		nRead = read(ufd[1].fd, &ch, 1);
		if (nRead <= 0) {
		    /* It's possible to have already drained the channel in
		     * response to a client command handled above. Skip
		     * it if we're blocking. */
		    if ((errno == EAGAIN) || (errno == EINTR)) {
			trace("try again to read (stdout) to pymol server.");
			goto nextchannel;
		    }
		    trace("lost connection (stdout) from pymol server.");
		    break;
		} else {
		    dyBufferAppend(&dybuffer, &ch, 1);
		    
		    if (ch == '\n') {
			ch = 0;
			dyBufferAppend(&dybuffer, &ch, 1);
			trace("STDOUT>%s",dybuffer.data);
			dyBufferSetLength(&dybuffer,0);
		    }
		}
	    } else if (ufd[1].revents & POLLHUP) { 
		trace("received hangup on stdout to pymol server.");
		break;
	    }
	nextchannel:
	    if (ufd[2].revents & POLLIN) { 
		ssize_t nRead;
		char ch;

		/* pyMol Stderr Connection: pymol standard error output */

		nRead = read(ufd[2].fd, &ch, 1);
		if (nRead <= 0) {
		    trace("unexpected read error from server (stderr): %s",
			  strerror(errno));
		    if (errno != EINTR) { 
			trace("lost connection (stderr) to pymol server.");
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
	    } else if (ufd[2].revents & POLLHUP) { 
		trace("received hangup on stderr from pymol server.");
		break;
	    }
	}
	
	if (status == 0) {
	    gettimeofday(&now,NULL);
	    
	    if (proxy.need_update && proxy.can_update)
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
        trace("error waiting on pymol server to exit: %s", strerror(errno));
    } else if (status == 0) {
        trace("attempting to signal (SIGTERM) pymol server.");
        kill(-pid, SIGTERM); // kill process group
        alarm(5);
        status = waitpid(pid, &result, 0);
        alarm(0);
	
        while ((status == -1) && (errno == EINTR)) {
	    trace("Attempting to signal (SIGKILL) pymol server.");
	    kill(-pid, SIGKILL); // kill process group
	    alarm(10);
	    status = waitpid(pid, &result, 0);
	    alarm(0); 
	}
    }
    
    trace("pymol server process ended (result=%d)", result);
    
    dyBufferFree(&proxy.image);
    
    Tcl_DeleteInterp(interp);
    if (WIFEXITED(result)) {
	result = WEXITSTATUS(result);
    }
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

