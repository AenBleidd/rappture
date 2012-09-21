/*
 * ----------------------------------------------------------------------
 *  Rappture::daemon
 *
 *  Programs call this to dissociate themselves from their parents,
 *  so they can continue to run in the background as a daemon process.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

static int RpDaemonCmd _ANSI_ARGS_((ClientData cdata,
    Tcl_Interp *interp, int argc, CONST84 char *argv[]));

/*
 * ------------------------------------------------------------------------
 *  RpDaemon_Init()
 *
 *  Called in Rappture_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */
int
RpDaemon_Init(interp)
    Tcl_Interp *interp;  /* interpreter being initialized */
{
    /* install the daemon command */
    Tcl_CreateCommand(interp, "::Rappture::daemon", RpDaemonCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpDaemonCmd()
 *
 *  Invoked whenever someone uses the "daemon" command to fork
 *  and dissociate this process from its parent.  Handles the
 *  following syntax:
 *
 *      daemon
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 *
 *  For details, see W. Richard Stevens, "Advanced Programming in
 *  the Unix Environment," p. 418, Addison Wesley, 1992.
 * ------------------------------------------------------------------------
 */
static int
RpDaemonCmd(cdata, interp, argc, argv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int argc;                 /* number of command line args */
    CONST84 char *argv[];     /* strings for command line args */
{
    pid_t pid;
    int result;

    pid = fork();
    if (pid < 0) {
        Tcl_AppendResult(interp, "can't fork daemon", (char*)NULL);
        if (errno == EAGAIN) {
            Tcl_AppendResult(interp, ": resource limit",
                (char*)NULL);
        } else if (errno == ENOMEM) {
            Tcl_AppendResult(interp, ": out of memory",
                (char*)NULL);
        }
        return TCL_ERROR;
    }
    else if (pid != 0) {
        Tcl_Exit(0);  /* parent exits here */
    }

    /* child continues... */
    setsid();    /* become session leader */
    result = chdir("/");  /* root never goes away, so sit here */
    assert(result == 0);
    /* close at least this much, so pipes in exec'ing process get closed */
    Tcl_UnregisterChannel(interp, Tcl_GetStdChannel(TCL_STDIN));
    Tcl_UnregisterChannel(interp, Tcl_GetStdChannel(TCL_STDOUT));
    Tcl_UnregisterChannel(interp, Tcl_GetStdChannel(TCL_STDERR));

    /* make sure that these are really closed */
    close(0);
    close(1);
    close(2);

    Tcl_ResetResult(interp);  /* in case there was an error above */
    return TCL_OK;
}
