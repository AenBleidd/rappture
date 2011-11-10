
/*
 * ----------------------------------------------------------------------
 * proxypymol.c
 *
 *      This module creates the Tcl interface to the pymol server.  It acts as
 *      a go-between establishing communication between a molvisviewer widget
 *      and the pymol server. The communication protocol from the molvisviewer
 *      widget is the Tcl language.  Commands are then relayed to the pymol
 *      server.  Responses from the pymol server are translated into Tcl
 *      commands and send to the molvisviewer widget. For example, resulting
 *      image rendered offscreen is returned as ppm-formatted image data.
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

/*
 * Notes:   The proxy should not maintain any state information from the
 *	    client, other that what it needs for event (rotate, pan, zoom,
 *	    atom scale, bond thickness, etc.) compression.  This is because 
 *	    the connection is periodically broken (timeout, error, etc.).
 *	    It's the responsibility of the client (molvisviewer) to restore 
 *	    the settings of the previous view.  The proxy is just a relay
 *	    between the client and the pymol server.
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
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <syslog.h>
#include <unistd.h>
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

#define READTRACE	1
#define EXPECTTRACE	0
#define WRITETRACE	0

static int debug = FALSE;

static FILE *flog;
static FILE *scriptFile;
static int savescript = FALSE;

typedef struct Image {
    struct Image *nextPtr;		/* Next image in chain of images. The
					 * list is ordered by the most
					 * recently received image from the
					 * pymol server to the least. */
    struct Image *prevPtr;		/* Previous image in chain of
					 * images. The list is ordered by the
					 * most recently received image from
					 * the pymol server to the least. */
    ssize_t nWritten;			/* Number of bytes of image data
					 * already delivered.*/
    size_t bytesLeft;			/* Number of bytes of image data left
					 * to delivered to the client. */
    char data[1];			/* Start of image data. We allocate
					 * the size of the Image structure
					 * plus the size of the image data. */
} Image;

#define BUFFER_SIZE		4096

typedef struct {
    const char *ident;
    char bytes[BUFFER_SIZE];
    int fill;
    int mark;
    int fd;
} ReadBuffer;

#define BUFFER_OK		 0
#define BUFFER_ERROR		-1
#define BUFFER_CONTINUE		-2

#define FORCE_UPDATE		(1<<0)
#define CAN_UPDATE		(1<<1)
#define INVALIDATE_CACHE	(1<<3)
#define ATOM_SCALE_PENDING	(1<<4)
#define STICK_RADIUS_PENDING	(1<<5)
#define ROTATE_PENDING		(1<<6)
#define PAN_PENDING		(1<<7)
#define ZOOM_PENDING		(1<<8)
#define UPDATE_PENDING		(1<<9)
#define VIEWPORT_PENDING	(1<<10)

typedef struct {
    Tcl_Interp *interp;
    unsigned int flags;			/* Various flags. */
    Image *headPtr, *tailPtr;		/* List of images to be delivered to
					 * the client.  The most recent images
					 * are in the front of the list. */

    int sin, sout;			/* Server file descriptors. */
    int cin, cout;			/* Client file descriptors. */
    ReadBuffer client;			/* Read buffer for client input. */
    ReadBuffer server;			/* Read buffer for server output. */
    int frame;
    int rockOffset;
    int cacheId;
    int error;
    int status;
    int width, height;			/* Size of viewport. */
    float xAngle, yAngle, zAngle;	/* Euler angles of pending
					 * rotation.  */
    float sphereScale;			/* Atom scale of pending re-scale. */
    float stickRadius;			/* Bond thickness of pending
					 * re-scale. */
    float zoom;
    float xPan, yPan;
} PymolProxy;

#define ERROR(...)      LogMessage(LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
#define TRACE(...)      LogMessage(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define WARN(...)       LogMessage(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)       LogMessage(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)

static const char *syslogLevels[] = {
    "emergency",			/* System is unusable */
    "alert",				/* Action must be taken immediately */
    "critical",				/* Critical conditions */
    "error",				/* Error conditions */
    "warning",				/* Warning conditions */
    "notice",				/* Normal but significant condition */
    "info",				/* Informational */
    "debug",				/* Debug-level messages */
};

void
LogMessage(int priority, const char *path, int lineNum, const char* fmt, ...)
{
#define MSG_LEN (2047)
    char message[MSG_LEN+1];
    const char *s;
    int length;
    va_list lst;

    va_start(lst, fmt);
    s = strrchr(path, '/');
    if (s == NULL) {
        s = path;
    } else {
        s++;
    }
    length = snprintf(message, MSG_LEN, "pymolproxy (%d) %s: %s:%d ", 
		      getpid(), syslogLevels[priority],  s, lineNum);
    length += vsnprintf(message + length, MSG_LEN - length, fmt, lst);
    message[MSG_LEN] = '\0';
    if (debug) {
	fprintf(stderr, "%s\n", message);
    } else {
	syslog(priority, message, length);
    }
}

static void PollForEvents(PymolProxy *proxyPtr);
static void
Debug TCL_VARARGS_DEF(const char *, arg1)
{
    if (debug) {
        const char *format;
        va_list args;

        format = TCL_VARARGS_START(const char *, arg1, args);
        fprintf(flog, "pymolproxy: ");
        vfprintf(flog, format, args);
        fprintf(flog, "\n");
        fflush(flog);
    }
}

static void
script TCL_VARARGS_DEF(const char *, arg1)
{
    if (savescript) {
        const char *format;
        va_list args;

        format = TCL_VARARGS_START(const char *, arg1, args);
        vfprintf(scriptFile, format, args);
        fflush(scriptFile);
    }
}

static void
InitBuffer(ReadBuffer *readPtr, const char *ident, int f)
{
    readPtr->ident = ident;
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

/*  
 * FillBuffer --
 *
 *	Fills the buffer with available data.  Any existing data
 *	in the buffer is slide to the front of the buffer, then
 *	the channel is read to fill the rest of the buffer.
 *
 *	If EOF or an error occur when reading the channel, then 
 *	BUFFER_ERROR is returned. If the buffer can't be filled, 
 *	then BUFFER_CONTINUE is returned.  
 *
 */
static int
FillBuffer(ReadBuffer *readPtr)
{
    size_t bytesLeft;
    ssize_t nRead;

#if READTRACE
    Debug("Entering FillBuffer (mark=%d, fill=%d)\n", readPtr->mark, 
	  readPtr->fill);
#endif
    if (readPtr->mark >= readPtr->fill) {
	INFO("tossing full buffer mark=%d, fill=%d", readPtr->mark, 
	     readPtr->fill);
	readPtr->mark = readPtr->fill = 0;
    }
    if (readPtr->mark > 0) {
	size_t i, j;

	/* Some data has been consumed. Move the unconsumed data to the front
	 * of the buffer. */
	for (i = 0, j = readPtr->mark; j < readPtr->fill; i++, j++) {
	    readPtr->bytes[i] = readPtr->bytes[j];
	}
	readPtr->mark = 0;
	readPtr->fill = i;
    }
    bytesLeft = BUFFER_SIZE - readPtr->fill - 1;
    nRead = read(readPtr->fd, readPtr->bytes + readPtr->fill, bytesLeft);
    if (nRead == 0) {
	INFO("EOF found reading \"%s\" buffer", readPtr->ident);
	return BUFFER_ERROR;		/* EOF, treat as an error. */
    }
    if (nRead < 0) {
	if (errno != EAGAIN) {
#if READTRACE
	    Debug("in FillBuffer: read failed %d: %s", errno, strerror(errno));
	    Debug("Leaving FillBuffer FAIL(read %d bytes) mark=%d, fill=%d\n", 
		  nRead, readPtr->mark, readPtr->fill);
#endif
	    ERROR("error reading \"%s\" buffer: %s", readPtr->ident, 
		  strerror(errno));
	    return BUFFER_ERROR;
	}
	INFO("Short read for \"%s\" buffer", readPtr->ident);
	return BUFFER_CONTINUE;
    }
    readPtr->fill += nRead;
#if READTRACE
    Debug("Leaving FillBuffer (read %d bytes) mark=%d, fill=%d\n", 
	  nRead, readPtr->mark, readPtr->fill);
#endif
    return (nRead == bytesLeft) ? BUFFER_OK : BUFFER_CONTINUE;
}

/* 
 * NextLine --
 *
 *	Returns the next available line (terminated by a newline).
 *	If insufficient data is in the buffer, then the channel is
 *	read for more data.  If reading the channel results in a
 *	short read, NULL is returned and *nBytesPtr is set to
 *	BUFFER_CONTINUE.
 */
static char *
NextLine(ReadBuffer *readPtr, int *nBytesPtr)
{
    int i;
    int status;

#if READTRACE
    Debug("Entering NextLine (mark=%d, fill=%d)\n",readPtr->mark, readPtr->fill);
#endif
    status = BUFFER_OK;
    for (;;) {
#if READTRACE
	Debug("in NextLine: mark=%d fill=%d\n", readPtr->mark, readPtr->fill);
#endif
	/* Check for a newline in the current buffer (it's the next 
	 * full line). */
	for (i = readPtr->mark; i < readPtr->fill; i++) {
	    if (readPtr->bytes[i] == '\n') {
		char *p;
		
		/* Save the start of the line. */
		p = readPtr->bytes + readPtr->mark;
		i++;			/* Include the newline. */
		*nBytesPtr = i - readPtr->mark;
		readPtr->mark = i;
#if READTRACE
		Debug("Leaving NextLine(%.*s)\n", *nBytesPtr, p);
#endif
		return p;
	    }
	}
	/* Couldn't find a newline, so it may be that we need to read some
	 * more. Check first that last read wasn't a short read. */
	if (status == BUFFER_CONTINUE) {
	    *nBytesPtr = BUFFER_CONTINUE;  /* No complete line just yet. */
	    return NULL;
	}
	/* Try to add more data to the buffer. */
	status = FillBuffer(readPtr);
	if (status == BUFFER_ERROR) {
	    *nBytesPtr = BUFFER_ERROR;
	    return NULL;	/* EOF or error on read. */
	}
	/* BUFFER_OK or BUFFER_CONTINUE */
    }
#if READTRACE
    Debug("Leaving NextLine failed to read line\n");
#endif
    *nBytesPtr = BUFFER_CONTINUE;
    return NULL;
}

/* 
 * ReadFollowingData --
 *
 *	Read the requested number of bytes from the buffer.  Fails if
 *	the requested number of bytes are not immediately available.
 *	Never should be short. 
 */
static int 
ReadFollowingData(ReadBuffer *readPtr, char *out, int nBytes)
{
#if READTRACE
    Debug("Entering ReadFollowingData(%d)\n", nBytes);
#endif
    while (nBytes > 0) {
	int bytesLeft;
	int status;

	bytesLeft = readPtr->fill - readPtr->mark;
	if (bytesLeft > 0) {
	    int size;

	    /* Pull bytes out of the buffer, updating the mark. */
	    size = (bytesLeft >  nBytes) ? nBytes : bytesLeft;
	    memcpy(out, readPtr->bytes + readPtr->mark, size);
	    readPtr->mark += size;
	    nBytes -= size;
	    out += size;
	}
	if (nBytes == 0) {
	    /* Received requested # bytes. */
#if READTRACE
	    Debug("Leaving ReadFollowingData(%d)\n", nBytes);
#endif
	    return BUFFER_OK;
	}
	/* Didn't get enough bytes, need to read some more. */
	status = FillBuffer(readPtr);
	if (status == BUFFER_ERROR) {
	    return BUFFER_ERROR;
	}
#if READTRACE
	Debug("in ReadFollowingData: after fill: mark=%d fill=%d status=%d\n", 
	      readPtr->mark, readPtr->fill, status);
#endif
    }
#if READTRACE
    Debug("Leaving ReadFollowingData(%d)\n", nBytes);
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
        return TCL_ERROR;
    }
#if EXPECTTRACE
    Debug("Entering Expect(want=\"%s\", maxSize=%d)\n", match, maxSize);
#endif
    c = match[0];
    length = strlen(match);
    for (;;) {
	int nBytes;
	char *line;

	line = NextLine(&proxyPtr->server, &nBytes);
	if (line != NULL) {
#if EXPECTTRACE
	    Debug("pymol says (read %d bytes):%.*s", nBytes, nBytes, line);
#endif
	    if ((c == line[0]) && (strncmp(line, match, length) == 0)) {
		if (maxSize < nBytes) {
		    nBytes = maxSize;
		}
		memcpy(out, line, nBytes);
		clear_error(proxyPtr);
#if EXPECTTRACE
		Debug("Leaving Expect: got (%.*s)\n", nBytes, out);
#endif
		return TCL_OK;
	    }
	    continue;
	}
	if (nBytes == BUFFER_ERROR) {
	    Tcl_AppendResult(proxyPtr->interp, 
		"error reading server to find match for \"", match, "\": ", 
		Tcl_PosixError(proxyPtr->interp), (char *)NULL);
	    return TCL_ERROR;
	}
    }
    Tcl_AppendResult(proxyPtr->interp, "failed to find match for \"", match, 
	"\"", (char *)NULL);
    proxyPtr->error = 2;
    proxyPtr->status = TCL_ERROR;
    return TCL_ERROR;
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
#if KEEPSTATS
    WriteStats("pymolproxy", code);
#endif
    
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

    Debug("command from client is (%s)", cmd);
    result = Tcl_Eval(interp, cmd);

    gettimeofday(&tv, NULL);
    finish = CVT2SECS(tv);

    stats.cmdTime += finish - start;
    stats.nCommands++;
    return result;
}

static Image *
NewImage(PymolProxy *proxyPtr, size_t dataLength)
{
    Image *imgPtr;

    imgPtr = malloc(sizeof(Image) + dataLength);
    if (imgPtr == NULL) {
	ERROR("can't allocate image of %lu bytes", 
	      (unsigned long)(sizeof(Image) + dataLength));
	abort();
    }
    imgPtr->prevPtr = imgPtr->nextPtr = NULL;
    imgPtr->bytesLeft = dataLength;
    if (proxyPtr->headPtr != NULL) {
	proxyPtr->headPtr->prevPtr = imgPtr;
    }
    imgPtr->nextPtr = proxyPtr->headPtr;
    if (proxyPtr->tailPtr == NULL) {
	proxyPtr->tailPtr = imgPtr;
    }
    proxyPtr->headPtr = imgPtr;
    imgPtr->nWritten = 0;
    return imgPtr;
}

INLINE static void
FreeImage(Image *imgPtr)
{
    assert(imgPtr != NULL);
    free(imgPtr);
}

static void
WriteImage(PymolProxy *proxyPtr, int fd)
{
    Image *imgPtr, *prevPtr; 

    if (proxyPtr->tailPtr == NULL) {
	ERROR("Should not be here: no image available to write");
	return;
    }
    for (imgPtr = proxyPtr->tailPtr; imgPtr != NULL; imgPtr = prevPtr) {
	ssize_t bytesLeft;

	assert(imgPtr->nextPtr == NULL);
	prevPtr = imgPtr->prevPtr;
#if WRITETRACE
	Debug("WriteImage: want to write %d bytes.", imgPtr->bytesLeft);
#endif
	for (bytesLeft = imgPtr->bytesLeft; bytesLeft > 0; /*empty*/) {
	    ssize_t nWritten;
#if WRITETRACE
	    Debug("WriteImage: try to write %d bytes.", bytesLeft);
#endif
	    nWritten = write(fd, imgPtr->data + imgPtr->nWritten, bytesLeft);
#if WRITETRACE
	    Debug("WriteImage: wrote %d bytes.", nWritten);
#endif
	    if (nWritten < 0) {
	        ERROR("Error writing fd=%d, %s", fd, strerror(errno));
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
	/* Check if image is on the head.  */
	proxyPtr->tailPtr = prevPtr;
	if (prevPtr != NULL) {
	    prevPtr->nextPtr = NULL;
	}
	FreeImage(imgPtr);
    }
    proxyPtr->headPtr = NULL;
}

static int
Pymol(PymolProxy *proxyPtr, const char *format, ...)
{
    va_list ap;
    char buffer[BUFSIZ];
    char expect[BUFSIZ];
    int result;
    ssize_t nWritten;
    size_t length;
    char *p;

    if (proxyPtr->error) {
        return TCL_ERROR;
    }
    
    va_start(ap, format);
    result = vsnprintf(buffer, BUFSIZ-1, format, ap);
    va_end(ap);
    
    Debug("to-pymol>(%s) code=%d", buffer, result);
    script("%s\n", buffer);
    
    
    /* Write the command out to the server. */
    length = strlen(buffer);
    nWritten = write(proxyPtr->sin, buffer, length);
    if (nWritten != length) {
	ERROR("short write to pymol (wrote=%d, should have been %d): %s",
	      nWritten, length, strerror(errno));
    }
    for (p = buffer; *p != '\0'; p++) {
	if (*p == '\n') {
	    *p = '\0';
	    break;
	}
    }
    sprintf(expect, "PyMOL>%s", buffer);
    /* Now wait for the "PyMOL>" prompt. */
    if (Expect(proxyPtr, expect, buffer, BUFSIZ) != TCL_OK) {
	proxyPtr->error = 1;
	proxyPtr->status = TCL_ERROR;
	return TCL_ERROR;
    }
    return  proxyPtr->status;
}

static void
SetViewport(PymolProxy *proxyPtr)
{
    if (proxyPtr->flags & VIEWPORT_PENDING) {
	Pymol(proxyPtr, "viewport %d,%d\n", proxyPtr->width, proxyPtr->height);
	proxyPtr->flags &= ~VIEWPORT_PENDING;
    }
}

static void
SetZoom(PymolProxy *proxyPtr)
{
    if (proxyPtr->flags & ZOOM_PENDING) {
        Pymol(proxyPtr,"move z,%f\n", proxyPtr->zoom);
	proxyPtr->flags &= ~ZOOM_PENDING;
    }
}

static void
SetPan(PymolProxy *proxyPtr)
{
    if (proxyPtr->flags & PAN_PENDING) {
	Pymol(proxyPtr,"move x,%f\nmove y,%f\n", proxyPtr->xPan,proxyPtr->yPan);
	proxyPtr->flags &= ~PAN_PENDING;
    }
}

static void
SetRotation(PymolProxy *proxyPtr)
{
    if (proxyPtr->flags & ROTATE_PENDING) {
	/* Every pymol command line generates a new rendering. Execute all
	 * three turns as a single command line. */
	Pymol(proxyPtr,"turn x,%f\nturn y,%f\nturn z,%f\n", 
	      proxyPtr->xAngle, proxyPtr->yAngle, proxyPtr->zAngle);
	proxyPtr->xAngle = proxyPtr->yAngle = proxyPtr->zAngle = 0.0f;
	proxyPtr->flags &= ~ROTATE_PENDING;
    }
}

static void
SetSphereScale(PymolProxy *proxyPtr)
{
    if (proxyPtr->flags & ATOM_SCALE_PENDING) {
	Pymol(proxyPtr, "set sphere_scale,%f,all\n", proxyPtr->sphereScale);
	proxyPtr->flags &= ~ATOM_SCALE_PENDING;
    }
}

static void
SetStickRadius(PymolProxy *proxyPtr)
{
    if (proxyPtr->flags & STICK_RADIUS_PENDING) {
	Pymol(proxyPtr, "set stick_radius,%f,all\n", proxyPtr->stickRadius);
	proxyPtr->flags &= ~STICK_RADIUS_PENDING;
    }
}

static void
UpdateSettings(PymolProxy *proxyPtr)
{
    /* Handle all the pending setting changes now. */
    if (proxyPtr->flags & VIEWPORT_PENDING) {
	SetViewport(proxyPtr);
    }
    if (proxyPtr->flags & ROTATE_PENDING) {
	SetRotation(proxyPtr);
    }
    if (proxyPtr->flags & PAN_PENDING) {
	SetPan(proxyPtr);
    }
    if (proxyPtr->flags & ZOOM_PENDING) {
	SetZoom(proxyPtr);
    }
    if (proxyPtr->flags & ATOM_SCALE_PENDING) {
	SetSphereScale(proxyPtr);
    }
    if (proxyPtr->flags & STICK_RADIUS_PENDING) {
	SetStickRadius(proxyPtr);
    }
}

static int
CartoonCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    int bool, defer, push, i;
    PymolProxy *proxyPtr = clientData;
    const char *model;

    clear_error(proxyPtr);
    defer = push = FALSE;
    model = "all";
    bool = FALSE;
    for(i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-defer") == 0) {
            defer = TRUE;
	} else if (strcmp(argv[i],"-push") == 0) {
            push = TRUE;
	} else if (strcmp(argv[i],"-model") == 0) {
            if (++i < argc) {
                model = argv[i];
	    }
	} else { 
	    if (Tcl_GetBoolean(interp, argv[i], &bool) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    proxyPtr->flags |= INVALIDATE_CACHE; /* cartoon */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    if (bool) {
	Pymol(proxyPtr, "show cartoon,%s\n", model);
    } else {
	Pymol(proxyPtr, "hide cartoon,%s\n", model);
    }
    return proxyPtr->status;
}

static int
CartoonTraceCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
		const char *argv[])
{
    int bool, defer, push, i;
    PymolProxy *proxyPtr = clientData;
    const char *model;

    clear_error(proxyPtr);
    defer = push = FALSE;
    bool = FALSE;
    model = "all";
    for(i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-defer") == 0) {
            defer = TRUE;
	} else if (strcmp(argv[i],"-push") == 0) {
            push = TRUE;
	} else if (strcmp(argv[i],"-model") == 0) {
            if (++i < argc) {
                model = argv[i];
	    }
	} else { 
	    if (Tcl_GetBoolean(interp, argv[i], &bool) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    proxyPtr->flags |= INVALIDATE_CACHE; /* cartoon_trace  */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    Pymol(proxyPtr, "set cartoon_trace,%d,%s\n", bool, model);
    return proxyPtr->status;
}

static int
DisableCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    const char *model = "all";
    int i, defer = 0, push = 0;

    clear_error(proxyPtr);

    for(i = 1; i < argc; i++) {

        if (strcmp(argv[i], "-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[i], "-push") == 0 )
            push = 1;
        else
            model = argv[i];
        
    }

    proxyPtr->flags |= INVALIDATE_CACHE;  /* disable */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }

    Pymol( proxyPtr, "disable %s\n", model);

    return proxyPtr->status;
}


static int
EnableCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	  const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    const char *model;
    int i, defer, push;

    clear_error(proxyPtr);
    push = defer = FALSE;
    model = "all";
    for(i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-defer") == 0) {
            defer = TRUE;
	} else if (strcmp(argv[i], "-push") == 0) {
            push = TRUE;
	} else {
            model = argv[i];
	}
    }
    proxyPtr->flags |= INVALIDATE_CACHE; /* enable */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    Pymol( proxyPtr, "enable %s\n", model);
    return proxyPtr->status;
}

static int
FrameCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	 const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    int i, push, defer, frame;

    clear_error(proxyPtr);
    frame = 0;
    push = defer = TRUE;
    for(i = 1; i < argc; i++) { 
        if (strcmp(argv[i],"-defer") == 0) {
            defer = TRUE;
	} else if (strcmp(argv[i],"-push") == 0) {
            push = TRUE;
	} else {
            frame = atoi(argv[i]);
	}
    }
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
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
    int i, push, defer, state, size;
    const char *model;

    clear_error(proxyPtr);
    model = "all";
    size = 14;
    state = TRUE;
    push = defer = FALSE;
    for(i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-defer") == 0) {
            defer = TRUE;
	} else if (strcmp(argv[i],"-push") == 0) {
            push = TRUE;
	} else if (strcmp(argv[i],"-model") == 0) {
            if (++i < argc) {
                model = argv[i];
	    }
	} else if (strcmp(argv[i],"-size") == 0) {
            if (++i < argc) {
                size = atoi(argv[i]);
	    }
	} else if (Tcl_GetBoolean(interp, argv[i], &state) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    proxyPtr->flags |= INVALIDATE_CACHE;  /* label */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    Pymol(proxyPtr, 
	  "set label_color,white,%s\n"
	  "set label_size,%d,%s\n", 
	  model, size, model);
    if (state) {
        Pymol(proxyPtr, "label %s,\"%%s%%s\" %% (ID,name)\n", model);
    } else {
        Pymol(proxyPtr, "label %s\n", model);
    }
    return proxyPtr->status;
}

/* 
 * LoadPDBCmd --
 *
 *	Load a PDB file into pymol.  There is no good way to load PDB data
 *	into pymol without using a file.  This causes problems in that we
 *	don't know when pymol is done with the file.  So there's always
 *	a bit of mess left around. We use the same file for every file
 *	so there's a chance that pymol is still using a file while we're
 *	overwriting it.  
 *
 *	The command expects the number of bytes in the pdb data, the 
 *	name, and the state.
 */
static int
LoadPDBCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    const char *data, *name;
    char *allocated;
    PymolProxy *proxyPtr = clientData;
    int state, defer, push;
    int nBytes;
    int i, j;

    if (proxyPtr == NULL){
	return TCL_ERROR;
    }
    clear_error(proxyPtr);
    defer = push = FALSE;
    for(i = j = 1; i < argc; i++) {
	if (strcmp(argv[i],"-defer") == 0) {
	    defer = TRUE;
	} else if (strcmp(argv[i],"-push") == 0) {
	    push = TRUE;
	} else {
	    if (j < i) {
		argv[j] = argv[i];
	    }
	    j++;
	}
    }
    argc = j;
    if (argc < 4) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
			 " <data>|follows <model> <state> ?<nBytes>?\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    data = argv[1];
    name = argv[2];
    if (Tcl_GetInt(interp, argv[3], &state) != TCL_OK) {
	return TCL_ERROR;
    }
    nBytes = -1;
    if (strcmp(data, "follows") == 0) {
	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
			 " follows <model> <state> <nBytes>\"", (char *)NULL);
	    return TCL_ERROR;
	}
	if (Tcl_GetInt(interp, argv[4], &nBytes) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (nBytes < 0) {
	    Tcl_AppendResult(interp, "bad value for # bytes \"", argv[4],
			 "\"", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }

    /* Does not invalidate cache? */

    allocated = NULL;
    if (nBytes >= 0) {
	allocated = malloc(sizeof(char) * nBytes);
	if (allocated == NULL) {
	    Tcl_AppendResult(interp, "can't allocate buffer for pdbdata.",
			     (char *)NULL);
	    return TCL_ERROR;
	}
	if (ReadFollowingData(&proxyPtr->client, allocated, nBytes) 
	    != BUFFER_OK) {
	    Tcl_AppendResult(interp, "can't read pdbdata from client.",
			     (char *)NULL);
	    free(allocated);
	    return TCL_ERROR;
	}
	data = allocated;
    } else {
	nBytes = strlen(data);
    }
    {
	int f;
	ssize_t nWritten;
	char fileName[200];

	strcpy(fileName, "/tmp/pdb.XXXXXX");
	proxyPtr->status = TCL_ERROR;
	f = mkstemp(fileName);
	if (f < 0) {
	    Tcl_AppendResult(interp, "can't create temporary file \"", 
		fileName, "\":", Tcl_PosixError(interp), (char *)NULL);
	    goto error;
	} 
	nWritten = write(f, data, nBytes);
	if (nBytes != nWritten) {
	    Tcl_AppendResult(interp, "can't write PDB data to \"",
			     fileName, "\": ", Tcl_PosixError(interp),
			     (char *)NULL);
	    close(f);
	    goto error;
	}
	close(f);
	Pymol(proxyPtr, "loadandremovepdbfile %s,%s,%d\n", fileName, name, state);
	proxyPtr->status = TCL_OK;
    }
 error:
    if (allocated != NULL) {
	free(allocated);
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
    proxyPtr->flags |= INVALIDATE_CACHE; /* orthoscopic */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
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
    proxyPtr->flags |= INVALIDATE_CACHE; /* pan */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    if ((x != 0.0f) || (y != 0.0f)) {
	proxyPtr->xPan = x * 0.05;
	proxyPtr->yPan = -y * 0.05;
	proxyPtr->flags |= PAN_PENDING;
    }
    return proxyPtr->status;
}

static int
PngCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    char buffer[BUFSIZ];
    int nBytes=0;
    PymolProxy *proxyPtr = clientData;
    size_t length;
    Image *imgPtr;

    clear_error(proxyPtr);

    /* Force pymol to update the current scene. */
    Pymol(proxyPtr, "refresh\n");
    Pymol(proxyPtr, "png -\n");

    if (Expect(proxyPtr, "png image follows: ", buffer, BUFSIZ-1) != TCL_OK) {
	return TCL_ERROR;
    }
    if (sscanf(buffer, "png image follows: %d\n", &nBytes) != 1) {
	Tcl_AppendResult(interp, "can't get # bytes from \"", buffer, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    } 
    sprintf(buffer, "nv>image %d %d %d %d\n", nBytes, proxyPtr->cacheId, 
	    proxyPtr->frame, proxyPtr->rockOffset);
    length = strlen(buffer);
    imgPtr = NewImage(proxyPtr, nBytes + length);
    strcpy(imgPtr->data, buffer);
    if (ReadFollowingData(&proxyPtr->server, imgPtr->data + length, nBytes)
	!= BUFFER_OK) {
        ERROR("can't read %d bytes for \"image follows\" buffer: %s", nBytes, 
	      strerror(errno));
	return  TCL_ERROR;
    }
    if (Expect(proxyPtr, " ScenePNG", buffer, BUFSIZ-1) != TCL_OK) {
	return TCL_ERROR;
    }
    stats.nFrames++;
    stats.nBytes += nBytes;
    return proxyPtr->status;
}


static int
PpmCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    char buffer[BUFSIZ];
    int nBytes=0;
    PymolProxy *proxyPtr = clientData;
    size_t length;
    Image *imgPtr;

    clear_error(proxyPtr);

    /* Force pymol to update the current scene. */
    Pymol(proxyPtr, "refresh\n");
    Pymol(proxyPtr, "png -,format=1\n");

    if (Expect(proxyPtr, "ppm image follows: ", buffer, BUFSIZ-1) != TCL_OK) {
	return TCL_ERROR;
    }
    if (sscanf(buffer, "ppm image follows: %d\n", &nBytes) != 1) {
	Tcl_AppendResult(interp, "can't get # bytes from \"", buffer, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    } 
    sprintf(buffer, "nv>image %d %d %d %d\n", nBytes, proxyPtr->cacheId, 
	    proxyPtr->frame, proxyPtr->rockOffset);
    length = strlen(buffer);
    imgPtr = NewImage(proxyPtr, nBytes + length);
    strcpy(imgPtr->data, buffer);
    if (ReadFollowingData(&proxyPtr->server, imgPtr->data + length, nBytes)
	!= BUFFER_OK) {
        ERROR("can't read %d bytes for \"image follows\" buffer: %s", nBytes, 
	      strerror(errno));
	return  TCL_ERROR;
    }
#ifdef notdef
    if (Expect(proxyPtr, " ScenePNG", buffer, BUFSIZ-1) != TCL_OK) {
	return TCL_ERROR;
    }
#endif
    stats.nFrames++;
    stats.nBytes += nBytes;
    return proxyPtr->status;
}


static int
PrintCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	 const char *argv[])
{
    char buffer[800];
    int nBytes=0;
    PymolProxy *proxyPtr = clientData;
    size_t length;
    Image *imgPtr;
    int width, height;
    const char *token, *bgcolor;

    clear_error(proxyPtr);

    if (argc != 5) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"", 
			 argv[0], " token width height color\"", (char *)NULL);
	return TCL_ERROR;
    }
    token = argv[1];
    if (Tcl_GetInt(interp, argv[2], &width) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &height) != TCL_OK) {
	return TCL_ERROR;
    }
    bgcolor = argv[4];
    /* Force pymol to update the current scene. */
    if (strcmp(bgcolor, "none") == 0) {
	Pymol(proxyPtr, "set ray_opaque_background,off\n");
	Pymol(proxyPtr, "refresh\n", bgcolor);
    } else {
	Pymol(proxyPtr, "set ray_opaque_background,on\n");
	Pymol(proxyPtr, "bg_color %s\nrefresh\n", bgcolor);
    }
    Pymol(proxyPtr, "ray %d,%d\n", width, height);
    if (Expect(proxyPtr, " Ray:", buffer, BUFSIZ-1) != TCL_OK) {
	return TCL_ERROR;
    }

    Pymol(proxyPtr, "png -,dpi=300\nbg_color black\n");
    if (Expect(proxyPtr, "png image follows: ", buffer, BUFSIZ-1) != TCL_OK) {
	return TCL_ERROR;
    }

    if (sscanf(buffer, "png image follows: %d\n", &nBytes) != 1) {
	Tcl_AppendResult(interp, "can't get # bytes from \"", buffer, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    sprintf(buffer, "nv>image %d print \"%s\" %d\n", nBytes, token, 
	    proxyPtr->rockOffset);
    Debug("header is png is (%s)\n", buffer);
    length = strlen(buffer);
    imgPtr = NewImage(proxyPtr, nBytes + length);
    strcpy(imgPtr->data, buffer);
    if (ReadFollowingData(&proxyPtr->server, imgPtr->data + length, nBytes)
	!= BUFFER_OK) {
        ERROR("can't read %d bytes for \"image follows\" buffer: %s", nBytes,
	      strerror(errno));
	return  TCL_ERROR;
    }
    if (Expect(proxyPtr, " ScenePNG", buffer, BUFSIZ-1) != TCL_OK) {
	return TCL_ERROR;
    }

    stats.nFrames++;
    stats.nBytes += nBytes;
    return proxyPtr->status;
}

static int
RawCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    int arg, defer = 0, push = 0;
    const char *cmd;
    clear_error(proxyPtr);

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

    proxyPtr->flags |= INVALIDATE_CACHE; /* raw */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    Pymol(proxyPtr,"%s\n", cmd);
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
                
    proxyPtr->flags |= INVALIDATE_CACHE; /* reset */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    Pymol(proxyPtr, 
	  "reset\n"
	  "zoom complete=1\n");
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

    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    Pymol(proxyPtr,"turn y, %f\n", y - proxyPtr->rockOffset);
    proxyPtr->rockOffset = y;
    return proxyPtr->status;
}

static int
RepresentationCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
		  const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    const char *model;
    const char *rep;
    int defer, push, i;

    clear_error(proxyPtr);
    defer = push = FALSE;
    model = "all";
    rep = NULL;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-defer") == 0 ) {
            defer = TRUE;
	} else if (strcmp(argv[i],"-push") == 0) {
            push = TRUE;
	} else if (strcmp(argv[i],"-model") == 0) {
            if (++i < argc) {
                model = argv[i];
	    }
        } else {
            rep = argv[i];
	}
    }
    if (rep == NULL) {
	Tcl_AppendResult(interp, "missing representation argument",
			 (char *)NULL);
	return TCL_ERROR;
    }

    proxyPtr->flags |= INVALIDATE_CACHE; /* representation */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    if (strcmp(rep, "ballnstick") == 0) { /* Ball 'n Stick */
	Pymol(proxyPtr, 
	      "set stick_color,white,%s\n"
	      "show sticks,%s\n"
	      "show spheres,%s\n"
	      "hide lines,%s\n"
	      "hide cartoon,%s\n",
	      model, model, model, model, model);
    } else if (strcmp(rep, "spheres") == 0) { /* spheres */    
	Pymol(proxyPtr, 
	      "hide sticks,%s\n"
	      "show spheres,%s\n"
	      "hide lines,%s\n"
	      "hide cartoon,%s\n"
	      "set sphere_quality,2,%s\n"
	      "set ambient,.2,%s\n", 
	      model, model, model, model, model, model);
    } else if (strcmp(rep, "none") == 0) { /* nothing */    
	Pymol(proxyPtr, 
	      "hide sticks,%s\n", 
	      "hide spheres,%s\n"
	      "hide lines,%s\n"
	      "hide cartoon,%s\n",
	      model, model, model, model);
    } else if (strcmp(rep, "sticks") == 0) { /* sticks */    
	Pymol(proxyPtr, 
	      "set stick_color,white,%s\n"
	      "show sticks,%s\n"
	      "hide spheres,%s\n"
	      "hide lines,%s\n"
	      "hide cartoon,%s\n",
	      model, model, model, model, model);
    } else if (strcmp(rep, "lines") == 0) { /* lines */    
	Pymol(proxyPtr, 
	      "hide sticks,%s\n"
	      "hide spheres,%s\n"
	      "show lines,%s\n"
	      "hide cartoon,%s\n",
	      model, model, model, model);
    } else if (strcmp(rep, "cartoon") == 0) { /* cartoon */    
	Pymol(proxyPtr, 
	      "hide sticks,%s\n"
	      "hide spheres,%s\n"
	      "hide lines,%s\n"
	      "show cartoon,%s\n", 
	      model, model, model, model);
    } 
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
    proxyPtr->flags |= INVALIDATE_CACHE; /* rotate */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    if ((xAngle != 0.0f) || (yAngle != 0.0f) || (zAngle != 0.0f)) {
	proxyPtr->xAngle += xAngle;
	proxyPtr->yAngle += yAngle;
	proxyPtr->zAngle += zAngle;
	proxyPtr->flags |= ROTATE_PENDING;
    }
    return proxyPtr->status;
}

static int
ScreenCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	  const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    int width = -1, height = -1;
    int defer, push, i, varg;

    clear_error(proxyPtr);
    defer = push = FALSE;
    varg = 1;
    for(i = 1; i < argc; i++) {
        if ( strcmp(argv[i],"-defer") == 0 ) 
            defer = 1;
        else if ( strcmp(argv[i], "-push") == 0 )
            push = 1;
        else if (varg == 1) {
            width = atoi(argv[i]);
            height = width;
            varg++;
        }
        else if (varg == 2) {
            height = atoi(argv[i]);
            varg++;
        }
    }
    if ((width < 0) || (height < 0)) {
	return TCL_ERROR;
    }
    proxyPtr->flags |= INVALIDATE_CACHE; /* viewport */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    proxyPtr->width = width;
    proxyPtr->height = height;
    proxyPtr->flags |= VIEWPORT_PENDING;

    //usleep(205000); // .2s delay for pymol to update its geometry *HACK ALERT*
        
    return proxyPtr->status;
}

static int
SphereScaleCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    int defer = 0, push = 0, i;
    double scale;
    const char *model = "all";
    PymolProxy *proxyPtr = clientData;

    clear_error(proxyPtr);
    scale = 0.25f;
    for(i = 1; i < argc; i++) {
        if ( strcmp(argv[i],"-defer") == 0 ) {
            defer = 1;
	} else if (strcmp(argv[i],"-push") == 0) {
            push = 1;
	} else if (strcmp(argv[i],"-model") == 0) {
            if (++i < argc) {
                model = argv[i];
	    }
        } else {
	    if (Tcl_GetDouble(interp, argv[i], &scale) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    proxyPtr->flags |= INVALIDATE_CACHE;  /* sphere_scale */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }

    if (strcmp(model, "all") == 0) {
	proxyPtr->flags |= ATOM_SCALE_PENDING;
	proxyPtr->sphereScale = scale;
    } else {
	Pymol(proxyPtr, "set sphere_scale,%f,%s\n", scale, model);
    }
    return proxyPtr->status;
}

static int
StickRadiusCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
		 const char *argv[])
{
    int defer = 0, push = 0, i;
    double scale;
    const char *model = "all";
    PymolProxy *proxyPtr = clientData;

    clear_error(proxyPtr);
    scale = 0.25f;
    for(i = 1; i < argc; i++) {
        if ( strcmp(argv[i],"-defer") == 0 ) {
            defer = 1;
	} else if (strcmp(argv[i],"-push") == 0) {
            push = 1;
	} else if (strcmp(argv[i],"-model") == 0) {
            if (++i < argc)
                model = argv[i];
        } else {
	    if (Tcl_GetDouble(interp, argv[i], &scale) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    proxyPtr->flags |= INVALIDATE_CACHE;  /* stick_radius */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }

    if (strcmp(model, "all") == 0) {
	proxyPtr->flags |= STICK_RADIUS_PENDING;
	proxyPtr->stickRadius = scale;
    } else {
	Pymol(proxyPtr, "set stick_radius,%f,%s\n", scale, model);
    }
    return proxyPtr->status;
}

static int
TransparencyCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
		const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    const char *model;
    float transparency;
    int defer, push;
    int i;

    clear_error(proxyPtr);
    model = "all";
    defer = push = FALSE;
    transparency = 0.0f;
    for(i = 1; i < argc; i++) {
        if ( strcmp(argv[i],"-defer") == 0 ) {
            defer = 1;
	} else if (strcmp(argv[i],"-push") == 0) {
            push = 1;
	} else if (strcmp(argv[i],"-model") == 0) {
            if (++i < argc) {
                model = argv[i];
	    }
	} else {
	    transparency = atof(argv[i]);
	}
    }
    proxyPtr->flags |= INVALIDATE_CACHE; /* transparency */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    } 
    Pymol(proxyPtr, 
	  "set sphere_transparency,%g,%s\n"
	  "set stick_transparency,%g,%s\n"
	  "set cartoon_transparency,%g,%s\n",
	  transparency, model, transparency, model, 
	  transparency, model);
    return proxyPtr->status;
}

static int
VMouseCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	  const char *argv[])
{
    PymolProxy *proxyPtr = clientData;
    int i, defer = 0, push = 0, varg = 1;
    int arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0, arg5 = 0;

    clear_error(proxyPtr);

    for(i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-defer") == 0)
            defer = 1;
        else if (strcmp(argv[i], "-push") == 0)
            push = 1;
        else if (varg == 1) {
            arg1 = atoi(argv[i]);
            varg++;
        }
        else if (varg == 2) {
            arg2 = atoi(argv[i]);
            varg++;
        }
        else if (varg == 3) {
            arg3 = atoi(argv[i]);
            varg++;
        }
        else if (varg == 4) {
            arg4 = atoi(argv[i]);
            varg++;
        }
        else if (varg == 5) {
            arg5 = atoi(argv[i]);
            varg++;
        }
    }

    proxyPtr->flags |= INVALIDATE_CACHE; /* vmouse */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    Pymol(proxyPtr, "vmouse %d,%d,%d,%d,%d\n", arg1, arg2, arg3, arg4, arg5);
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
    int defer = 0, push = 0, i, varg = 1;

    clear_error(proxyPtr);

    for(i = 1; i < argc; i++) {
	if (strcmp(argv[i],"-defer") == 0)
	    defer = 1;
	else if (strcmp(argv[i],"-push") == 0)
	    push = 1;
	else if (varg == 1) {
	    double value;
	    if (Tcl_GetDouble(interp, argv[i], &value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    factor = (float)value;
	    varg++;
	}
    }
    proxyPtr->flags |= INVALIDATE_CACHE; /* Zoom */
    if (!defer || push) {
	proxyPtr->flags |= UPDATE_PENDING;
    }
    if (push) {
	proxyPtr->flags |= FORCE_UPDATE;
    }
    if (factor != 0.0) {
	proxyPtr->zoom = factor;
	proxyPtr->flags |= ZOOM_PENDING;
    }
    return proxyPtr->status;
}

        
static void
ChildHandler(int sigNum) 
{
    ERROR("pymol (%d) died unexpectedly", stats.child);
    exit(1);
}

static int 
ProxyInit(int cin, int cout, char *const *argv)
{
    char stderrFile[200];
    int status, result = 0;
    int sin[2];
    int sout[2];
    Tcl_Interp *interp;
    pid_t child, parent;
    PymolProxy proxy;
    struct timeval end;

    /* Create three pipes for communication with the external application. One
     * each for the applications's: stdin, stdout, and stderr  */

    if (pipe(sin) == -1)
        return -1;

    if (pipe(sout) == -1) {
        close(sin[0]);
        close(sin[1]);
        return -1;
    }

    parent = getpid();
    sprintf(stderrFile, "/tmp/pymol%d.stderr", parent);

    /* Fork the new process.  Connect I/O to the new socket.  */

    child = fork();
    if (child < 0) {
        ERROR("can't fork process: %s\n", strerror(errno));
        return -3;
    }

    interp = Tcl_CreateInterp();
    Tcl_MakeSafe(interp);

    if (child == 0) {
        int f;
	char path[200];

        /* Child process */
        
        /* 
         * Create a new process group, so we can later kill this process and
         * all its children without affecting the process that created this
         * one.
         */
        setpgid(child, 0); 
        
        /* Redirect stdin, stdout, and stderr to pipes before execing */ 

        dup2(sin[0],  0);		// stdin
        dup2(sout[1], 1);		// stdout
	f = open(stderrFile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (f < 0) {
	    ERROR("can't open server error file `%s': %s", path, 
		  strerror(errno));
	    exit(1);
	}
        dup2(f, 2);			/* Redirect stderr to a file */
        
	/* Close all other descriptors  */        
	for (f = 3; f < FD_SETSIZE; f++) {
            close(f);
	}
        INFO("attempting to start \"%s\"", argv[0]);
        
        execvp(argv[0], argv);
        ERROR("can't exec `%s': %s", argv[0], strerror(errno));
	exit(-1);
    } else {
	signal(SIGCHLD, ChildHandler);
    }
    stats.child = child;

    Debug("Started %s DISPLAY=%s\n", argv[0], getenv("DISPLAY"));

    /* close opposite end of pipe, these now belong to the child process  */
    close(sin[0]);
    close(sout[1]);

    memset(&proxy, 0, sizeof(PymolProxy));
    proxy.sin  = sin[1];
    proxy.sout = sout[0];
    proxy.cin  = cin;
    proxy.cout = cout;
    proxy.flags = CAN_UPDATE;
    proxy.frame = 1;
    proxy.interp = interp;

    Tcl_CreateCommand(interp, "cartoon",       CartoonCmd,       &proxy, NULL);
    Tcl_CreateCommand(interp, "cartoontrace",  CartoonTraceCmd,  &proxy, NULL);
    Tcl_CreateCommand(interp, "disable",       DisableCmd,       &proxy, NULL);
    Tcl_CreateCommand(interp, "enable",        EnableCmd,        &proxy, NULL);
    Tcl_CreateCommand(interp, "frame",         FrameCmd,         &proxy, NULL);
    Tcl_CreateCommand(interp, "label",         LabelCmd,         &proxy, NULL);
    Tcl_CreateCommand(interp, "loadpdb",       LoadPDBCmd,       &proxy, NULL);
    Tcl_CreateCommand(interp, "orthoscopic",   OrthoscopicCmd,   &proxy, NULL);
    Tcl_CreateCommand(interp, "pan",           PanCmd,           &proxy, NULL);
    Tcl_CreateCommand(interp, "png",           PngCmd,           &proxy, NULL);
    Tcl_CreateCommand(interp, "ppm",           PpmCmd,           &proxy, NULL);
    Tcl_CreateCommand(interp, "print",         PrintCmd,         &proxy, NULL);
    Tcl_CreateCommand(interp, "raw",           RawCmd,           &proxy, NULL);
    Tcl_CreateCommand(interp, "representation",RepresentationCmd,&proxy, NULL);
    Tcl_CreateCommand(interp, "reset",         ResetCmd,         &proxy, NULL);
    Tcl_CreateCommand(interp, "rock",          RockCmd,          &proxy, NULL);
    Tcl_CreateCommand(interp, "rotate",        RotateCmd,        &proxy, NULL);
    Tcl_CreateCommand(interp, "screen",        ScreenCmd,        &proxy, NULL);
    Tcl_CreateCommand(interp, "spherescale",   SphereScaleCmd,   &proxy, NULL);
    Tcl_CreateCommand(interp, "stickradius",   StickRadiusCmd,   &proxy, NULL);
    Tcl_CreateCommand(interp, "transparency",  TransparencyCmd,  &proxy, NULL);
    Tcl_CreateCommand(interp, "viewport",      ScreenCmd,        &proxy, NULL);
    Tcl_CreateCommand(interp, "vmouse",        VMouseCmd,        &proxy, NULL);
    Tcl_CreateCommand(interp, "zoom",          ZoomCmd,          &proxy, NULL);


    gettimeofday(&end, NULL);
    stats.start = end;

    // Main Proxy Loop
    //  accept tcl commands from socket
    //  translate them into pyMol commands, and issue them to child proccess
    //  send images back

    PollForEvents(&proxy);

    unlink(stderrFile);
    close(proxy.cout);
    close(proxy.sout);
    close(proxy.cin);
    close(proxy.sin);

    if (waitpid(child, &result, 0) < 0) {
        ERROR("error waiting on pymol server to exit: %s", strerror(errno));
    }
    INFO("attempting to signal (SIGTERM) pymol server.");
    kill(-child, SIGTERM);		// Kill process group
    alarm(5);
    
    if (waitpid(child, &result, 0) < 0) {
        ERROR("error waiting on pymol server to exit after SIGTERM: %s", 
	      strerror(errno));
    }
    status = -1;
    while ((status == -1) && (errno == EINTR)) {
	ERROR("Attempting to signal (SIGKILL) pymol server.");
	kill(-child, SIGKILL);	// Kill process group
	alarm(10);
	status = waitpid(child, &result, 0);
	alarm(0); 
    }
    INFO("pymol server process ended (result=%d)", result);

    Tcl_DeleteInterp(interp);
    
    if (WIFEXITED(result)) {
	result = WEXITSTATUS(result);
    }
    DoExit(result);
    return 0;
}


static void
PollForEvents(PymolProxy *proxyPtr)
{
    Tcl_DString clientCmds;
    struct pollfd pollFd[3];
    int flags;

    if (write(proxyPtr->cout, "PyMol 1.0\n", 10) != 10) {
	ERROR("short write of signature");
    }

    flags = fcntl(proxyPtr->cin, F_GETFL);
    fcntl(proxyPtr->cin, F_SETFL, flags|O_NONBLOCK);

#ifdef notdef
    flags = fcntl(proxyPtr->sout, F_GETFL);
    fcntl(proxyPtr->sout, F_SETFL, flags|O_NONBLOCK);

    flags = fcntl(proxyPtr->cout, F_GETFL);
    fcntl(proxyPtr->cout, F_SETFL, flags|O_NONBLOCK);
#endif

    /* Read file descriptors. */
    pollFd[0].fd = proxyPtr->cout;	/* Client standard output  */
    pollFd[1].fd = proxyPtr->sout;	/* Server standard error.  */
    pollFd[0].events = pollFd[1].events = POLLIN;

    /* Write file descriptors. */
    pollFd[2].fd = proxyPtr->cin;	/* Client standard input. */
    pollFd[2].events = POLLOUT;

    InitBuffer(&proxyPtr->client, "client", proxyPtr->cout);
    InitBuffer(&proxyPtr->server, "server", proxyPtr->sout);

    Tcl_DStringInit(&clientCmds);
    for (;;) {
	int timeout, nChannels;

	nChannels =  (proxyPtr->headPtr != NULL) ? 3 : 2;
#define PENDING_TIMEOUT		10  /* milliseconds. */
	timeout = (proxyPtr->flags & UPDATE_PENDING) ? PENDING_TIMEOUT : -1;
	nChannels = poll(pollFd, nChannels, timeout);
	if (nChannels < 0) {
	    ERROR("POLL ERROR: %s", strerror(errno));
	    continue;		/* or exit? */
	}

	/* 
	 * The next two sections are to drain any leftover data in 
	 * the pymol server process' stdout or stderr.  We don't want the
	 * the pymol server to block writing to stderr or stdout.
	 */
	if (pollFd[1].revents & POLLIN) { /* Server stdout */
	    int nBytes;
	    char *line;
	    
	    Debug("Reading pymol stdout\n");
	    /* Don't care what's in the server output buffer. */
	    FlushBuffer(&proxyPtr->server);
	    line = NextLine(&proxyPtr->server, &nBytes);
	    if (line != NULL) {
		Debug("STDOUT>%.*s", nBytes, line);
		INFO("Garbage found from pymol server: %.%s", nBytes, line);
	    } else if (nBytes == BUFFER_CONTINUE) {
		Debug("No data found on pymol stdout\n");
		continue;
	    } else {
		ERROR("can't read pymol stdout (nBytes=%d): %s\n", nBytes,
		      strerror(errno));
		goto error;		/* Get out on EOF or error. */
	    }
	}

	/* We have some descriptors ready. */
	if (pollFd[0].revents & POLLIN) { /* Client stdout */
	    Debug("Reading client stdout\n");
	    for (;;) {
		int nBytes;
		char *line;
		
		line = NextLine(&proxyPtr->client, &nBytes);
		if (line != NULL) {
		    const char *cmd;

		    Tcl_DStringAppend(&clientCmds, line, nBytes);
		    cmd = Tcl_DStringValue(&clientCmds);
		    if (Tcl_CommandComplete(cmd)) {
			int result;

			/* May execute more than one command. */
			result = ExecuteCommand(proxyPtr->interp, cmd);
			if (result != TCL_OK) {
			    Tcl_Obj *objPtr;

			    objPtr = Tcl_GetObjResult(proxyPtr->interp);
			    ERROR(Tcl_GetString(objPtr));
			    goto error;
			}
			Tcl_DStringSetLength(&clientCmds, 0);
		    }
		    continue;
		}
		if (nBytes == BUFFER_CONTINUE) {
		    break;
		}
		ERROR("can't read client stdout (nBytes=%d): %s\n", nBytes,
		      strerror(errno));
		goto error;		/* Get out on EOF or error. */
	    }
	    Debug("done with client stdout\n");
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

	/* Handle all the pending setting changes now. */
	UpdateSettings(proxyPtr);

	/* Write the current image buffer. */
	if (proxyPtr->headPtr == NULL) {
	    /* We might want to refresh the image if we're not currently
	     * transmitting an image back to the client. The image will be
	     * refreshed after the image has been completely transmitted. */
	    if ((nChannels == 0) || (proxyPtr->flags & FORCE_UPDATE)) {
		if (proxyPtr->flags & UPDATE_PENDING) {
		    Tcl_Eval(proxyPtr->interp, "ppm");
		    proxyPtr->flags &= ~UPDATE_PENDING;
		}
		proxyPtr->flags &= ~FORCE_UPDATE;
		continue;
	    }
	}
	if ((proxyPtr->headPtr != NULL) && (pollFd[2].revents & POLLOUT)) { 
	    WriteImage(proxyPtr, pollFd[2].fd);
	}
    }
 error:
    Tcl_DStringFree(&clientCmds);
    return;
}

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
    scriptFile = NULL;
    if (savescript) {
	char fileName[200];
	sprintf(fileName, "/tmp/pymolproxy.py");
        scriptFile = fopen(fileName, "w");
    }    
    ProxyInit(fileno(stdout), fileno(stdin), argv + 1);
    return 0;
}

