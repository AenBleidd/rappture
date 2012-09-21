/*
 * ----------------------------------------------------------------------
 *  Rappture::signal
 *
 *  This is an interface to the system signal() function, which allows
 *  you to catch and handle system signals.  We use this in Rappture
 *  to catch a SIGHUP and clean up scratch files.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static Tcl_CmdProc RpSignalCmd;
static void RpSignalHandler _ANSI_ARGS_((int signum));
static int RpSignalOptionError _ANSI_ARGS_((Tcl_Interp *interp,
    CONST84 char *option));

/*
 * Global table of all known signal handlers:
 */
typedef struct RpSignals {
    Tcl_Interp *interp;
    Tcl_HashTable *handlers;
} RpSignals;

static RpSignals *SigInfoPtr = NULL;

/*
 * signal configuration options:
 */
typedef struct RpSignalNames {
    char *name;
    int signum;
} RpSignalNames;

#ifdef _WIN32
#define SIGHUP -1
#define SIGQUIT -1
#define SIGKILL -1
#define SIGPIPE -1
#define SIGALRM -1
#define SIGUSR1 -1
#define SIGUSR2 -1
#define SIGCHLD -1
#define SIGCONT -1
#define SIGSTOP -1
#define SIGTSTP -1
#define SIGTTIN -1
#define SIGTTOU -1
#endif

static RpSignalNames signalNames[] = {
    {"SIGHUP",  SIGHUP},   /* Hangup detected on controlling terminal */
    {"SIGINT",  SIGINT},   /* Interrupt from keyboard */
    {"SIGQUIT", SIGQUIT},  /* Quit from keyboard */
    {"SIGILL",  SIGILL},   /* Illegal Instruction */
    {"SIGABRT", SIGABRT},  /* Abort signal from abort(3) */
    {"SIGFPE",  SIGFPE},   /* Floating point exception */
    {"SIGKILL", SIGKILL},  /* Kill signal */
    {"SIGSEGV", SIGSEGV},  /* Invalid memory reference */
    {"SIGPIPE", SIGPIPE},  /* Broken pipe: write to pipe with no readers */
    {"SIGALRM", SIGALRM},  /* Timer signal from alarm(2) */
    {"SIGTERM", SIGTERM},  /* Termination signal */
    {"SIGUSR1", SIGUSR1},  /* User-defined signal 1 */
    {"SIGUSR2", SIGUSR2},  /* User-defined signal 2 */
    {"SIGCHLD", SIGCHLD},  /* Child stopped or terminated */
    {"SIGCONT", SIGCONT},  /* Continue if stopped */
    {"SIGSTOP", SIGSTOP},  /* Stop process */
    {"SIGTSTP", SIGTSTP},  /* Stop typed at tty */
    {"SIGTTIN", SIGTTIN},  /* tty input for background process */
    {"SIGTTOU", SIGTTOU},  /* tty output for background process */
    {(char*)NULL, 0}
};

/*
 * ------------------------------------------------------------------------
 *  RpSignal_Init()
 *
 *  Called in Rappture_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */
int
RpSignal_Init(interp)
    Tcl_Interp *interp;  /* interpreter being initialized */
{
    int i, isnew;
    Tcl_HashTable *htPtr;
    Tcl_HashEntry *entryPtr;

    if (SigInfoPtr != NULL) {
        Tcl_AppendResult(interp, "signals already being handled ",
            "by another interpreter", (char*)NULL);
        return TCL_ERROR;
    }

    /* create the table of signal handlers */
    SigInfoPtr = (RpSignals*)ckalloc(sizeof(RpSignals));
    SigInfoPtr->interp = interp;
    SigInfoPtr->handlers = (Tcl_HashTable*)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(SigInfoPtr->handlers, TCL_STRING_KEYS);

    for (i=0; signalNames[i].name != NULL; i++) {
        entryPtr = Tcl_CreateHashEntry(SigInfoPtr->handlers,
            signalNames[i].name, &isnew);

        htPtr = (Tcl_HashTable*)ckalloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(htPtr, TCL_STRING_KEYS);

        Tcl_SetHashValue(entryPtr, (char*)htPtr);
    }

    /* install the signal command */
    Tcl_CreateCommand(interp, "::Rappture::signal", RpSignalCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpSignalCmd()
 *
 *  Invoked whenever someone uses the "signal" command to get/set
 *  limits for child processes.  Handles the following syntax:
 *
 *      signal ?<signal>? ?<handler>? ?<callback>?
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpSignalCmd(cdata, interp, argc, argv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int argc;                 /* number of command line args */
    CONST84 char *argv[];     /* strings for command line args */
{
    Tcl_HashTable *htPtr;
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch pos;
    int i, isnew, signum;

    /*
     * HANDLE: signal
     *   return a list of all signal names
     */
    if (argc < 2) {
        for (i=0; signalNames[i].name != NULL; i++) {
            Tcl_AppendElement(interp, signalNames[i].name);
        }
        return TCL_OK;
    }

    /*
     * HANDLE: signal <signal>
     *   return a list of handler names for one signal
     */
    if (argc < 3) {
        entryPtr = Tcl_FindHashEntry(SigInfoPtr->handlers, argv[1]);
        if (entryPtr == NULL) {
            return RpSignalOptionError(interp, argv[1]);
        }

        htPtr = (Tcl_HashTable*)Tcl_GetHashValue(entryPtr);
        entryPtr = Tcl_FirstHashEntry(htPtr, &pos);
        while (entryPtr != NULL) {
            Tcl_AppendElement(interp, Tcl_GetHashKey(htPtr, entryPtr));
            entryPtr = Tcl_NextHashEntry(&pos);
        }
        return TCL_OK;
    }

    /*
     * HANDLE: signal <signal> <handler>
     *   return the command for a certain handler on a signal
     */
    if (argc < 4) {
        entryPtr = Tcl_FindHashEntry(SigInfoPtr->handlers, argv[1]);
        if (entryPtr == NULL) {
            return RpSignalOptionError(interp, argv[1]);
        }

        htPtr = (Tcl_HashTable*)Tcl_GetHashValue(entryPtr);
        entryPtr = Tcl_FindHashEntry(htPtr, argv[2]);
        if (entryPtr == NULL) {
            Tcl_AppendResult(interp, "handler \"", argv[2],
                "\" not defined", (char*)NULL);
            return TCL_ERROR;
        }
        Tcl_SetResult(interp, (char*)Tcl_GetHashValue(entryPtr), TCL_VOLATILE);
        return TCL_OK;
    }

    /*
     * HANDLE: signal <signal> <handler> <command>
     *   define the command handler for a signal
     */
    if (argc < 5) {
        entryPtr = Tcl_FindHashEntry(SigInfoPtr->handlers, argv[1]);
        if (entryPtr == NULL) {
            return RpSignalOptionError(interp, argv[1]);
        }

        htPtr = (Tcl_HashTable*)Tcl_GetHashValue(entryPtr);
        entryPtr = Tcl_CreateHashEntry(htPtr, argv[2], &isnew);
        if (*argv[3] == '\0') {
            if (!isnew) {
                free((void*)Tcl_GetHashValue(entryPtr));
            }
            Tcl_DeleteHashEntry(entryPtr);
        } else {
            Tcl_SetHashValue(entryPtr, (ClientData)strdup(argv[3]));
        }

        /*
         * If there are any handlers for this signal, then install
         * a signal handler.  Otherwise, remove the signal handler.
         */
        signum = -1;
        for (i=0; signalNames[i].name != NULL; i++) {
            if (strcmp(argv[1], signalNames[i].name) == 0) {
                signum = signalNames[i].signum;
                break;
            }
        }
        if (Tcl_FirstHashEntry(htPtr, &pos) != NULL) {
            (void) signal(signum, RpSignalHandler);
        } else {
            (void) signal(signum, SIG_DFL);
        }
        return TCL_OK;
    }

    Tcl_AppendResult(interp, "wrong # args: should be \"",
        argv[0], " ?signal? ?handler? ?command?\"", (char*)NULL);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  RpSignalHandler()
 *
 *  Invoked whenever the "signal" command is deleted, to clean up
 *  all storage in the handler table.
 * ------------------------------------------------------------------------
 */
static void
RpSignalHandler(signum)
    int signum;               /* signal being handled */
{
    int i, status;
    Tcl_HashTable *htPtr;
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch pos;
    char *cmd;

    for (i=0; signalNames[i].name != NULL; i++) {
        if (signalNames[i].signum == signum) {
            break;
        }
    }
    if (signalNames[i].name != NULL) {
        entryPtr = Tcl_FindHashEntry(SigInfoPtr->handlers, signalNames[i].name);
        if (entryPtr) {
            htPtr = (Tcl_HashTable*)Tcl_GetHashValue(entryPtr);

            /* execute all of the handlers associated with this signal */
            entryPtr = Tcl_FirstHashEntry(htPtr, &pos);
            while (entryPtr) {
                cmd = (char*)Tcl_GetHashValue(entryPtr);

                status = Tcl_Eval(SigInfoPtr->interp, cmd);
                if (status != TCL_OK) {
                    Tcl_BackgroundError(SigInfoPtr->interp);
                }
                entryPtr = Tcl_NextHashEntry(&pos);
            }
        }
    }
    Tcl_ResetResult(SigInfoPtr->interp);
}

/*
 * ------------------------------------------------------------------------
 *  RpSignalOptionError()
 *
 *  Used internally to set an error message in the interpreter
 *  describing an unrecognized signal option.  Sets the result of
 *  the Tcl interpreter and returns an error status code.
 * ------------------------------------------------------------------------
 */
static int
RpSignalOptionError(interp, option)
    Tcl_Interp *interp;       /* interpreter handling this request */
    CONST84 char *option;     /* bad option supplied by the user */
{
    int i;

    Tcl_AppendResult(interp, "bad signal \"", option,
        "\": should be one of ",(char*)NULL);

    for (i=0; signalNames[i].name != NULL; i++) {
        if (i > 0) {
            Tcl_AppendResult(interp, ", ", (char*)NULL);
        }
        Tcl_AppendResult(interp, signalNames[i].name, (char*)NULL);
    }
    return TCL_ERROR;
}
