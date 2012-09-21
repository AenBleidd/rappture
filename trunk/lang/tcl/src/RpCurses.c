/* 
 * ----------------------------------------------------------------------
 *  RpCurses - interface to the ncurses library for Rappture apps
 *
 *  Provides access to curses functions, so that tty-based programs
 *  can write out status info to a screen in the manner of "top" or
 *  "sftp".
 *
 *  if {[Rappture::curses isatty]} {
 *    Rappture::curses start
 *    Rappture::curses puts ?-at y x? ?-bold? ?-clear? ?-dim? ?-reverse? <str>
 *    Rappture::curses refresh
 *    Rappture::curses stop
 *  }
 *  
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "tcl.h"
#include <string.h>
#include <ncurses.h>
#include <unistd.h>

static CONST char *commandNames[] = {
    "beep", "clear", "flash", "getch",
    "isatty", "puts", "refresh", "screensize",
    "start", "stop", (char*)NULL
};

enum command {
    COMMAND_BEEP, COMMAND_CLEAR, COMMAND_FLASH, COMMAND_GETCH,
    COMMAND_ISATTY, COMMAND_PUTS, COMMAND_REFRESH, COMMAND_SCREENSIZE,
    COMMAND_START, COMMAND_STOP
};

/*
 * Forward declarations
 */
static int		CursesObjCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));

/*
 * ------------------------------------------------------------------------
 *  RpCurses_Init --
 *
 *  Invoked when the Rappture GUI library is being initialized
 *  to install the "curses" command into the interpreter.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpCurses_Init(interp)
    Tcl_Interp *interp;         /* interpreter being initialized */
{
    /* install the command */
    Tcl_CreateObjCommand(interp, "Rappture::curses", CursesObjCmd,
        NULL, NULL);

    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CursesObjCmd()
 *
 * Called whenever the "curses" command is invoked to access ncurses
 * library functions.
 * ----------------------------------------------------------------------
 */
static int
CursesObjCmd(clientData, interp, objc, objv)
    ClientData clientData;        /* not used */
    Tcl_Interp *interp;           /* current interpreter */
    int objc;                     /* number of command arguments */
    Tcl_Obj *CONST objv[];        /* command argument objects */
{
    int result = TCL_OK;
    int pos, cmdToken;
    char *opt;
    
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
        return TCL_ERROR;
    }

    /*
     * Look up the command option and map it to an enumerated value.
     */
    result = Tcl_GetIndexFromObj(interp, objv[1], commandNames,
            "option", 0, &cmdToken);

    if (result != TCL_OK) {
        return result;
    }

    switch (cmdToken) {
        case COMMAND_PUTS: {
            int flags = 0;  /* attribute flags -- bold, dim, reverse, etc */
            int clear = 0;  /* non-zero => clear to EOL before printing */
            int y, x;

            pos = 2;
            while (pos < objc) {
                opt = Tcl_GetStringFromObj(objv[pos], (int*)NULL);
                if (*opt != '-') {
                    break;
                }
                else if (strcmp(opt,"-at") == 0) {
                    if (objc-pos < 2) {
                        Tcl_WrongNumArgs(interp, 2, objv, "-at y x string");
                        return TCL_ERROR;
                    }
                    if (Tcl_GetIntFromObj(interp, objv[pos+1], &y) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    if (Tcl_GetIntFromObj(interp, objv[pos+2], &x) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    move(y,x);
                    pos += 3;
                }
                else if (strcmp(opt,"-bold") == 0) {
                    flags |= A_BOLD;
                    pos++;
                }
                else if (strcmp(opt,"-dim") == 0) {
                    flags |= A_DIM;
                    pos++;
                }
                else if (strcmp(opt,"-reverse") == 0) {
                    flags |= A_REVERSE;
                    pos++;
                }
                else if (strcmp(opt,"-underline") == 0) {
                    flags |= A_UNDERLINE;
                    pos++;
                }
                else if (strcmp(opt,"-clear") == 0) {
                    clear = 1;
                    pos++;
                }
                else if (strcmp(opt,"--") == 0) {
                    pos++;
                    break;
                }
                else {
                    Tcl_AppendResult(interp, "bad option \"", opt,
                        "\": should be -at, -bold, -clear, -dim, -reverse, -underline", (char*)NULL);
                    return TCL_ERROR;
                }
            }
            if (pos >= objc) {
                Tcl_WrongNumArgs(interp, 2, objv, "?-at y x? ?-bold? ?-clear? ?-dim? ?-reverse? ?-underline? string");
                return TCL_ERROR;
            }

            if (clear) clrtoeol();
            if (flags != 0) attron(flags);
            printw(Tcl_GetStringFromObj(objv[pos],(int*)NULL));
            if (flags != 0) attroff(flags);
            break;
        }

        case COMMAND_GETCH: {
            int c;
            char *opt, str[10];
            int tdelay = 100;  /* time delay for blocking in ms */

            pos = 2;
            while (pos < objc) {
                opt = Tcl_GetStringFromObj(objv[pos], (int*)NULL);
                if (*opt != '-') {
                    break;
                }
                else if (strcmp(opt,"-wait") == 0) {
                    if (objc-pos < 1) {
                        Tcl_WrongNumArgs(interp, 2, objv, "-wait ms");
                        return TCL_ERROR;
                    }
                    opt = Tcl_GetStringFromObj(objv[pos+1], (int*)NULL);
                    if (strcmp(opt,"forever") == 0) {
                        tdelay = -1;
                    }
                    else if (Tcl_GetIntFromObj(interp, objv[pos+1], &tdelay) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    pos += 2;
                }
                else {
                    Tcl_AppendResult(interp, "bad option \"", opt,
                        "\": should be -wait", (char*)NULL);
                    return TCL_ERROR;
                }
            }
            if (pos < objc) {
                Tcl_WrongNumArgs(interp, 2, objv, "?-wait ms?");
            }

            timeout(tdelay);
            c = getch();

            /*
             * BLECH! Some systems don't use the proper termcap info, so
             *   they don't map special keys the way they should.  Instead,
             *   they send escape sequences for arrows, such as ^[A.
             *   Look for this and map to the proper key value here.
             */
            if (c == 0x1b) {  /* escape key */
                timeout(0);   /* look for the next key */
                c = getch();
                if (c == 'O' || c == '[') {
                    c = getch();
                    if (c == 'A') {
                        c = KEY_UP;
                    } else if (c == 'B') {
                        c = KEY_DOWN;
                    } else if (c == 'C') {
                        c = KEY_LEFT;
                    } else if (c == 'D') {
                        c = KEY_RIGHT;
                    } else {
                        c = 0x1b;
                    }
                } else {
                    c = 0x1b;
                }
            }

            if (c == KEY_UP) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("up",-1));
            } else if (c == KEY_DOWN) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("down",-1));
            } else if (c == KEY_LEFT) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("left",-1));
            } else if (c == KEY_RIGHT) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("right",-1));
            } else if (c != ERR) {
                str[0] = c;
                str[1] = '\0';
                Tcl_SetObjResult(interp, Tcl_NewStringObj(str,1));
            }
            break;
        }

        case COMMAND_ISATTY: {
            Tcl_SetObjResult(interp, Tcl_NewBooleanObj(isatty(0) == 1));
            break;
        }

        case COMMAND_CLEAR: {
            clear();
            break;
        }

        case COMMAND_REFRESH: {
            refresh();
            break;
        }

        case COMMAND_SCREENSIZE: {
            int rows,cols;
            Tcl_Obj *listPtr;

            getmaxyx(stdscr,rows,cols);
            listPtr = Tcl_NewListObj(0, (Tcl_Obj**)NULL);
            Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(rows));
            Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(cols));
            Tcl_SetObjResult(interp, listPtr);
            break;
        }

        case COMMAND_BEEP: {
            beep();
            break;
        }

        case COMMAND_FLASH: {
            flash();
            break;
        }

        case COMMAND_START: {
            initscr();
            raw();      /* can press keys without Enter to control things */
            noecho();   /* don't see the control keypresses */
            break;
        }

        case COMMAND_STOP: {
            endwin();
            break;
        }
    }
    return result;
}
