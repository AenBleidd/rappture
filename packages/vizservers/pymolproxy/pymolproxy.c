
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

/*
 *   +--------------+        +------------+         +--------+
 *   |              | output |            |  input  |        | 
 *   |              |------->|            |-------->|        |
 *   | molvisviewer |        | pymolproxy |  output | pymol  |
 *   |    client    |        |            |<--------| server | 
 *   |              | input  |            |  errors |        | 
 *   |              |<-------|            |<--------|        | 
 *   +--------------+        +------------+         +--------+
 * 
 * The communication between the pymol server and proxy is completely
 * synchronous.  The proxy relays/translates commands from the client to the
 * server and then wait for the responses.  It also watches the server's
 * stdout and stderr changes so that extraneous writes don't block the
 * the server. (I don't know exactly what those responses are)
 *
 * The communication between the client and the proxy is different.  The
 * client commands are read when they become available on the socket.  Proxy
 * writes to the client (image output) are non-blocking so that we don't
 * deadlock with the client.  The client may be busy sending the next command
 * when we want to write the resulting image from the last command.
 *
 * FIXME: Need to fix the reads from the pymol server.  We want to check for
 * an expected string within the pymol server output.  Right now it's doing
 * single character reads to stop when we hit a newline.  We should create a
 * read buffer that lets us do block reads.  This problem only really affects
 * the echoing back of commands (waiting for the pymol command prompt).  Most
 * command lines are small.  Still it's a good area for improvement.
 *
 * FIXME: Might be a problem if another image is received from the server
 *	  before the last one is transmitted.
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
    double cmdTime;		/* Elapsed time spend executing commands. */
    struct timeval start;	/* Start of elapsed time. */
} Stats;

static Stats stats;

static FILE *flog;
static int debug = 0;
#ifdef notdef
static long _flags = 0;
#endif

typedef struct {
    char *data;
    int   used;
    int   allocated;
    ssize_t nWritten;
    size_t bytesLeft;
} DyBuffer;

#define BUFFER_SIZE		4096

typedef struct {
    char bytes[BUFFER_SIZE];
    int fill;
    int mark;
    int fd;
} ReadBuffer;

#define BUFFER_OK		 0
#define BUFFER_ERROR		-1
#define BUFFER_CONTINUE		-2
#define BUFFER_SHORT_READ	-3

typedef struct {
    int serverInput;
    int serverOutput;
    int serverError;
    int clientInput;
    int clientOutput;
    DyBuffer image;
    Tcl_Interp *interp;
    int updatePending;
    int rotatePending;
    int scalePending;
    int forceUpdate;
    int can_update;
    int sync;
    int showLabels;
    int frame;
    int rock_offset;
    int cacheid;
    int invalidate_cache;
    int error;
    int status;
    ReadBuffer client;
    ReadBuffer server;
    float xAngle, yAngle, zAngle;
    float atomScale;
} PymolProxy;

static void PollForEvents(PymolProxy *proxyPtr);
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

static void
InitBuffer(ReadBuffer *readPtr, int f)
{
    readPtr->fd = f;
    readPtr->fill = 0;
    readPtr->mark = 0;
}

static void
FlushBuffer(ReadBuffer *readPtr)
{
    readPtr->fill = 0;
    readPtr->mark = 0;
}

static int
FillBuffer(ReadBuffer *readPtr)
{
    size_t bytesLeft;
    ssize_t nRead;

#ifdef notdef
    trace("Entering FillBuffer (mark=%d, fill=%d)\n", readPtr->mark, 
	  readPtr->fill);
#endif
    if (readPtr->mark >= readPtr->fill) {
	readPtr->mark = readPtr->fill = 0;
    }
    if (readPtr->mark > 0) {
	size_t i, j;

	for (i = 0, j = readPtr->mark; j < readPtr->fill; i++, j++) {
	    readPtr->bytes[i] = readPtr->bytes[j];
	}
	readPtr->mark = 0;
	readPtr->fill = i;
    }
    bytesLeft = BUFFER_SIZE - readPtr->fill - 1;
    nRead = read(readPtr->fd, readPtr->bytes + readPtr->fill, bytesLeft);
    if (nRead == 0) {
	return BUFFER_ERROR;
    }
    if (nRead <= 0) {
	if (errno != EAGAIN) {
#ifdef notdef
	    trace("in FillBuffer: read failed %d: %s", errno, strerror(errno));
	    trace("Leaving FillBuffer FAIL(read %d bytes) mark=%d, fill=%d\n", 
		  nRead, readPtr->mark, readPtr->fill);
#endif
	    return BUFFER_ERROR;
	}
	return BUFFER_SHORT_READ;
    }
    readPtr->fill += nRead;
#ifdef notdef
    trace("Leaving FillBuffer (read %d bytes) mark=%d, fill=%d\n", 
	  nRead, readPtr->mark, readPtr->fill);
#endif
    return (nRead == bytesLeft) ? BUFFER_OK : BUFFER_SHORT_READ;
}

static char *
GetLine(ReadBuffer *readPtr, int *nBytesPtr)
{
    int i;
    int status;

#ifdef notdef
    trace("Entering GetLine (mark=%d, fill=%d)\n",readPtr->mark, readPtr->fill);
#endif
    status = BUFFER_OK;
    for (;;) {
	/* Look for the next newline (the next full line). */
	trace("in GetLine: mark=%d fill=%d\n", readPtr->mark, readPtr->fill);
	for (i = readPtr->mark; i < readPtr->fill; i++) {
	    if (readPtr->bytes[i] == '\n') {
		char *p;
		
		/* Save the start of the line. */
		p = readPtr->bytes + readPtr->mark;
		i++;
		*nBytesPtr = i - readPtr->mark;
		readPtr->mark = i;
#ifdef notdef
		trace("Leaving GetLine(%.*s)\n", *nBytesPtr, p);
#endif
		return p;
	    }
	}
	/* Couldn't find a newline, so it may be that we need to read some
	 * more. Check first that last read wasn't a short read. */
	if (status == BUFFER_SHORT_READ) {
	    break;
	}
	status = FillBuffer(readPtr);
	if (status == BUFFER_ERROR) {
	    *nBytesPtr = BUFFER_ERROR;
	    return NULL;	/* EOF or error on read. */
	}
    }
#ifdef notdef
    trace("Leaving GetLine failed to read line\n");
#endif
    *nBytesPtr = BUFFER_CONTINUE;
    return NULL;
}

static int 
GetBytes(ReadBuffer *readPtr, char *out, int nBytes)
{
#ifdef notdef
    trace("Entering GetBytes(%d)\n", nBytes);
#endif
    while (nBytes > 0) {
	int bytesLeft;
	int status;

	bytesLeft = readPtr->fill - readPtr->mark;
	if (bytesLeft > 0) {
	    int size;

	    size = (bytesLeft >  nBytes) ? nBytes : bytesLeft;
	    memcpy(out, readPtr->bytes + readPtr->mark, size);
	    readPtr->mark += size;
	    nBytes -= size;
	    out += size;
	}
	if (nBytes == 0) {
	    /* Received requested # bytes. */
#ifdef notdef
	    trace("Leaving GetBytes(%d)\n", nBytes);
#endif
	    return BUFFER_OK;
	}
	/* Didn't get enough bytes, need to read some more. */
	status = FillBuffer(readPtr);
	if (status == BUFFER_ERROR) {
	    return BUFFER_ERROR;
	}
#ifdef notdef
	trace("in GetBytes: mark=%d fill=%d\n", readPtr->mark, readPtr->fill);
#endif
    }
#ifdef notdef
    trace("Leaving GetBytes(%d)\n", nBytes);
#endif
    return BUFFER_OK;
}

INLINE static void
clear_error(PymolProxy *proxyPtr)
{
    proxyPtr->error = 0;
    proxyPtr->status = TCL_OK;
}

static int
Expect(PymolProxy *proxyPtr, char *match, char *out, int maxSize)
{
    char c;
    size_t length;

    if (proxyPtr->status != TCL_OK) {
        return proxyPtr->status;
    }
    trace("Entering Expect(match=%s, maxSize=%d)\n", match, maxSize);
    c = match[0];
    length = strlen(match);
    for (;;) {
	int nBytes;
	char *line;

	line = GetLine(&proxyPtr->server, &nBytes);
	if (line != NULL) {
	    trace("pymol says:%.*s", nBytes, out);
	    if ((c == line[0]) && (strncmp(line, match, length) == 0)) {
		if (maxSize < nBytes) {
		    nBytes = maxSize;
		}
		memcpy(out, line, nBytes);
		clear_error(proxyPtr);
		trace("Leaving Expect: match is (%.*s)\n", nBytes, out);
		return BUFFER_OK;
	    }
	    continue;
	}
	if (nBytes != BUFFER_CONTINUE) {
	    return BUFFER_ERROR;
	}
    }
    trace("Leaving Expect: failed to find (%s)\n", match);
    proxyPtr->error = 2;
    proxyPtr->status = TCL_ERROR;
    return BUFFER_ERROR;
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
ExecuteCommand(Tcl_Interp *interp, const char *cmd) 
{
    struct timeval tv;
    double start, finish;
    int result;

    gettimeofday(&tv, NULL);
    start = CVT2SECS(tv);

    result = Tcl_Eval(interp, cmd);
    trace("Executed (%s)", cmd);

    gettimeofday(&tv, NULL);
    finish = CVT2SECS(tv);

    stats.cmdTime += finish - start;
    stats.nCommands++;
    return result;
}

INLINE static void
dyBufferInit(DyBuffer *imgPtr)
{
    imgPtr->data = NULL;
    imgPtr->used = 0;
    imgPtr->allocated = 0;
    imgPtr->nWritten = 0;
    imgPtr->bytesLeft = 0;
}

INLINE static void
dyBufferFree(DyBuffer *imgPtr)
{
    assert(imgPtr != NULL);
    if (imgPtr->data != NULL) {
	free(imgPtr->data);
    }
    dyBufferInit(imgPtr);
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

static void
WriteImage(PymolProxy *proxyPtr, int fd)
{
    DyBuffer *imgPtr; 
    ssize_t bytesLeft;

    imgPtr = &proxyPtr->image;
    trace("WriteImage: want to write %d bytes.", imgPtr->bytesLeft);
    for (bytesLeft = imgPtr->bytesLeft; bytesLeft > 0; /*empty*/) {
	ssize_t nWritten;

	trace("WriteImage: try to write %d bytes.", bytesLeft);
        nWritten = write(fd, imgPtr->data + imgPtr->nWritten, bytesLeft);
	trace("WriteImage: wrote %d bytes.", nWritten);
        if (nWritten < 0) {
	    trace("Error writing fd(%d), %d/%s.", fd, errno, 
		  strerror(errno));
	    return;
	}
	bytesLeft -= nWritten;
	if (bytesLeft > 0) {
	    /* Wrote a short buffer, means we would block. */
	    imgPtr->nWritten += nWritten;
	    imgPtr->bytesLeft = bytesLeft;
	    return;
	}
	imgPtr->nWritten += nWritten;
    }
    dyBufferFree(imgPtr);
}


#ifdef notdef
static int
ReadImage(PymolProxy *proxyPtr, int fd, size)
{
    int result, total, left;

    for( total = 0, left = size; left > 0; left -= result) {
	result = read(fd, buffer+total, left);
	
	if (result > 0) {
	    total += result;
	    continue;
	}
        
	if ((result < 0) && (errno != EAGAIN) && (errno != EINTR)) { 
	    trace("Error reading fd(%d), %d/%s.", fd, errno, 
		  strerror(errno));
	    break;
	}
	
	result = 0;
    }
    return total;
}
#endif

static int
bwrite(int fd, char *buffer, int size)
{
    int result;
    int total = 0;
    int left = size;

    trace("bwrite: want to write %d bytes.", size);
    while(1) {
        result = write(fd,buffer+total,left);

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
bread(int fd, char *buffer, int size)
{
    int result, total, left;

    for( total = 0, left = size; left > 0; left -= result) {
	result = read(fd, buffer+total, left);
	
	if (result > 0) {
	    total += result;
	    continue;
	}
        
	if ((result < 0) && (errno != EAGAIN) && (errno != EINTR)) { 
	    trace("Error reading fd(%d), %d/%s.", fd, errno, 
		  strerror(errno));
	    break;
	}
	
	result = 0;
    }
    return total;
}

#ifdef notdef

static int 
bflush(int fd, char *buffer, int size, int bytes)
{
    int bsize;

    while(bytes) {
	if (bytes > size)
	    bsize = size;
	else
	    bsize = bytes;
	
	bsize = bread(fd,buffer,bsize);
        
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
getline(int fd, char *buffer, int size, int timeout)
{
    int pos = 0, status, timeo;
    struct timeval now, end, tmo;
    struct pollfd ufd;

    gettimeofday(&now,NULL);
    timerset_ms(&tmo, timeout);
    timeradd(&now,&tmo,&end);

    ufd.fd = fd;
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
            status = read(fd,&buffer[pos],1);

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
Pymol(PymolProxy *proxyPtr, char *format, ...)
{

    if (proxyPtr->error) {
        return TCL_ERROR;
    }
    {
	va_list ap;
	char cmd[BUFSIZ];
	ssize_t nWritten;
	size_t length;

	va_start(ap, format);
	vsnprintf(cmd, BUFSIZ-1, format, ap);
	va_end(ap);
	
	trace("to-pymol>(%s)", cmd);
	
	/* Write the command out to the server. */
	length = strlen(cmd);
	nWritten = write(proxyPtr->serverInput, cmd, length);
	if (nWritten != length) {
	    trace("short write to pymol (wrote=%d, should have been %d)",
		  nWritten, length);
	}
    }
    {
	char out[BUFSIZ];
	int result;

	/* Now wait for the "PyMOL>" prompt. */
	result = Expect(proxyPtr, "PyMOL>", out, BUFSIZ);
	if (result == BUFFER_ERROR) {
	    trace("timeout reading data (buffer=%s)", out);
	    proxyPtr->error = 1;
	    proxyPtr->status = TCL_ERROR;
	    return proxyPtr->status;
	}
	return  proxyPtr->status;
    }
}

static void
DoRotate(PymolProxy *proxyPtr)
{
    if (proxyPtr->rotatePending) {
	/* Every pymol command line generates a new rendering. Execute all
	 * three turns as a single command line. */
	Pymol(proxyPtr,"turn x,%f; turn y,%f; turn z,%f\n", 
	      proxyPtr->xAngle, proxyPtr->yAngle, proxyPtr->zAngle);
	proxyPtr->xAngle = proxyPtr->yAngle = proxyPtr->zAngle = 0.0f;
	proxyPtr->rotatePending = FALSE;
    }
}

static void
DoAtomScale(PymolProxy *proxyPtr)
{
    if (proxyPtr->scalePending) {
	proxyPtr->scalePending = FALSE;
	Pymol(proxyPtr, "set sphere_scale,%f,all\n", proxyPtr->atomScale);
    }
}

static int
AtomscaleCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    int defer = 0, push = 0, arg;
    double scale;
    const char *model = "all";
    PymolProxy *proxyPtr = clientData;

    clear_error(proxyPtr);
    scale = 0.25f;
    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 ) {
            defer = 1;
	} else if (strcmp(argv[arg],"-push") == 0) {
            push = 1;
	} else if (strcmp(argv[arg],"-model") == 0) {
            if (++arg < argc)
                model = argv[arg];
        } else {
	    if (Tcl_GetDouble(interp, argv[arg], &scale) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    proxyPtr->invalidate_cache = TRUE;  /* Spheres */
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;

    if (strcmp(model, "all") == 0) {
	proxyPtr->scalePending = TRUE;
	proxyPtr->atomScale = scale;
    } else {
	Pymol(proxyPtr, "set sphere_scale,%f,%s\n", scale, model);
    }
    return proxyPtr->status;
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

    proxyPtr->invalidate_cache = TRUE;  /* Ball 'n Stick */
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;

    Pymol(proxyPtr, "hide everything,%s\n",model);
    Pymol(proxyPtr, "set stick_color,white,%s\n",model);
    if (ghost) {
	radius = 0.1f;
	transparency = 0.75f;
    } else {
	radius = 0.14f;
	transparency = 0.0f;
    }
    Pymol(proxyPtr, "set stick_radius,%g,%s\n", radius, model);
#ifdef notdef
    Pymol(proxyPtr, "set sphere_scale=0.25,%s\n", model);
#endif
    Pymol(proxyPtr, "set sphere_transparency,%g,%s\n", transparency, model);
    Pymol(proxyPtr, "set stick_transparency,%g,%s\n", transparency, model);
    Pymol(proxyPtr, "show sticks,%s\n", model);
    Pymol(proxyPtr, "show spheres,%s\n", model);

    if (proxyPtr->showLabels) {
        Pymol(proxyPtr, "show labels,%s\n", model);
    }
    return proxyPtr->status;
}


static int
BmpCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    char buffer[BUFSIZ];
    unsigned int nBytes=0;
    PymolProxy *proxyPtr = clientData;
    DyBuffer *imgPtr; 
    int nScanned;
    size_t length;
#ifdef notdef
    float samples = 10.0;
    ssize_t nWritten;
#endif
    clear_error(proxyPtr);

    if (proxyPtr->invalidate_cache)
        proxyPtr->cacheid++;

    proxyPtr->updatePending = FALSE;
    proxyPtr->forceUpdate = FALSE;
    proxyPtr->invalidate_cache = FALSE;

   /* Force pymol to update the current scene. */
    Pymol(proxyPtr,"refresh\n");
    Pymol(proxyPtr,"bmp -\n");
    if (Expect(proxyPtr, "image follows: ", buffer, BUFSIZ) != BUFFER_OK) {
        trace("can't find image follows line (%s)", buffer);
    }
    nScanned = sscanf(buffer, "image follows: %d\n", &nBytes);
    if (nScanned != 1) {
        trace("can't scan image follows buffer (%s)", buffer);
	return TCL_ERROR;

    }
#ifdef notdef
    nWritten = write(3,&samples,sizeof(samples));
#endif
    sprintf(buffer, "nv>image %d %d %d %d\n", nBytes, proxyPtr->cacheid, 
	    proxyPtr->frame, proxyPtr->rock_offset);

    length = strlen(buffer);
    imgPtr = &proxyPtr->image;
    dyBufferSetLength(imgPtr, nBytes + length);
    strcpy(imgPtr->data, buffer);
    if (GetBytes(&proxyPtr->server, imgPtr->data + length, nBytes)!=BUFFER_OK){
        trace("can't read %d bytes for \"image follows\" buffer", nBytes);
	return  TCL_ERROR;
    }
    imgPtr->bytesLeft = nBytes + length;
    imgPtr->nWritten = 0;
    stats.nFrames++;
    stats.nBytes += nBytes;
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

    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->invalidate_cache = TRUE;  /* Disable */

    Pymol( proxyPtr, "disable %s\n", model);

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

    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->invalidate_cache = TRUE;  /* Enable */

    Pymol( proxyPtr, "enable %s\n", model);

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
                
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->frame = frame;

    /* Does not invalidate cache? */

    Pymol(proxyPtr,"frame %d\n", frame);
        
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
            state =  !proxyPtr->showLabels;
    }

    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->invalidate_cache = TRUE;  /* Label */

    if (state) {
        Pymol(proxyPtr, "set label_color,white,all\n");
        Pymol(proxyPtr, "set label_size,14,all\n");
        Pymol(proxyPtr, "label all,\"%%s%%s\" %% (ID,name)\n");
    }
    else
        Pymol(proxyPtr, "label all\n");

    proxyPtr->showLabels = state;

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

    proxyPtr->invalidate_cache = TRUE;  /* Lines */
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;

    Pymol(proxyPtr, "hide everything,%s\n",model);

    lineWidth = (ghost) ? 0.25f : 1.0f;
    Pymol(proxyPtr, "set line_width,%g,%s\n", lineWidth, model);
    Pymol(proxyPtr, "show lines,%s\n", model);
    if (proxyPtr->showLabels) {
        Pymol(proxyPtr, "show labels,%s\n", model);
    }
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
    
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;

    /* Does not invalidate cache? */

    {
	char fileName[200];
	FILE *f;
	size_t nBytes;
	ssize_t nWritten;

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
	Pymol(proxyPtr, "load %s, %s, %d\n", fileName, name, state);
	Pymol(proxyPtr, "zoom complete=1\n");
    }
    return proxyPtr->status;
}

static int
OrthoscopicCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	      const char *argv[])
{
    int bool, defer, push, i;
    PymolProxy *proxyPtr = clientData;

    clear_error(proxyPtr);
    defer = push = FALSE;
    bool = FALSE;
    for(i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-defer") == 0) {
            defer = TRUE;
	} else if (strcmp(argv[i],"-push") == 0) {
            push = TRUE;
	} else { 
	    if (Tcl_GetBoolean(interp, argv[i], &bool) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    proxyPtr->invalidate_cache = TRUE;  
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    Pymol(proxyPtr, "set orthoscopic=%d\n", bool);
    return proxyPtr->status;
}

/* 
 * PanCmd --
 *
 *	Issue "move" commands for changes in the x and y coordinates of the
 *	camera.  The problem here is that there is no event compression.
 *	Consecutive "pan" commands are not compressed into a single
 *	directive.  The means that the pymol server will render scenes that
 *	are not used by the client.
 *
 *	Need to 1) defer the "move" commands until we find the next command
 *	isn't a "pan". 2) Track the x and y coordinates as they are
 *	compressed.
 */
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
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->invalidate_cache = TRUE;  /* Pan */

    if ((x != 0.0f) || (y != 0.0f)) {
	/* Every pymol command line generates a new rendering. Execute both
	 * moves as a single command line. */
	Pymol(proxyPtr,"move x,%f; move y,%f\n", x * 0.05, -y * 0.05);
    }
    return proxyPtr->status;
}

static int
PngCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    char buffer[800];
    unsigned int nBytes=0;
    PymolProxy *proxyPtr = clientData;
#ifdef notdef
    float samples = 10.0;
    ssize_t nWritten;
#endif
    clear_error(proxyPtr);

    if (proxyPtr->invalidate_cache)
        proxyPtr->cacheid++;

    proxyPtr->updatePending = FALSE;
    proxyPtr->forceUpdate = FALSE;
    proxyPtr->invalidate_cache = FALSE;

   /* Force pymol to update the current scene. */
    Pymol(proxyPtr,"refresh\n");

    Pymol(proxyPtr,"png -\n");

    Expect(proxyPtr, "image follows: ", buffer, 800);

    sscanf(buffer, "image follows: %d\n", &nBytes);
 
#ifdef notdef
    nWritten = write(3, &samples, sizeof(samples));
#endif
    dyBufferSetLength(&proxyPtr->image, nBytes);

    bread(proxyPtr->serverOutput, proxyPtr->image.data, proxyPtr->image.used);
    proxyPtr->image.bytesLeft = proxyPtr->image.used;
    proxyPtr->image.nWritten = 0;

    Expect(proxyPtr, " ScenePNG", buffer,800);

    if ((nBytes > 0) && (proxyPtr->image.used == nBytes)) {
	ssize_t nWritten;

        sprintf(buffer, "nv>image %d %d %d %d\n", 
                nBytes, proxyPtr->cacheid, proxyPtr->frame, proxyPtr->rock_offset);
        trace("to-molvis> %s", buffer);
	/* Write the header */
        nWritten = write(proxyPtr->clientInput, buffer, strlen(buffer));
	/* and the data. */
        bwrite(proxyPtr->clientInput, proxyPtr->image.data, nBytes);

	stats.nFrames++;
	stats.nBytes += nBytes;
    }
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

    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->invalidate_cache = TRUE;  /* Raw */

    Pymol(proxyPtr,"%s\n", cmd);
    dyBufferFree(&buffer);

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
                
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->invalidate_cache = TRUE;  /* Reset */
        
    Pymol(proxyPtr, "reset\n");
    Pymol(proxyPtr, "zoom complete=1\n");

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
                
    /* Does not invalidate cache. */

    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;

    Pymol(proxyPtr,"turn y, %f\n", y - proxyPtr->rock_offset);

    proxyPtr->rock_offset = y;

    return proxyPtr->status;
}

/* 
 * RotateCmd --
 *
 *	Issue "turn" commands for changes in the angle of the camera.  The
 *	problem here is that there is no event compression.  Consecutive
 *	"rotate" commands are not compressed into a single directive.  The
 *	means that the pymol server will render many scene that are not used
 *	by the client.
 *
 *	Need to 1) defer the "turn" commands until we find the next command
 *	isn't a "rotate". 2) Track the rotation angles as they are compressed.
 */
static int
RotateCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	  const char *argv[])
{
    float xAngle, yAngle, zAngle;
    PymolProxy *proxyPtr = clientData;
    int defer = 0, push = 0, arg, varg = 1;

    clear_error(proxyPtr);

    xAngle = yAngle = zAngle = 0.0f;
    for(arg = 1; arg < argc; arg++) {
	if (strcmp(argv[arg],"-defer") == 0) {
	    defer = 1; 
	} else if (strcmp(argv[arg],"-push") == 0) {
	    push = 1;
	} else  if (varg == 1) {
	    double value;
	    if (Tcl_GetDouble(interp, argv[arg], &value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    xAngle = (float)value;
	    varg++;
	} else if (varg == 2) {
	    double value;
	    if (Tcl_GetDouble(interp, argv[arg], &value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    yAngle = (float)value;
	    varg++;
	} else if (varg == 3) {
	    double value;
	    if (Tcl_GetDouble(interp, argv[arg], &value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    zAngle = (float)value;
	    varg++;
	}
    } 
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->invalidate_cache = TRUE;  /* Rotate */

    if ((xAngle != 0.0f) || (yAngle != 0.0f) || (zAngle != 0.0f)) {
	proxyPtr->xAngle += xAngle;
	proxyPtr->yAngle += yAngle;
	proxyPtr->zAngle += zAngle;
	proxyPtr->rotatePending = TRUE;
    }
    return proxyPtr->status;
}

static int
SpheresCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    int defer = 0, ghost = 0, push = 0, arg;
    float scale;
    const char *model = "all";
    PymolProxy *proxyPtr = clientData;

    clear_error(proxyPtr);
    scale = 0.41f;
    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 ) {
            defer = 1;
	} else if (strcmp(argv[arg],"-push") == 0) {
            push = 1;
	} else if (strcmp(argv[arg],"-ghost") == 0) {
            ghost = 1;
	} else if (strcmp(argv[arg],"-normal") == 0) {
            ghost = 0;
	} else if (strcmp(argv[arg],"-model") == 0) {
            if (++arg < argc)
                model = argv[arg];
        } else if (strcmp(argv[arg],"-scale") == 0) {
            if (++arg < argc)
                scale = atof(argv[arg]);
        }
        else
            model = argv[arg];
    }

    proxyPtr->invalidate_cache = TRUE;  /* Spheres */
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;

    Pymol(proxyPtr, "hide everything, %s\n", model);
#ifdef notdef
    Pymol(proxyPtr, "set sphere_scale,%f,%s\n", scale, model);
#endif
    //Pymol(proxyPtr, "set sphere_quality,2,%s\n", model);
    Pymol(proxyPtr, "set ambient,.2,%s\n", model);

    if (ghost)
        Pymol(proxyPtr, "set sphere_transparency,.75,%s\n", model);
    else
        Pymol(proxyPtr, "set sphere_transparency,0,%s\n", model);

    Pymol(proxyPtr, "show spheres,%s\n", model);

    if (proxyPtr->showLabels) {
        Pymol(proxyPtr, "show labels,%s\n", model);
    }
    return proxyPtr->status;
}

static int
ScreenCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
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

    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->invalidate_cache = TRUE;  /* viewport */

    Pymol(proxyPtr, "viewport %d,%d\n", width, height);

    //usleep(205000); // .2s delay for pymol to update its geometry *HACK ALERT*
        
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

    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->invalidate_cache = TRUE;  /* vmouse */

    Pymol(proxyPtr, "vmouse %d,%d,%d,%d,%d\n", arg1, arg2, arg3, 
		    arg4, arg5);

    return proxyPtr->status;
}


/* 
 * ZoomCmd --
 *
 *	Issue "move" commands for changes in the z-coordinate of the camera.
 *	The problem here is that there is no event compression.  Consecutive
 *	"zoom" commands are not compressed into a single directive.  The means
 *	that the pymol server will render scenes that are not used by the
 *	client.
 *
 *	Need to 1) defer the "move" commands until we find the next command
 *	isn't a "zoom". 2) Track the z-coordinate as they are compressed.
 */
static int
ZoomCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
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
	    double value;
	    if (Tcl_GetDouble(interp, argv[arg], &value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    factor = (float)value;
	    varg++;
	}
    }
    proxyPtr->updatePending = !defer || push;
    proxyPtr->forceUpdate = push;
    proxyPtr->invalidate_cache = TRUE; /* Zoom */

    if (factor != 0.0) {
        Pymol(proxyPtr,"move z,%f\n", factor);
    }
    return proxyPtr->status;
}

        
static int 
ProxyInit(int c_in, int c_out, char *const *argv)
{
    int status, result = 0;
    int serverIn[2];
    int serverOut[2];
    int serverErr[2];
    Tcl_Interp *interp;
    int child;
    PymolProxy proxy;
    struct timeval end;

    /* Create three pipes for communication with the external application. One
     * each for the applications's: stdin, stdout, and stderr  */

    if (pipe(serverIn) == -1)
        return -1;

    if (pipe(serverOut) == -1) {
        close(serverIn[0]);
        close(serverIn[1]);
        return -1;
    }

    if (pipe(serverErr) == -1) {
        close(serverIn[0]);
        close(serverIn[1]);
        close(serverOut[0]);
        close(serverOut[1]);
        return -1;
    }

    /* Fork the new process.  Connect i/o to the new socket.  */

    child = fork();
        
    if (child < 0) {
        fprintf(stderr, "can't fork process: %s\n", strerror(errno));
        return -3;
    }
    if (child == 0) {
        int f;

        /* Child process */
        
        /* 
         * Create a new process group, so we can later kill this process and
         * all its children without affecting the process that created this
         * one.
         */
        setpgid(child, 0); 
        
        /* Redirect stdin, stdout, and stderr to pipes before execing */ 

        dup2(serverIn[0], 0);  // stdin
        dup2(serverOut[1],1);  // stdout
        dup2(serverErr[1],2);  // stderr
        
	/* Close all other descriptors  */        
	for (f = 3; f < FD_SETSIZE; f++) {
            close(f);
	}
        
        execvp(argv[0], argv);
        trace("Failed to start pymol `%s'", argv[0]);
	exit(-1);
    }
    stats.child = child;

    /* close opposite end of pipe, these now belong to the child process  */
    close(serverIn[0]);
    close(serverOut[1]);
    close(serverErr[1]);

    signal(SIGPIPE, SIG_IGN); /* ignore SIGPIPE (e.g. nanoscale terminates)*/

    proxy.serverInput  = serverIn[1];
    proxy.serverOutput = serverOut[0];
    proxy.serverError = serverErr[0];
    proxy.clientInput  = c_in;
    proxy.clientOutput = c_out;
    proxy.showLabels = FALSE;
    proxy.updatePending = FALSE;
    proxy.forceUpdate = FALSE;
    proxy.rotatePending = FALSE;
    proxy.scalePending = FALSE;
    proxy.can_update = TRUE;
    proxy.sync = 0;
    proxy.frame = 1;
    proxy.rock_offset = 0;
    proxy.cacheid = 0;
    proxy.invalidate_cache = FALSE;
    interp = Tcl_CreateInterp();
    Tcl_MakeSafe(interp);
    proxy.interp = interp;
    proxy.xAngle = proxy.yAngle = proxy.zAngle = 0.0f;
    dyBufferInit(&proxy.image);

    Tcl_CreateCommand(interp, "atomscale",   AtomscaleCmd,   &proxy, NULL);
    Tcl_CreateCommand(interp, "ballnstick",  BallNStickCmd,  &proxy, NULL);
    Tcl_CreateCommand(interp, "bmp",         BmpCmd,         &proxy, NULL);
    Tcl_CreateCommand(interp, "disable",     DisableCmd,     &proxy, NULL);
    Tcl_CreateCommand(interp, "enable",      EnableCmd,      &proxy, NULL);
    Tcl_CreateCommand(interp, "frame",       FrameCmd,       &proxy, NULL);
    Tcl_CreateCommand(interp, "label",       LabelCmd,       &proxy, NULL);
    Tcl_CreateCommand(interp, "lines",       LinesCmd,       &proxy, NULL);
    Tcl_CreateCommand(interp, "loadpdb",     LoadPDBCmd,     &proxy, NULL);
    Tcl_CreateCommand(interp, "orthoscopic", OrthoscopicCmd, &proxy, NULL);
    Tcl_CreateCommand(interp, "pan",         PanCmd,         &proxy, NULL);
    Tcl_CreateCommand(interp, "png",         PngCmd,         &proxy, NULL);
    Tcl_CreateCommand(interp, "raw",         RawCmd,         &proxy, NULL);
    Tcl_CreateCommand(interp, "reset",       ResetCmd,       &proxy, NULL);
    Tcl_CreateCommand(interp, "rock",        RockCmd,        &proxy, NULL);
    Tcl_CreateCommand(interp, "rotate",      RotateCmd,      &proxy, NULL);
    Tcl_CreateCommand(interp, "screen",      ScreenCmd,      &proxy, NULL);
    Tcl_CreateCommand(interp, "spheres",     SpheresCmd,     &proxy, NULL);
    Tcl_CreateCommand(interp, "viewport",    ScreenCmd,      &proxy, NULL);
    Tcl_CreateCommand(interp, "vmouse",      VMouseCmd,      &proxy, NULL);
    Tcl_CreateCommand(interp, "zoom",        ZoomCmd,        &proxy, NULL);


    gettimeofday(&end, NULL);
    stats.start = end;

    // Main Proxy Loop
    //  accept tcl commands from socket
    //  translate them into pyMol commands, and issue them to child proccess
    //  send images back
    PollForEvents(&proxy);

    close(proxy.clientOutput);
    close(proxy.serverOutput);
    close(proxy.serverError);
    close(proxy.clientInput);
    close(proxy.serverInput);

    status = waitpid(child, &result, WNOHANG);
    if (status == -1) {
        trace("error waiting on pymol server to exit: %s", strerror(errno));
    } else if (status == 0) {
        trace("attempting to signal (SIGTERM) pymol server.");
        kill(-child, SIGTERM); // kill process group
        alarm(5);
        status = waitpid(child, &result, 0);
        alarm(0);
	
        while ((status == -1) && (errno == EINTR)) {
	    trace("Attempting to signal (SIGKILL) pymol server.");
	    kill(-child, SIGKILL); // kill process group
	    alarm(10);
	    status = waitpid(child, &result, 0);
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
	sprintf(fileName, "/tmp/pymolproxy.log");
        flog = fopen(fileName, "w");
    }    
    ProxyInit(fileno(stdout), fileno(stdin), argv + 1);
    return 0;
}

#endif


static void
PollForEvents(PymolProxy *proxyPtr)
{
    DyBuffer *imgPtr; 
    Tcl_DString clientCmds;
    struct pollfd pollResults[4];
    int flags;

    flags = fcntl(proxyPtr->clientInput, F_GETFL);
    fcntl(proxyPtr->clientInput, F_SETFL, flags|O_NONBLOCK);

#ifdef notdef
    flags = fcntl(proxyPtr->serverOutput, F_GETFL);
    fcntl(proxyPtr->serverOutput, F_SETFL, flags|O_NONBLOCK);
#endif

    pollResults[0].fd = proxyPtr->clientOutput;
    pollResults[1].fd = proxyPtr->serverOutput;
    pollResults[2].fd = proxyPtr->serverError;
    pollResults[0].events = pollResults[1].events = 
	pollResults[2].events = POLLIN;

    pollResults[3].fd = proxyPtr->clientInput;
    pollResults[3].events = POLLOUT;

    InitBuffer(&proxyPtr->client, proxyPtr->clientOutput);
    InitBuffer(&proxyPtr->server, proxyPtr->serverOutput);

    Tcl_DStringInit(&clientCmds);
    imgPtr = &proxyPtr->image;
    for (;;) {
	int timeout, nChannels;

	nChannels =  (imgPtr->bytesLeft > 0) ? 4 : 3;

#define PENDING_TIMEOUT		10  /* milliseconds. */
	timeout = (proxyPtr->updatePending) ? PENDING_TIMEOUT : -1;
	nChannels = poll(pollResults, nChannels, timeout);
	if (nChannels < 0) {
	    trace("POLL ERROR: %s", strerror(errno));
	    continue;		/* or exit? */
	}

	/* 
	 * The next two sections are to drain any leftover data in 
	 * the pymol server process' stdout or stderr.  We don't want the
	 * the pymol server to block writing to stderr or stdout.
	 */
	if (pollResults[1].revents & POLLIN) { 
	    int nBytes;
	    char *line;
	    
	    trace("Reading pymol stdout\n");
	    /* Don't care what's in the server output buffer. */
	    FlushBuffer(&proxyPtr->server);
	    line = GetLine(&proxyPtr->server, &nBytes);
	    if (line != NULL) {
		trace("STDOUT>%.*s", nBytes, line);
		trace("Done with pymol stdout\n");
	    } else if (nBytes == BUFFER_CONTINUE) {
		trace("Done with pymol stdout\n");
	    } else {
		trace("Failed reading pymol stdout (nBytes=%d)\n", nBytes);
		goto error;	/* Get out on EOF or error. */
	    }
	}

	if (pollResults[2].revents & POLLIN) { 
	    ssize_t nRead;
	    char buf[BUFSIZ];
	    
	    trace("Reading pymol stderr\n");
	    /* pyMol Stderr Connection: pymol standard error output */
	    
	    nRead = read(pollResults[2].fd, buf, BUFSIZ-1);
	    if (nRead <= 0) {
		trace("unexpected read error from server (stderr): %s",
		      strerror(errno));
		if (errno != EINTR) { 
		    trace("lost connection (stderr) to pymol server.");
		    return;
		}
	    }
	    buf[nRead] = '\0';
	    trace("stderr>%s", buf);
	    trace("Done reading pymol stderr\n");
	}

	/* We have some descriptors ready. */
	if (pollResults[0].revents & POLLIN) { 
	    trace("Reading client stdout\n");
	    for (;;) {
		int nBytes;
		char *line;
		
		line = GetLine(&proxyPtr->client, &nBytes);
		if (line != NULL) {
		    const char *cmd;

		    Tcl_DStringAppend(&clientCmds, line, nBytes);
		    cmd = Tcl_DStringValue(&clientCmds);
		    if (Tcl_CommandComplete(cmd)) {
			/* May execute more than one command. */
			ExecuteCommand(proxyPtr->interp, cmd);
			Tcl_DStringSetLength(&clientCmds, 0);
		    }
		    continue;
		}
		if (nBytes == BUFFER_CONTINUE) {
		    break;
		}
		trace("Failed reading client stdout (nBytes=%d)\n", nBytes);
		goto error;		/* Get out on EOF or error. */
	    }
	    trace("done with client stdout\n");
	}
	/* 
	 * Write the currently queued image if there is one.
	 *
	 * We want to transmit the current image back to the client.  But if
	 * the client's busy (e.g. sending us another command), there's a
	 * chance we'll deadlock.  Therefore, the file descriptor is
	 * non-blocking and we write only what we can.  Must be careful not to
	 * refresh the image until we're done.
	 */

	if (proxyPtr->rotatePending) {
	    DoRotate(proxyPtr);
	}
	if (proxyPtr->scalePending) {
	    DoAtomScale(proxyPtr);
	}

	/* Write the current image buffer. */
	if (imgPtr->nWritten == 0) {
	    /* We might want to refresh the image if we're not currently
	     * transmitting an image back to the client. The image will be
	     * refreshed after the image has been completely transmitted. */
	    if ((nChannels == 0) || (proxyPtr->forceUpdate)) {
		if (proxyPtr->updatePending) {
		    Tcl_Eval(proxyPtr->interp, "bmp");
		    proxyPtr->updatePending = FALSE;
		}
		proxyPtr->forceUpdate = FALSE;
		continue;
	    }
	}
	if ((imgPtr->bytesLeft > 0) && (pollResults[3].revents & POLLOUT)) { 
	    WriteImage(proxyPtr, pollResults[3].fd);
	}
    }
 error:
    Tcl_DStringFree(&clientCmds);
    return;
}
