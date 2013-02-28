
/*
 * ----------------------------------------------------------------------
 * proxypymol2.c
 *
 *      This module creates the Tcl interface to the pymol server.  It acts as
 *      a go-between establishing communication between a molvisviewer widget
 *      and the pymol server. The communication protocol from the molvisviewer
 *      widget is the Tcl language.  Commands are then relayed to the pymol
 *      server.  Responses from the pymol server are translated into Tcl
 *      commands and send to the molvisviewer widget. For example, resulting
 *      image rendered offscreen is returned as ppm-formatted image data.
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
 *   +--------------+          +------------+          +----------+
 *   |              | (stdin)  |            |  (stdin) |          | 
 *   |              |--------->|            |--------->|          |
 *   | molvisviewer |          | pymolproxy |          | pymol    |
 *   |  (client)    |          |            |          | (server) |(stderr)
 *   |              | (stdout) |            | (stdout) |          |-------> file
 *   |              |<---------|            |<---------|          | 
 *   +--------------+          +------------+          +----------+
 * 
 * We're using a simple 2 thread setup: one for read client requests and
 * relaying them to the pymol server, another reading pymol server output,
 * and sending images by to the client.  The reason is because the client
 * blocks on writes.  While it is sending requests or data, the proxy
 * must be available to accept it.  Likewise, the proxy buffers images in a
 * linked list when the client isn't ready to receive them.
 *
 * Reader thread:
 * The communication between the pymol server and proxy is asynchronous.  
 * The proxy translates commands from the client and sends them to the
 * server without waiting for a response.  It watches the server's
 * stdout in a separate thread snooping for image data.  
 *
 * Writer thread:
 * The communication between the client and the proxy is also asynchronous.  
 * The client commands are read when they become available on the socket.  
 * The proxy writes images to the client when it can, otherwise it stores
 * them in a list in memory.  This should prevent deadlocks from occuring:
 * the client sends a request, while the proxy is writing an image.
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
#include <pthread.h>
#include <md5.h>

#undef INLINE
#ifdef __GNUC__
#  define INLINE __inline__
#else
#  define INLINE
#endif

#define FALSE 0
#define TRUE  1

static int debug = FALSE;
static char stderrFile[200];
static FILE *fdebug;
static FILE *frecord;
static int recording = FALSE;
static int pymolIsAlive = TRUE;
static int statsFile = -1;

#define WANT_DEBUG	0
#define READ_DEBUG	0
#define WRITE_DEBUG	0
#define EXEC_DEBUG	0

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

#define IO_TIMEOUT (30000)
#define STATSDIR	"/var/tmp/visservers"
#define CVT2SECS(x)  ((double)(x).tv_sec) + ((double)(x).tv_usec * 1.0e-6)

typedef struct {
    pid_t pid;				/* Child process. */
    size_t numFrames;			/* # of frames sent to client. */
    size_t numBytes;			/* # of bytes for all frames. */
    size_t numCommands;			/* # of commands executed */
    double cmdTime;			/* Elapsed time spend executing
					 * commands. */
    struct timeval start;		/* Start of elapsed time. */
} Stats;

static Stats stats;

typedef struct Image {
    struct Image *nextPtr;		/* Next image in chain of images. The
					 * list is ordered by the most
					 * recently received image from the
					 * pymol server to the least. */
    struct Image *prevPtr;		/* Previous image in chain of
					 * images. The list is ordered by the
					 * most recently received image from
					 * the pymol server to the least. */
    int id;
    ssize_t numWritten;			/* Number of bytes of image data
					 * already delivered.*/
    size_t bytesLeft;			/* Number of bytes of image data left
					 * to delivered to the client. */
    unsigned char data[1];		/* Start of image data. We allocate
					 * the size of the Image structure
					 * plus the size of the image data. */
} Image;

#define BUFFER_SIZE		4096

typedef struct {
    Image *headPtr, *tailPtr;		/* List of images to be delivered to
					 * the client.  The most recent images
					 * are in the front of the list. */
} ImageList;

typedef struct {
    const char *ident;
    unsigned char *bytes;
    size_t fill;
    size_t mark;
    size_t newline;
    size_t bufferSize;
    int lastStatus;
    int fd;
} ReadBuffer;

#define BUFFER_OK		 0
#define BUFFER_ERROR		-1
#define BUFFER_CONTINUE		-2
#define BUFFER_EOF		-3

typedef struct {
    Tcl_Interp *interp;
    unsigned int flags;			/* Various flags. */
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
    pid_t pid;
} PymolProxy;

#if WANT_DEBUG
#define DEBUG(...)	if (debug) PrintToLog(__VA_ARGS__)
#else 
#define DEBUG(...) 
#endif

#define ERROR(...)      SysLog(LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
#define TRACE(...)      SysLog(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define WARN(...)       SysLog(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)       SysLog(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)

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

#if WANT_DEBUG
static void
PrintToLog TCL_VARARGS_DEF(const char *, arg1)
{
    const char *format;
    va_list args;
    
    format = TCL_VARARGS_START(const char *, arg1, args);
    fprintf(fdebug, "pymolproxy: ");
    vfprintf(fdebug, format, args);
    fprintf(fdebug, "\n");
    fflush(fdebug);
}
#endif

void
SysLog(int priority, const char *path, int lineNum, const char* fmt, ...)
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
    length = snprintf(message, MSG_LEN, "pymolproxy2 (%d) %s: %s:%d ", 
	getpid(), syslogLevels[priority],  s, lineNum);
    length += vsnprintf(message + length, MSG_LEN - length, fmt, lst);
    message[MSG_LEN] = '\0';
    if (debug) {
	DEBUG("%s\n", message);
    } else {
	syslog(priority, message, length);
    }
}

static void
Record TCL_VARARGS_DEF(const char *, arg1)
{
    const char *format;
    va_list args;
    
    format = TCL_VARARGS_START(const char *, arg1, args);
    vfprintf(frecord, format, args);
    fflush(frecord);
}

static int
SendToPymol(PymolProxy *proxyPtr, const char *format, ...)
{
    va_list ap;
    char buffer[BUFSIZ];
    int result;
    ssize_t numWritten;
    size_t length;

    if (proxyPtr->error) {
        return TCL_ERROR;
    }
    
    va_start(ap, format);
    result = vsnprintf(buffer, BUFSIZ-1, format, ap);
    va_end(ap);
    
#ifdef EXEC_DEBUG
    DEBUG("to-pymol>(%s) code=%d", buffer, result);
#endif
    if (recording) {
	Record("%s\n", buffer);
    }
    
    /* Write the command out to the server. */
    length = strlen(buffer);
    numWritten = write(proxyPtr->sin, buffer, length);
    if (numWritten != length) {
	ERROR("short write to pymol (wrote=%d, should have been %d): %s",
	      numWritten, length, strerror(errno));
    }
    proxyPtr->status = result;
    return  proxyPtr->status;
}



/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

static void
FlushReadBuffer(ReadBuffer *bp)
{
    bp->fill = bp->mark = 0;
    bp->newline = 0;
}

/**
 * \param[in] fd File descriptor to read
 * \param[in] bufferSize Block size to use in internal buffer
 */

static void
InitReadBuffer(ReadBuffer *bp, const char *id, int fd, size_t bufferSize)
{
    bp->ident = id;
    bp->bufferSize = bufferSize;
    bp->fd = fd;
    bp->lastStatus = BUFFER_OK;
    bp->bytes = malloc(bufferSize);
    FlushReadBuffer(bp);
}

/**
 * \brief Checks if a new line is currently in the buffer.
 *
 * \return the index of the character past the new line.
 */
static size_t
NextLine(ReadBuffer *bp)
{
    /* Check for a newline in the current buffer. */

    if (bp->newline > 0) {
	return bp->newline;
    } else {
	unsigned char *p;
	size_t newline;

	p = (unsigned char *)memchr(bp->bytes + bp->mark, '\n', 
				    bp->fill - bp->mark);
	if (p == NULL) {
	    newline = 0;
	} else {
	    newline = (p - bp->bytes + 1);
	}
	bp->newline = newline;
	return newline;
    }
}

/**  
 * \brief Fills the buffer with available data.
 * 
 * Any existing data in the buffer is moved to the front of the buffer, 
 * then the channel is read to fill the rest of the buffer.
 *
 * \return If an error occur when reading the channel, then ERROR is
 * returned. ENDFILE is returned on EOF.  If the buffer can't be filled, 
 * then CONTINUE is returned.
 */
static int 
FillReadBuffer(ReadBuffer *bp)
{
    ssize_t numRead;
    size_t bytesLeft;

#if READ_DEBUG
    DEBUG("Enter FillReadBuffer for %s: mark=%lu fill=%lu", 
	  bp->ident, bp->mark, bp->fill);
#endif
    if (bp->mark >= bp->fill) {
        FlushReadBuffer(bp);	/* Fully consumed buffer */
    }
    if (bp->mark > 0) {
        /* Some data has been consumed. Move the unconsumed data to the front
         * of the buffer. */
#if READ_DEBUG
        DEBUG("memmove %lu bytes", bp->fill - bp->mark);
#endif
        memmove(bp->bytes, bp->bytes + bp->mark, 
		bp->fill - bp->mark);
        bp->fill -= bp->mark;
        bp->mark = 0;
    }

    bytesLeft = bp->bufferSize - bp->fill;
#if READ_DEBUG
    DEBUG("going to read %lu bytes", bytesLeft);
#endif
    numRead = read(bp->fd, bp->bytes + bp->fill, bytesLeft);
    if (numRead == 0) {
        /* EOF */
#if READ_DEBUG
        DEBUG("EOF found reading %s buffer (fill=%d): ", 
	      bp->ident, bp->fill);
#endif
        return BUFFER_EOF;
    }
    if (numRead < 0) {
        if (errno != EAGAIN) {
            ERROR("error reading %s buffer: %s", bp->ident, strerror(errno));
            return BUFFER_ERROR;
        }
#if READ_DEBUG
        DEBUG("Short read for buffer");
#endif
        return BUFFER_CONTINUE;
    }
    bp->fill += numRead;
#if READ_DEBUG
    DEBUG("Read %lu bytes", numRead);
#endif
    return ((size_t)numRead == bytesLeft) ? BUFFER_OK : BUFFER_CONTINUE;
}

/**
 * \brief Read the requested number of bytes from the buffer.

 * Fails if the requested number of bytes are not immediately 
 * available. Never should be short. 
 */
static int
ReadFollowingData(ReadBuffer *bp, unsigned char *out, size_t numBytes)
{
#if READ_DEBUG
    DEBUG("Enter ReadFollowingData %s", bp->ident);
#endif
    while (numBytes > 0) {
        size_t bytesLeft;

        bytesLeft = bp->fill - bp->mark;
        if (bytesLeft > 0) {
            int size;

            /* Pull bytes out of the buffer, updating the mark. */
            size = (bytesLeft >  numBytes) ? numBytes : bytesLeft;
            memcpy(out, bp->bytes + bp->mark, size);
            bp->mark += size;
            numBytes -= size;
            out += size;
        }
        if (numBytes == 0) {
            /* Received requested # bytes. */
            return BUFFER_OK;
        }
        /* Didn't get enough bytes, need to read some more. */
        bp->lastStatus = FillReadBuffer(bp);
        if ((bp->lastStatus == BUFFER_ERROR) || 
	    (bp->lastStatus == BUFFER_EOF)) {
            return bp->lastStatus;
        }
    }
    return BUFFER_OK;
}

/** 
 * \brief Returns the next available line (terminated by a newline)
 * 
 * If insufficient data is in the buffer, then the channel is
 * read for more data.  If reading the channel results in a
 * short read, CONTINUE is returned and *numBytesPtr is set to 0.
 */
static int
GetLine(ReadBuffer *bp, size_t *numBytesPtr, const char **bytesPtr)
{
#if READ_DEBUG
    DEBUG("Enter GetLine");
#endif
    *numBytesPtr = 0;
    *bytesPtr = NULL;

    bp->lastStatus = BUFFER_OK;
    for (;;) {
        size_t newline;

        newline = NextLine(bp);
        if (newline > 0) {
            /* Start of the line. */
            *bytesPtr = (const char *)(bp->bytes + bp->mark);
            /* Number of bytes in the line. */
            *numBytesPtr = newline - bp->mark;
            bp->mark = newline;
	    bp->newline = 0;
            return BUFFER_OK;
        }
        /* Couldn't find a newline, so it may be that we need to read some
         * more. Check first that last read wasn't a short read. */
        if (bp->lastStatus == BUFFER_CONTINUE) {
            /* No complete line just yet. */
            return BUFFER_CONTINUE;
        }
        /* Try to add more data to the buffer. */
        bp->lastStatus = FillReadBuffer(bp);
        if (bp->lastStatus == BUFFER_ERROR ||
            bp->lastStatus == BUFFER_EOF) {
            return bp->lastStatus;
        }
        /* OK or CONTINUE */
    }
    return BUFFER_CONTINUE;
}

static int
IsLineAvailable(ReadBuffer *bp)
{
    return (NextLine(bp) > 0);
}

static int
WaitForNextLine(ReadBuffer *bp, struct timeval *tvPtr)
{
    fd_set readFds;
    int n;

    if (IsLineAvailable(bp)) {
	return 1;
    }
    FD_ZERO(&readFds);
    FD_SET(bp->fd, &readFds);
    n = select(bp->fd+1, &readFds, NULL, NULL, tvPtr);
    return (n > 0);
}

INLINE static void
clear_error(PymolProxy *proxyPtr)
{
    proxyPtr->error = 0;
    proxyPtr->status = TCL_OK;
}


#define STATSDIR	"/var/tmp/visservers"

static int
GetStatsFile(const char *string)
{
    Tcl_DString ds;
    int i;
    char fileName[33];
    const char *path;
    int length;
    char pidstr[200];
    md5_state_t state;
    md5_byte_t digest[16];

    if (string == NULL) {
	return statsFile;
    }
    /* By itself the client's key/value pairs aren't unique.  Add in the
     * process id of this render server. */
    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, string, -1);
    Tcl_DStringAppendElement(&ds, "pid");
    sprintf(pidstr, "%d", getpid());
    Tcl_DStringAppendElement(&ds, pidstr);

    /* Create a md5 hash of the key/value pairs and use it as the file name. */
    string = Tcl_DStringValue(&ds);
    length = strlen(string);
    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)string, length);
    md5_finish(&state, digest);
    for (i = 0; i < 16; i++) {
        sprintf(fileName + i * 2, "%02x", digest[i]);
    }
    Tcl_DStringSetLength(&ds, 0);
    Tcl_DStringAppend(&ds, STATSDIR, -1);
    Tcl_DStringAppend(&ds, "/", 1);
    Tcl_DStringAppend(&ds, fileName, 32);
    path = Tcl_DStringValue(&ds);

    statsFile = open(path, O_EXCL | O_CREAT | O_WRONLY, 0600);
    Tcl_DStringFree(&ds);
    if (statsFile < 0) {
	ERROR("can't open \"%s\": %s", fileName, strerror(errno));
	return -1;
    }
    return statsFile;
}

static int
WriteToStatsFile(int f, const char *s, size_t length)
{

    if (f >= 0) {
	ssize_t numWritten;

        numWritten = write(f, s, length);
        if (numWritten == (ssize_t)length) {
            close(dup(f));
        }
    }
    return 0;
}

static int
ServerStats(int code) 
{
    double start, finish;
    char buf[BUFSIZ];
    Tcl_DString ds;
    int result;
    int f;

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
     *   - Name of render server
     *   - Process ID
     *   - Hostname where server is running
     *   - Start date of session
     *   - Start date of session in seconds
     *   - Number of frames returned
     *   - Number of bytes total returned (in frames)
     *   - Number of commands received
     *   - Total elapsed time of all commands
     *   - Total elapsed time of session
     *   - Exit code of vizserver
     *   - User time 
     *   - System time
     *   - User time of children 
     *   - System time of children 
     */ 

    Tcl_DStringInit(&ds);
    
    Tcl_DStringAppendElement(&ds, "render_stop");
    /* renderer */
    Tcl_DStringAppendElement(&ds, "renderer");
    Tcl_DStringAppendElement(&ds, "pymol");
    /* pid */
    Tcl_DStringAppendElement(&ds, "pid");
    sprintf(buf, "%d", getpid());
    Tcl_DStringAppendElement(&ds, buf);
    /* host */
    Tcl_DStringAppendElement(&ds, "host");
    gethostname(buf, BUFSIZ-1);
    buf[BUFSIZ-1] = '\0';
    Tcl_DStringAppendElement(&ds, buf);
    /* date */
    Tcl_DStringAppendElement(&ds, "date");
    strcpy(buf, ctime(&stats.start.tv_sec));
    buf[strlen(buf) - 1] = '\0';
    Tcl_DStringAppendElement(&ds, buf);
    /* date_secs */
    Tcl_DStringAppendElement(&ds, "date_secs");
    sprintf(buf, "%ld", stats.start.tv_sec);
    Tcl_DStringAppendElement(&ds, buf);
    /* num_frames */
    Tcl_DStringAppendElement(&ds, "num_frames");
    sprintf(buf, "%lu", (unsigned long int)stats.numFrames);
    Tcl_DStringAppendElement(&ds, buf);
    /* frame_bytes */
    Tcl_DStringAppendElement(&ds, "frame_bytes");
    sprintf(buf, "%lu", (unsigned long int)stats.numBytes);
    Tcl_DStringAppendElement(&ds, buf);
    /* num_commands */
    Tcl_DStringAppendElement(&ds, "num_commands");
    sprintf(buf, "%lu", (unsigned long int)stats.numCommands);
    Tcl_DStringAppendElement(&ds, buf);
    /* cmd_time */
    Tcl_DStringAppendElement(&ds, "cmd_time");
    sprintf(buf, "%g", stats.cmdTime);
    Tcl_DStringAppendElement(&ds, buf);
    /* session_time */
    Tcl_DStringAppendElement(&ds, "session_time");
    sprintf(buf, "%g", finish - start);
    Tcl_DStringAppendElement(&ds, buf);
    /* status */
    Tcl_DStringAppendElement(&ds, "status");
    sprintf(buf, "%d", code);
    Tcl_DStringAppendElement(&ds, buf);
    {
	long clocksPerSec = sysconf(_SC_CLK_TCK);
	double clockRes = 1.0 / clocksPerSec;
	struct tms tms;

	memset(&tms, 0, sizeof(tms));
	times(&tms);
	/* utime */
	Tcl_DStringAppendElement(&ds, "utime");
	sprintf(buf, "%g", tms.tms_utime * clockRes);
	Tcl_DStringAppendElement(&ds, buf);
	/* stime */
	Tcl_DStringAppendElement(&ds, "stime");
	sprintf(buf, "%g", tms.tms_stime * clockRes);
	Tcl_DStringAppendElement(&ds, buf);
	/* cutime */
	Tcl_DStringAppendElement(&ds, "cutime");
	sprintf(buf, "%g", tms.tms_cutime * clockRes);
	Tcl_DStringAppendElement(&ds, buf);
	/* cstime */
	Tcl_DStringAppendElement(&ds, "cstime");
	sprintf(buf, "%g", tms.tms_cstime * clockRes);
	Tcl_DStringAppendElement(&ds, buf);
    }
    Tcl_DStringAppend(&ds, "\n", -1);
    f = GetStatsFile(NULL);
    result = WriteToStatsFile(f, Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
    Tcl_DStringFree(&ds);
    close(f);
    return result;
}


static int
CartoonCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    PymolProxy *p = clientData;
    int bool, defer, push, i;
    const char *model;

    clear_error(p);
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
    p->flags |= INVALIDATE_CACHE;	/* cartoon */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    if (bool) {
	SendToPymol(p, "show cartoon,%s\n", model);
    } else {
	SendToPymol(p, "hide cartoon,%s\n", model);
    }
    return p->status;
}

static int
CartoonTraceCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
		const char *argv[])
{
    PymolProxy *p = clientData;
    int bool, defer, push, i;
    const char *model;

    clear_error(p);
    defer = push = bool = FALSE;
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
    p->flags |= INVALIDATE_CACHE; /* cartoon_trace  */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    SendToPymol(p, "set cartoon_trace,%d,%s\n", bool, model);
    return p->status;
}

static int
DisableCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    PymolProxy *p = clientData;
    const char *model = "all";
    int i, defer, push;

    clear_error(p);
    defer = push = FALSE;
    for(i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[i], "-push") == 0 )
            push = 1;
        else
            model = argv[i];
    }

    p->flags |= INVALIDATE_CACHE;	/* disable */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    SendToPymol(p, "disable %s\n", model);
    return p->status;
}


static int
EnableCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	  const char *argv[])
{
    PymolProxy *p = clientData;
    const char *model;
    int i, defer, push;

    clear_error(p);
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
    p->flags |= INVALIDATE_CACHE; /* enable */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    SendToPymol(p, "enable %s\n", model);
    return p->status;
}

static int
FrameCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	 const char *argv[])
{
    PymolProxy *p = clientData;
    int i, push, defer, frame;

    clear_error(p);
    frame = 0;
    push = defer = FALSE;
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
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    p->frame = frame;

    /* Does not invalidate cache? */

    SendToPymol(p,"frame %d\n", frame);
    return p->status;
}

/* 
 * ClientInfoCmd --
 *
 *	  
 *	clientinfo path list
 */
static int
ClientInfoCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	      const char *argv[])
{
    Tcl_DString ds;
    int result;
    int i, numElems;
    const char **elems;
    char buf[BUFSIZ];
    static int first = 1;
    int f;

    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # of arguments: should be \"", argv[0],
		" list\"", (char *)NULL);
	return TCL_ERROR;
    }
    /* Use the initial client key value pairs as the parts for a generating
     * a unique file name. */
    f = GetStatsFile(argv[1]);
    if (f < 0) {
	Tcl_AppendResult(interp, "can't open stats file: ", 
                         Tcl_PosixError(interp), (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_DStringInit(&ds);
    if (first) {
	Tcl_DStringAppendElement(&ds, "render_start");
	first = 0;
    } else {
	Tcl_DStringAppendElement(&ds, "render_info");
    }
    /* renderer */
    Tcl_DStringAppendElement(&ds, "renderer");
    Tcl_DStringAppendElement(&ds, "nanovis");
    /* pid */
    Tcl_DStringAppendElement(&ds, "pid");
    sprintf(buf, "%d", getpid());
    Tcl_DStringAppendElement(&ds, buf);
    /* host */
    Tcl_DStringAppendElement(&ds, "host");
    gethostname(buf, BUFSIZ-1);
    buf[BUFSIZ-1] = '\0';
    Tcl_DStringAppendElement(&ds, buf);
    /* date */
    Tcl_DStringAppendElement(&ds, "date");
    strcpy(buf, ctime(&stats.start.tv_sec));
    buf[strlen(buf) - 1] = '\0';
    Tcl_DStringAppendElement(&ds, buf);
    /* date_secs */
    Tcl_DStringAppendElement(&ds, "date_secs");
    sprintf(buf, "%ld", stats.start.tv_sec);
    Tcl_DStringAppendElement(&ds, buf);

    /* Client arguments. */
    if (Tcl_SplitList(interp, argv[2], &numElems, &elems) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 0; i < numElems; i++) {
	Tcl_DStringAppendElement(&ds, elems[i]);
    }
    free(elems);
    Tcl_DStringAppend(&ds, "\n", 1);
    result = WriteToStatsFile(f, Tcl_DStringValue(&ds), Tcl_DStringLength(&ds));
    Tcl_DStringFree(&ds);
    return result;
}

static int
LabelCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	 const char *argv[])
{
    PymolProxy *p = clientData;
    int i, push, defer, bool, size;
    const char *model;

    clear_error(p);
    model = "all";
    size = 14;
    bool = TRUE;
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
	} else if (Tcl_GetBoolean(interp, argv[i], &bool) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    p->flags |= INVALIDATE_CACHE;	/* label */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    SendToPymol(p, "set label_color,white,%s\nset label_size,%d,%s\n", 
	    model, size, model);
    if (bool) {
        SendToPymol(p, "label %s,\"%%s%%s\" %% (ID,name)\n", model);
    } else {
        SendToPymol(p, "label %s\n", model);
    }
    return p->status;
}

/* 
 * LoadPDBCmd --
 *
 *	Load a PDB into pymol.  We write the pdb data into a temporary file
 *	and then let pymol read it and delete it.  There is no good way to
 *	load PDB data into pymol without using a file.  The specially created
 *	routine "loadandremovepdbfile" in pymol will remove the file after
 *	loading it.  
 *
 */
static int
LoadPDBCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    PymolProxy *p = clientData;
    const char *string;
    const char *name;
    unsigned char *allocated;
    int state, defer, push;
    size_t numBytes;
    int i, j;
    int status;

    if (p == NULL){
	return TCL_ERROR;
    }
    clear_error(p);
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
			 " <data>|follows <model> <state> ?<numBytes>?\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    string = argv[1];
    name   = argv[2];
    if (Tcl_GetInt(interp, argv[3], &state) != TCL_OK) {
	return TCL_ERROR;
    }
    numBytes = 0;
    status = BUFFER_ERROR;
    if (strcmp(string, "follows") == 0) {
	int n;

	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
			 " follows <model> <state> <numBytes>\"", (char *)NULL);
	    return TCL_ERROR;
	}
	if (Tcl_GetInt(interp, argv[4], &n) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (n < 0) {
	    Tcl_AppendResult(interp, "bad value for # bytes \"", argv[4],
			 "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	numBytes = (size_t)n;
    }
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    p->cacheId = state;

    /* Does not invalidate cache? */

    allocated = NULL;
    allocated = malloc(sizeof(char) * numBytes);
    if (allocated == NULL) {
	Tcl_AppendResult(interp, "can't allocate buffer for pdbdata.",
			 (char *)NULL);
	return TCL_ERROR;
    }
    status = ReadFollowingData(&p->client, allocated, numBytes);
    if (status != BUFFER_OK) {
	Tcl_AppendResult(interp, "can't read pdbdata from client.",
			 (char *)NULL);
	free(allocated);
	return TCL_ERROR;
    }
    string = (const char *)allocated;
    {
	int f;
	ssize_t numWritten;
	char fileName[200];

	strcpy(fileName, "/tmp/pdb.XXXXXX");
	p->status = TCL_ERROR;
	f = mkstemp(fileName);
	if (f < 0) {
	    Tcl_AppendResult(interp, "can't create temporary file \"", 
		fileName, "\":", Tcl_PosixError(interp), (char *)NULL);
	    goto error;
	} 
	numWritten = write(f, string, numBytes);
	if (numBytes != numWritten) {
	    Tcl_AppendResult(interp, "can't write PDB data to \"",
		fileName, "\": ", Tcl_PosixError(interp), (char *)NULL);
	    close(f);
	    goto error;
	}
	close(f);
	SendToPymol(p, "loadandremovepdbfile %s,%s,%d\n", fileName, name, 
		    state);
	p->status = TCL_OK;
    }
 error:
    if (allocated != NULL) {
	free(allocated);
    }
    return p->status;
}

static int
OrthoscopicCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	      const char *argv[])
{
    PymolProxy *p = clientData;
    int bool, defer, push, i;

    clear_error(p);
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
    p->flags |= INVALIDATE_CACHE; /* orthoscopic */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    SendToPymol(p, "set orthoscopic=%d\n", bool);
    return p->status;
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
PanCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *p = clientData;
    double x, y;
    int i;
    int defer, push;

    clear_error(p);
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
    p->flags |= INVALIDATE_CACHE;	/* pan */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    if ((x != 0.0f) || (y != 0.0f)) {
	p->xPan = x * 0.05;
	p->yPan = -y * 0.05;
	p->flags |= PAN_PENDING;
    }
    return p->status;
}

static int
PngCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *p = clientData;

    clear_error(p);

    /* Force pymol to update the current scene. */
    SendToPymol(p, "refresh\n");
    /* This is a hack. We're encoding the filename to pass extra information
     * to the MyPNGWrite routine inside of pymol. Ideally these would be
     * parameters of a new "molvispng" command that would be passed all the
     * way through to MyPNGWrite.
     *
     * The extra information is contained in the token we get from the
     * molvisviewer client, the frame number, and rock offset. */
    SendToPymol(p, "png -:%d:%d:%d\n", p->cacheId, p->frame, p->rockOffset);
    return p->status;
}


static int
PpmCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *p = clientData;

    clear_error(p);

    /* Force pymol to update the current scene. */
    SendToPymol(p, "refresh\n");
    /* This is a hack. We're encoding the filename to pass extra information
     * to the MyPNGWrite routine inside of pymol. Ideally these would be
     * parameters of a new "molvispng" command that would be passed all the
     * way through to MyPNGWrite.
     *
     * The extra information is contained in the token we get from the
     * molvisviewer client, the frame number, and rock offset. */
    SendToPymol(p, "png -:%d:%d:%d,format=1\n", p->cacheId, p->frame, 
	    p->rockOffset);
    p->flags &= ~(UPDATE_PENDING|FORCE_UPDATE);
    return p->status;
}


static int
PrintCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	 const char *argv[])
{
    PymolProxy *p = clientData;
    int width, height;
    const char *token, *bgcolor;

    clear_error(p);

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
	SendToPymol(p, "set ray_opaque_background,off\n");
	SendToPymol(p, "refresh\n", bgcolor);
    } else {
	SendToPymol(p, "set ray_opaque_background,on\n");
	SendToPymol(p, "bg_color %s\nrefresh\n", bgcolor);
    }
    /* This is a hack. We're encoding the filename to pass extra information
     * to the MyPNGWrite routine inside of pymol. Ideally these would be
     * parameters of a new "molvispng" command that would be passed all the
     * way through to MyPNGWrite.  
     *
     * The extra information is contained in the token we get from the
     * molvisviewer client, the frame number, and rock offset.
     */
    SendToPymol(p, "png -:%s:0:0,width=%d,height=%d,ray=1,dpi=300\n", 
	    token, width, height);
    SendToPymol(p, "bg_color black\n");
    return p->status;
}

static int
RawCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
{
    PymolProxy *p = clientData;
    int arg, defer = 0, push = 0;
    const char *cmd;
    clear_error(p);

    cmd = NULL;
    defer = push = FALSE;
    for(arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "-defer") == 0)
            defer = 1;
        else if (strcmp(argv[arg], "-push") == 0)
            push = 1;
        else {
            cmd = argv[arg];
        }
    }

    p->flags |= INVALIDATE_CACHE; /* raw */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    SendToPymol(p,"%s\n", cmd);
    return p->status;
}

static int
ResetCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	 const char *argv[])
{
    PymolProxy *p = clientData;
    int arg, push = 0, defer = 0;

    clear_error(p);
    defer = push = 0;
    for(arg = 1; arg < argc; arg++) {
        if ( strcmp(argv[arg],"-defer") == 0 )
            defer = 1;
        else if (strcmp(argv[arg],"-push") == 0 )
            push = 1;
    }
                
    p->flags |= INVALIDATE_CACHE; /* reset */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    SendToPymol(p, "reset\nzoom complete=1\n");
    return p->status;
}

static int
RockCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	const char *argv[])
{
    PymolProxy *p = clientData;
    float y = 0.0;
    int arg, push, defer;

    clear_error(p);

    defer = push = FALSE;
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
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    SendToPymol(p,"turn y, %f\n", y - p->rockOffset);
    p->rockOffset = y;
    return p->status;
}

static int
RepresentationCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
		  const char *argv[])
{
    PymolProxy *p = clientData;
    const char *model;
    const char *rep;
    int defer, push, i;

    clear_error(p);
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

    p->flags |= INVALIDATE_CACHE; /* representation */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    if (strcmp(rep, "ballnstick") == 0) { /* Ball 'n Stick */
	SendToPymol(p, 
	      "set stick_color,white,%s\n"
	      "show sticks,%s\n"
	      "show spheres,%s\n"
	      "hide lines,%s\n"
	      "hide cartoon,%s\n",
	      model, model, model, model, model);
    } else if (strcmp(rep, "spheres") == 0) { /* spheres */    
	SendToPymol(p, 
	      "hide sticks,%s\n"
	      "show spheres,%s\n"
	      "hide lines,%s\n"
	      "hide cartoon,%s\n"
	      "set sphere_quality,2,%s\n"
	      "set ambient,.2,%s\n", 
	      model, model, model, model, model, model);
    } else if (strcmp(rep, "none") == 0) { /* nothing */    
	SendToPymol(p, 
	      "hide sticks,%s\n", 
	      "hide spheres,%s\n"
	      "hide lines,%s\n"
	      "hide cartoon,%s\n",
	      model, model, model, model);
    } else if (strcmp(rep, "sticks") == 0) { /* sticks */    
	SendToPymol(p, 
	      "set stick_color,white,%s\n"
	      "show sticks,%s\n"
	      "hide spheres,%s\n"
	      "hide lines,%s\n"
	      "hide cartoon,%s\n",
	      model, model, model, model, model);
    } else if (strcmp(rep, "lines") == 0) { /* lines */    
	SendToPymol(p, 
	      "hide sticks,%s\n"
	      "hide spheres,%s\n"
	      "show lines,%s\n"
	      "hide cartoon,%s\n",
	      model, model, model, model);
    } else if (strcmp(rep, "cartoon") == 0) { /* cartoon */    
	SendToPymol(p, 
	      "hide sticks,%s\n"
	      "hide spheres,%s\n"
	      "hide lines,%s\n"
	      "show cartoon,%s\n", 
	      model, model, model, model);
    } 
    return p->status;
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
    PymolProxy *p = clientData;
    float xAngle, yAngle, zAngle;
    int defer, push, arg, varg = 1;

    clear_error(p);
    defer = push = 0;
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
    p->flags |= INVALIDATE_CACHE; /* rotate */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    if ((xAngle != 0.0f) || (yAngle != 0.0f) || (zAngle != 0.0f)) {
	p->xAngle += xAngle;
	p->yAngle += yAngle;
	p->zAngle += zAngle;
	p->flags |= ROTATE_PENDING;
    }
    return p->status;
}

static int
ScreenCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	  const char *argv[])
{
    PymolProxy *p = clientData;
    int width = -1, height = -1;
    int defer, push, i, varg;

    clear_error(p);
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
    p->flags |= INVALIDATE_CACHE; /* viewport */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    p->width = width;
    p->height = height;
    p->flags |= VIEWPORT_PENDING;
    return p->status;
}

static int
SphereScaleCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	   const char *argv[])
{
    int defer = 0, push = 0, i;
    double scale;
    const char *model = "all";
    PymolProxy *p = clientData;

    clear_error(p);
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
    p->flags |= INVALIDATE_CACHE;  /* sphere_scale */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    if (strcmp(model, "all") == 0) {
	p->flags |= ATOM_SCALE_PENDING;
	p->sphereScale = scale;
    } else {
	SendToPymol(p, "set sphere_scale,%f,%s\n", scale, model);
    }
    return p->status;
}

static int
StickRadiusCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	       const char *argv[])
{
    PymolProxy *p = clientData;
    int defer = 0, push = 0, i;
    double scale;
    const char *model = "all";

    clear_error(p);
    scale = 0.25f;
    for(i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-defer") == 0 ) {
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
    p->flags |= INVALIDATE_CACHE;  /* stick_radius */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }

    if (strcmp(model, "all") == 0) {
	p->flags |= STICK_RADIUS_PENDING;
	p->stickRadius = scale;
    } else {
	SendToPymol(p, "set stick_radius,%f,%s\n", scale, model);
    }
    return p->status;
}

static int
TransparencyCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
		const char *argv[])
{
    PymolProxy *p = clientData;
    const char *model;
    float transparency;
    int defer, push;
    int i;

    clear_error(p);
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
    p->flags |= INVALIDATE_CACHE; /* transparency */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    } 
    SendToPymol(p, 
	  "set sphere_transparency,%g,%s\n"
	  "set stick_transparency,%g,%s\n"
	  "set cartoon_transparency,%g,%s\n",
	  transparency, model, transparency, model, 
	  transparency, model);
    return p->status;
}

static int
VMouseCmd(ClientData clientData, Tcl_Interp *interp, int argc, 
	  const char *argv[])
{
    PymolProxy *p = clientData;
    int i, defer = 0, push = 0, varg = 1;
    int arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0, arg5 = 0;

    clear_error(p);

    for(i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-defer") == 0)
            defer = 1;
        else if (strcmp(argv[i], "-push") == 0)
            push = 1;
        else if (varg == 1) {
            arg1 = atoi(argv[i]);
            varg++;
        } else if (varg == 2) {
            arg2 = atoi(argv[i]);
            varg++;
        } else if (varg == 3) {
            arg3 = atoi(argv[i]);
            varg++;
        } else if (varg == 4) {
            arg4 = atoi(argv[i]);
            varg++;
        } else if (varg == 5) {
            arg5 = atoi(argv[i]);
            varg++;
        }
    }

    p->flags |= INVALIDATE_CACHE;	/* vmouse */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    SendToPymol(p, "vmouse %d,%d,%d,%d,%d\n", arg1, arg2, arg3, arg4, arg5);
    return p->status;
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
    PymolProxy *p = clientData;
    double factor = 0.0;
    int defer = 0, push = 0, i, varg = 1;

    clear_error(p);

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
    p->flags |= INVALIDATE_CACHE; /* Zoom */
    if (!defer || push) {
	p->flags |= UPDATE_PENDING;
    }
    if (push) {
	p->flags |= FORCE_UPDATE;
    }
    if (factor != 0.0) {
	p->zoom = factor;
	p->flags |= ZOOM_PENDING;
    }
    return p->status;
}

        

static int
ExecuteCommand(Tcl_Interp *interp, Tcl_DString *dsPtr) 
{
    struct timeval tv;
    double start, finish;
    int result;

    gettimeofday(&tv, NULL);
    start = CVT2SECS(tv);

#if EXEC_DEBUG
    DEBUG("command from client is (%s)", Tcl_DStringValue(dsPtr));
#endif
    result = Tcl_Eval(interp, Tcl_DStringValue(dsPtr));
    if (result != TCL_OK) {
#if EXEC_DEBUG
	DEBUG("result was %s\n", Tcl_GetString(Tcl_GetObjResult(interp)));
#endif
    }
    gettimeofday(&tv, NULL);
    finish = CVT2SECS(tv);

    stats.cmdTime += finish - start;
    stats.numCommands++;
    Tcl_DStringSetLength(dsPtr, 0);
    return result;
}

static void
SetViewport(PymolProxy *p)
{
    if (p->flags & VIEWPORT_PENDING) {
	SendToPymol(p, "viewport %d,%d\n", p->width, p->height);
	SendToPymol(p, "refresh\n");
	p->flags &= ~VIEWPORT_PENDING;
    }
}

static void
SetZoom(PymolProxy *p)
{
    if (p->flags & ZOOM_PENDING) {
        SendToPymol(p, "move z,%f\n", p->zoom);
	p->flags &= ~ZOOM_PENDING;
    }
}

static void
SetPan(PymolProxy *p)
{
    if (p->flags & PAN_PENDING) {
	SendToPymol(p, "move x,%f\nmove y,%f\n", p->xPan, p->yPan);
	p->flags &= ~PAN_PENDING;
    }
}

static void
SetRotation(PymolProxy *p)
{
    if (p->flags & ROTATE_PENDING) {
	/* Every pymol command line generates a new rendering. Execute all
	 * three turns as a single command line. */
	SendToPymol(p,"turn x,%f\nturn y,%f\nturn z,%f\n", p->xAngle, p->yAngle, 
		p->zAngle);
	p->xAngle = p->yAngle = p->zAngle = 0.0f;
	p->flags &= ~ROTATE_PENDING;
    }
}

static void
SetSphereScale(PymolProxy *p)
{
    if (p->flags & ATOM_SCALE_PENDING) {
	SendToPymol(p, "set sphere_scale,%f,all\n", p->sphereScale);
	p->flags &= ~ATOM_SCALE_PENDING;
    }
}

static void
SetStickRadius(PymolProxy *p)
{
    if (p->flags & STICK_RADIUS_PENDING) {
	SendToPymol(p, "set stick_radius,%f,all\n", p->stickRadius);
	p->flags &= ~STICK_RADIUS_PENDING;
    }
}

static void
UpdateSettings(PymolProxy *p)
{
    /* Handle all the pending setting changes now. */
    if (p->flags & VIEWPORT_PENDING) {
	SetViewport(p);
    }
    if (p->flags & ROTATE_PENDING) {
	SetRotation(p);
    }
    if (p->flags & PAN_PENDING) {
	SetPan(p);
    }
    if (p->flags & ZOOM_PENDING) {
	SetZoom(p);
    }
    if (p->flags & ATOM_SCALE_PENDING) {
	SetSphereScale(p);
    }
    if (p->flags & STICK_RADIUS_PENDING) {
	SetStickRadius(p);
    }
}

static Image *
NewImage(ImageList *listPtr, size_t dataLength)
{
    Image *imgPtr;
    static int id = 0;

    imgPtr = malloc(sizeof(Image) + dataLength);
    if (imgPtr == NULL) {
	ERROR("can't allocate image of %lu bytes", 
	      (unsigned long)(sizeof(Image) + dataLength));
	abort();
    }
    imgPtr->prevPtr = imgPtr->nextPtr = NULL;
    imgPtr->bytesLeft = dataLength;
    imgPtr->id = id++;
#if WRITE_DEBUG
    DEBUG("NewImage: allocating image %d of %d bytes", imgPtr->id, dataLength);
#endif
    if (listPtr->headPtr != NULL) {
	listPtr->headPtr->prevPtr = imgPtr;
    }
    imgPtr->nextPtr = listPtr->headPtr;
    if (listPtr->tailPtr == NULL) {
	listPtr->tailPtr = imgPtr;
    }
    listPtr->headPtr = imgPtr;
    imgPtr->numWritten = 0;
    return imgPtr;
}

INLINE static void
FreeImage(Image *imgPtr)
{
    assert(imgPtr != NULL);
    free(imgPtr);
}


static void
WriteImages(ImageList *listPtr, int fd)
{
    Image *imgPtr, *prevPtr; 

    if (listPtr->tailPtr == NULL) {
	ERROR("Should not be here: no image available to write");
	return;
    }
#if WRITE_DEBUG
	DEBUG("Entering WriteImages");
#endif
    for (imgPtr = listPtr->tailPtr; imgPtr != NULL; imgPtr = prevPtr) {
	ssize_t bytesLeft;

	assert(imgPtr->nextPtr == NULL);
	prevPtr = imgPtr->prevPtr;
#if WRITE_DEBUG
	DEBUG("WriteImages: image %d of %d bytes.", imgPtr->id, 
	      imgPtr->bytesLeft);
#endif
	for (bytesLeft = imgPtr->bytesLeft; bytesLeft > 0; /*empty*/) {
	    ssize_t numWritten;
#if WRITE_DEBUG
	    DEBUG("image %d: bytesLeft=%d", imgPtr->id, bytesLeft);
#endif
	    numWritten = write(fd, imgPtr->data + imgPtr->numWritten, bytesLeft);
#if WRITE_DEBUG
	    DEBUG("image %d: wrote %d bytes.", imgPtr->id, numWritten);
#endif
	    if (numWritten < 0) {
	        ERROR("Error writing fd=%d, %s", fd, strerror(errno));
#if WRITE_DEBUG
		DEBUG("Abnormal exit WriteImages");
#endif
		return;
	    }
	    bytesLeft -= numWritten;
	    if (bytesLeft > 0) {
#if WRITE_DEBUG
		DEBUG("image %d, wrote a short buffer, %d bytes left.", 
		      imgPtr->id, bytesLeft);
#endif
		/* Wrote a short buffer, means we would block. */
		imgPtr->numWritten += numWritten;
		imgPtr->bytesLeft = bytesLeft;
#if WRITE_DEBUG
		DEBUG("Abnormal exit WriteImages");
#endif
		return;
	    }
	    imgPtr->numWritten += numWritten;
	}
	/* Check if image is on the head.  */
	listPtr->tailPtr = prevPtr;
	if (prevPtr != NULL) {
	    prevPtr->nextPtr = NULL;
	}
	FreeImage(imgPtr);
    }
    listPtr->headPtr = NULL;
#if WRITE_DEBUG
    DEBUG("Exit WriteImages");
#endif
}


static void
ChildHandler(int sigNum) 
{
    ERROR("pymol (%d) died unexpectedly", stats.pid);
    pymolIsAlive = FALSE;
    /*DoExit(1);*/
}

typedef struct {
    const char *name;
    Tcl_CmdProc *proc;
} CmdProc;

static CmdProc cmdProcs[] = {
    { "cartoon",        CartoonCmd	  },       
    { "cartoontrace",   CartoonTraceCmd	  },  
    { "clientinfo",     ClientInfoCmd     }, 
    { "disable",        DisableCmd	  },       
    { "enable",         EnableCmd	  },        
    { "frame",          FrameCmd          },         
    { "label",          LabelCmd          },         
    { "loadpdb",        LoadPDBCmd	  },       
    { "orthoscopic",    OrthoscopicCmd	  },   
    { "pan",            PanCmd		  },           
    { "png",            PngCmd		  },           
    { "ppm",            PpmCmd		  },           
    { "print",          PrintCmd          },         
    { "raw",            RawCmd		  },           
    { "representation", RepresentationCmd },
    { "reset",          ResetCmd          },         
    { "rock",           RockCmd           },          
    { "rotate",         RotateCmd         },        
    { "screen",         ScreenCmd         },        
    { "spherescale",    SphereScaleCmd    },   
    { "stickradius",    StickRadiusCmd    },   
    { "transparency",   TransparencyCmd   },  
    { "viewport",       ScreenCmd         },        
    { "vmouse",         VMouseCmd         },        
    { "zoom",           ZoomCmd           },          
    { NULL,	        NULL              }
};

static int
InitProxy(PymolProxy *p, const char *fileName, char *const *argv)
{
    int sin[2], sout[2];		/* Pipes to connect to server. */
    pid_t child;
    struct timeval end;

#if DEBUG
    DEBUG("Entering InitProxy\n");
#endif
    /* Create tow pipes for communication with the external application. One
     * each for the applications's: stdin and stdout.  */

    if (pipe(sin) == -1) {
        return -1;
    }
    if (pipe(sout) == -1) {
        close(sin[0]);
        close(sin[1]);
        return -1;
    }

    /* Fork the new process.  Connect I/O to the new socket.  */
    child = fork();
    if (child < 0) {
        ERROR("can't fork process: %s\n", strerror(errno));
        return -3;
    }

    if (child == 0) {			/* Child process */
        int f;

        /* 
         * Create a new process group, so we can later kill this process and
         * all its children without affecting the process that created this
         * one.
         */
        setpgid(child, 0); 
        
        /* Redirect stdin, stdout, and stderr to pipes before execing */ 

        dup2(sin[0],  0);		/* Server standard input */
        dup2(sout[1], 1);		/* Server standard output */
	f = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (f < 0) {
	    ERROR("can't open server error file `%s': %s", fileName, 
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
	pymolIsAlive = TRUE;
	signal(SIGCHLD, ChildHandler);
    }
    stats.pid = child;

#if DEBUG
    DEBUG("Started %s DISPLAY=%s\n", argv[0], getenv("DISPLAY"));
#endif
    /* close opposite end of pipe, these now belong to the child process  */
    close(sin[0]);
    close(sout[1]);

    memset(p, 0, sizeof(PymolProxy));
    p->sin        = sin[1];
    p->sout       = sout[0];
    p->cin        = fileno(stdout);
    p->cout       = fileno(stdin);
    p->flags      = CAN_UPDATE;
    p->frame      = 1;
    p->pid      = child;
    InitReadBuffer(&p->client, "client", p->cout, 1<<16);
    InitReadBuffer(&p->server, "server", p->sout, 1<<18);

    /* Create safe interpreter and add pymol-specific commands to it. */
    {
	Tcl_Interp *interp;
	CmdProc *cp;

	interp = Tcl_CreateInterp();
	Tcl_MakeSafe(interp);

	for (cp = cmdProcs; cp->name != NULL; cp++) {
#if DEBUG
	    DEBUG("Adding command %s\n", cp->name);
#endif
	    Tcl_CreateCommand(interp, cp->name, cp->proc, p, NULL);
	}
	p->interp = interp;
    }
    gettimeofday(&end, NULL);
    stats.start = end;
    return 1;
}

static int
FreeProxy(PymolProxy *p)
{
    int result, status;

#if DEBUG
    DEBUG("Enter FreeProxy");
#endif
    close(p->cout);
    close(p->sout);
    close(p->cin);
    close(p->sin);

    Tcl_DeleteInterp(p->interp);
    ServerStats(0);

#if DEBUG
    DEBUG("Waiting for pymol server to exit");
#endif
    alarm(5);
    if (waitpid(p->pid, &result, 0) < 0) {
        ERROR("error waiting on pymol server to exit: %s", strerror(errno));
    }
#if DEBUG
    DEBUG("attempting to signal (SIGTERM) pymol server.");
#endif
    kill(-p->pid, SIGTERM);		// Kill process group
    alarm(5);
    
#if DEBUG
    DEBUG("Waiting for pymol server to exit after SIGTERM");
#endif
    if (waitpid(p->pid, &result, 0) < 0) {
	if (errno != ECHILD) {
	    ERROR("error waiting on pymol server to exit after SIGTERM: %s", 
		  strerror(errno));
	}
    }
    status = -1;
    while ((status == -1) && (errno == EINTR)) {
#if DEBUG
	DEBUG("Attempting to signal (SIGKILL) pymol server.");
#endif
	kill(-p->pid, SIGKILL);		// Kill process group
	alarm(10);
#if DEBUG
	DEBUG("Waiting for pymol server to exit after SIGKILL");
#endif
	status = waitpid(p->pid, &result, 0);
	alarm(0); 
    }
    INFO("pymol server process ended (result=%d)", result);

    if (WIFEXITED(result)) {
	result = WEXITSTATUS(result);
    }
    unlink(stderrFile);
    return result;
}


static void *
ClientToServer(void *clientData)
{
    PymolProxy *p = clientData;
    Tcl_DString command;
    struct timeval tv, *tvPtr;

#if READ_DEBUG
    DEBUG("Reader thread started");
#endif
    Tcl_DStringInit(&command);
    while (pymolIsAlive) {
	tvPtr = NULL;
#if READ_DEBUG
	DEBUG("Start I/O set");
#endif
	while ((pymolIsAlive) && (WaitForNextLine(&p->client, tvPtr))) {
	    size_t numBytes;
	    const char *line;
	    int status;

	    status = GetLine(&p->client, &numBytes, &line);
	    if (status != BUFFER_OK) {
		ERROR("can't read client stdout (numBytes=%d): %s\n", numBytes,
		      strerror(errno));
		goto done;
	    }
	    Tcl_DStringAppend(&command, line, numBytes);
	    if (Tcl_CommandComplete(Tcl_DStringValue(&command))) {
		int result;
		    
		/* May execute more than one command. */
		result = ExecuteCommand(p->interp, &command);
		if (result == TCL_BREAK) {
#if READ_DEBUG
		    DEBUG("TCL_BREAK found");
#endif
		    break;		/* This was caused by a "imgflush"
					 * command. Break out of the read loop
					 * and allow a new image to be
					 * rendered. */
		}
		if (p->flags & FORCE_UPDATE) {
#if READ_DEBUG
		    DEBUG("FORCE_UPDATE set");
#endif
		    break;
		}
	    }
	    tv.tv_sec = 0L;
	    tv.tv_usec = 0L;	/* On successive reads, we break
					 *  out * if no data is
					 *  available. */
	    tvPtr = &tv;			
	}
#if READ_DEBUG
	DEBUG("Finish I/O set");
#endif
	/* Handle all the pending setting changes now. */
	UpdateSettings(p);

	/* We might want to refresh the image if we're not currently
	 * transmitting an image back to the client. The image will be
	 * refreshed after the image has been completely transmitted. */
	if (p->flags & UPDATE_PENDING) {
#if READ_DEBUG
	    DEBUG("calling ppm because of update");
#endif
	    Tcl_Eval(p->interp, "ppm");
	    p->flags &= ~(UPDATE_PENDING|FORCE_UPDATE);
	}
    }
 done:
#if READ_DEBUG
    DEBUG("Leaving Reader thread");
#endif
    return NULL;
}

static void *
ServerToClient(void *clientData)
{
    ReadBuffer *bp = clientData;
    ImageList list;

#if WRITE_DEBUG
    DEBUG("Writer thread started");
#endif
    list.headPtr = list.tailPtr = NULL;
    while (pymolIsAlive) {
	while (WaitForNextLine(bp, NULL)) {
	    Image *imgPtr;
	    const char *line;
	    char header[200];
	    size_t len;
	    int numBytes;
	    size_t hdrLength;
	    int frameNum, rockOffset;
	    char cacheId[200];

	    /* Keep reading lines untils we find a "image follows:" line */
	    if (GetLine(bp, &len, &line) != BUFFER_OK) {
#if WRITE_DEBUG
		DEBUG("Leaving Writer thread");
#endif
		return NULL;
	    }
#if WRITE_DEBUG
	    DEBUG("Writer: line found is %s\n", line);
#endif
	    if (strncmp(line, "image follows: ", 15) != 0) {
		continue;
	    }
	    if (sscanf(line, "image follows: %d %s %d %d\n", &numBytes, cacheId,
		       &frameNum, &rockOffset) != 4) {
		ERROR("can't get # bytes from \"%s\"", line);
		DEBUG("Leaving Writer thread");
		return NULL;
	    } 
#if WRITE_DEBUG
	    DEBUG("found image line \"%.*s\"", numBytes, line);
#endif
	    sprintf(header, "nv>image %d %s %d %d\n", numBytes, cacheId, 
		    frameNum, rockOffset);
	    hdrLength = strlen(header);
#if WRITE_DEBUG
	    DEBUG("Queueing image numBytes=%d cacheId=%s, frameNum=%d, rockOffset=%d size=%d\n", numBytes, cacheId, frameNum, rockOffset, numBytes + hdrLength);
#endif
	    imgPtr = NewImage(&list, numBytes + hdrLength);
	    memcpy(imgPtr->data, header, hdrLength);
	    if (ReadFollowingData(bp, imgPtr->data + hdrLength, 
			(size_t)numBytes) != BUFFER_OK) {
		ERROR("can't read %d bytes for \"image follows\" buffer: %s", 
		      numBytes, strerror(errno));
#if WRITE_DEBUG
		DEBUG("Leaving Writer thread");
#endif
		return NULL;
	    }
	    stats.numFrames++;
	    stats.numBytes += numBytes;
	    {
		struct timeval tv;
		fd_set writeFds;
		int fd;

		tv.tv_sec = tv.tv_usec = 0L;
		FD_ZERO(&writeFds);
		fd = fileno(stdout);
		FD_SET(fd, &writeFds);
		if (select(fd+1, NULL, &writeFds, NULL, &tv) > 0) {
		    WriteImages(&list, fd);
		}
	    }
	}
    }
#if WRITE_DEBUG
    DEBUG("Leaving Writer thread");
#endif
    return NULL;
}

int
main(int argc, char **argv)
{
    PymolProxy proxy;
    pthread_t thread1, thread2;

    sprintf(stderrFile, "/tmp/pymol%d.stderr", getpid());
    fdebug = stderr;
    if (debug) {
        fdebug = fopen("/tmp/pymolproxy.log", "w");
    }    
    frecord = NULL;
    if (recording) {
	char fileName[200];

	sprintf(fileName, "/tmp/pymolproxy%d.py", getpid());
        frecord = fopen(fileName, "w");
    }    
    fprintf(stdout, "PyMol 1.0 (build %s)\n", SVN_VERSION);
    fflush(stdout);

    INFO("Starting pymolproxy (threaded version)");

    InitProxy(&proxy, stderrFile, argv + 1);
    if (pthread_create(&thread1, NULL, &ClientToServer, &proxy) < 0) {
        ERROR("Can't create reader thread: %s", strerror(errno));
    }
    if (pthread_create(&thread2, NULL, &ServerToClient, &proxy.server) < 0) {
        ERROR("Can't create writer thread: %s", strerror(errno));
    }
    if (pthread_join(thread1, NULL) < 0) {
        ERROR("Can't join reader thread: %s", strerror(errno));
    } 
    return FreeProxy(&proxy);
}
