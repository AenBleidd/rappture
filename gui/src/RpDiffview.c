/* 
 * ----------------------------------------------------------------------
 *  RpDiffview - like a textbox, but for showing diffs
 *
 *  Loads two strings, performs a diff, and shows the results.  Use
 *  it as follows:
 *
 *  Rappture::Diffview .dv \
 *      -addedbackground blue -addedforeground white \
 *      -deletedbackground red -deletedforeground white -overstrike true \
 *      -changedbackground black -changedforeground yellow \
 *      -xscrollcommand {.x set} -yscrollcommand {.y set}
 *  
 *  scrollbar .x -orient horizontal -command {.dv xview}
 *  scrollbar .y -orient vertical -command {.dv yview}
 *  
 *  .dv text 1 [exec cat /tmp/file1]
 *  .dv text 2 [exec cat /tmp/file2]
 *  .dv configure -diff 2->1 -layout sidebyside
 *  
 *  puts "DIFF OUTPUT:"
 *  puts [.dv diffs -std]
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "tk.h"
#include <string.h>
#include <stdlib.h>

/*
 * Special options for controlling diffs.
 */
enum diffdir {
    DIFF_1TO2, DIFF_2TO1
};
static char *diffdirStrings[] = {
    "1->2", "2->1", (char*)NULL
};

enum layoutStyle {
    LAYOUT_INLINE, LAYOUT_SIDEBYSIDE
};
static char *layoutStyleStrings[] = {
    "inline", "sidebyside", (char*)NULL
};

enum scancommand {
    SCAN_MARK, SCAN_DRAGTO
};
static CONST char *scanCommandNames[] = {
    "mark", "dragto", (char *) NULL
};


/*
 * Data structure for line start/end for all lines in a buffer.
 */
typedef struct {
    int numLines;		/* number of lines */
    int maxLines;		/* max size of storage in startPtr/endPtr */
    char **startPtr;		/* array of start pointers for each line */
    int *lenPtr;		/* array of lengths for each line */
} DiffviewLines;

/*
 * Data structure for text buffers within the widget:
 */
typedef struct {
    Tcl_Obj *textObj;		/* actual text within this buffer */
    DiffviewLines *lineLimits;	/* breakdown of textObj into line starts/ends */
} DiffviewBuffer;

/*
 * Data structure used internally by diff routine:
 */
typedef struct subseq {
    int index1;			/* index in buffer #1 */
    int index2;			/* index in buffer #2 */
    int next;			/* next candidate match in the chain */
} DiffviewSubseq;

/*
 * Data structure returned by diff routine:
 */
typedef struct {
    int op;			/* operation: 'a'=add 'd'=del 'c'=change */
    int fromIndex1;		/* starts at this line in buffer 1 */
    int toIndex1;		/* ends at this line in buffer 1 */
    int fromIndex2;		/* starts at this line in buffer 2 */
    int toIndex2;		/* ends at this line in buffer 2 */
    int fromWorld;		/* starts at this line in world view */
    int toWorld;		/* ends at this line in world view */
} DiffviewDiffOp;

typedef struct {
    int maxDiffs;		/* maximum storage space for diffs */
    int numDiffs;		/* number of diffs stored in ops */
    DiffviewDiffOp *ops;	/* list of diffs */
} DiffviewDiffs;

/*
 * Data structure used for each line of the final layout:
 */
typedef struct {
    char style;			/* layout style: 'a'=add, 'd'=del, etc. */
    char bnum;			/* show line from this buffer */
    int bline;			/* line number in buffer bnum */
    int diffNum;		/* line is part of this diff */
} DiffviewLayoutLine;

typedef struct {
    int maxLines;		/* maximum number of lines allocated */
    int numLines;		/* current number of lines in layout */
    DiffviewLayoutLine *lines;	/* info for all lines in world view */
} DiffviewLayout;

/*
 * Data structure for the widget:
 */
typedef struct {
    Tk_Window tkwin;		/* tk window for the widget */
    Display *display;		/* display containing the widget */
    Tcl_Interp *interp;		/* interpreter containing the widget command */
    Tcl_Command widgetCmd;	/* token for the widget command */
    Tk_OptionTable optionTable;	/* configuration options */

    /*
     * Used to display the widget:
     */
    Tk_Cursor cursor;		/* cursor for the widget */
    Tk_3DBorder normalBorder;	/* border around the widget */
    int borderWidth;		/* width of the border in pixels */
    int relief;			/* relief: raised, sunken, etc. */
    int highlightWidth;		/* width of border showing focus highlight */
    XColor *highlightBgColor;	/* color for focus highlight bg */
    XColor *highlightColor;	/* color for focus highlight */
    int inset;			/* width of all borders--offset to ul corner */

    DiffviewBuffer buffer[2];	/* text to diff -- buffers #1 and #2 */
    DiffviewDiffs *diffsPtr;    /* diff: longest common subseq btwn buffers */
    DiffviewLayout worldview;   /* array mapping each real line in world
				 * coordinates to the corresponding line
				 * in buffer #1/#2, along with its style */

    enum diffdir diffdir;	/* diff between these buffers */
    enum layoutStyle layout;	/* layout style (side-by-side or inline) */
    int maxWidth;		/* maximum width of world view in pixels */
    int maxHeight;		/* maximum height of world view in pixels */
    int lineHeight;		/* height of a line with current font */
    int lineAscent;		/* ascent of a line with current font */
    int topLine;		/* index of topmost line in view */
    int btmLine;		/* index of bottommost line in view */
    int fullLines;		/* number of full lines that fit in view */

    int xOffset;		/* offset in pixels to left edge of view */
    int xScrollUnit;		/* num pixels for one unit of horizontal
				 * scrolling (one average character) */
    int yOffset;		/* offset in pixels to top edge of view */
    int yScrollUnit;		/* num pixels for one unit of vertical
				 * scrolling (one average line) */
    char *takeFocus;		/* value of -takefocus option */
    char *yScrollCmd;		/* command prefix for scrolling */
    char *xScrollCmd;		/* command prefix for scrolling */

    int scanMarkX;		/* x-coord for "scan mark" */
    int scanMarkY;		/* y-coord for "scan mark" */
    int scanMarkXStart;		/* xOffset at start of scan */
    int scanMarkYStart;		/* yOffset at start of scan */


    Tk_Font tkfont;		/* text font for content of widget */
    Tk_Font tklastfont;		/* previous text font (so we detect changes) */
    XColor *fgColorPtr;		/* normal foreground color */
    XColor *addBgColorPtr;	/* background for "added" text in diff */
    XColor *addFgColorPtr;	/* foreground for "added" text in diff */
    XColor *delBgColorPtr;	/* background for "deleted" text in diff */
    XColor *delFgColorPtr;	/* foreground for "deleted" text in diff */
    XColor *chgBgColorPtr;	/* background for "changed" text in diff */
    XColor *chgFgColorPtr;	/* foreground for "changed" text in diff */
    int overStrDel;		/* non-zero => overstrike deleted text */
    GC normGC;			/* GC for drawing normal text */
    GC addFgGC;			/* GC for drawing "added" text in diff */
    GC delFgGC;			/* GC for drawing "deleted" text in diff */
    GC chgFgGC;			/* GC for drawing "changed" text in diff */
    int width;			/* overall width of widget, in pixels */
    int height;			/* overall height of widget, in pixels */

    int flags;			/* flag bits for redraw, etc. */
} Diffview;

/*
 * Tk widget flag bits:
 *
 * REDRAW_PENDING:	DoWhenIdle handler has been queued to redraw
 * UPDATE_V_SCROLLBAR:	non-zero => vertical scrollbar needs updating
 * UPDATE_H_SCROLLBAR:	non-zero => horizontal scrollbar needs updating
 * GOT_FOCUS:		non-zero => widget has input focus
 * FONT_CHANGED:	font changed, so layout needs to be recomputed
 * WIDGET_DELETED:	widget has been destroyed, so don't update
 */

#define REDRAW_PENDING		1
#define UPDATE_V_SCROLLBAR	2
#define UPDATE_H_SCROLLBAR	4
#define GOT_FOCUS		8
#define FONT_CHANGED		16
#define WIDGET_DELETED		32


/*
 * Default option values.
 */
#define BLACK				"#000000"
#define WHITE				"#ffffff"
#define NORMAL_BG			"#d9d9d9"

#define DEF_DIFFVIEW_ADDBG		"#ccffcc"
#define DEF_DIFFVIEW_ADDFG		BLACK
#define DEF_DIFFVIEW_BG_COLOR		NORMAL_BG
#define DEF_DIFFVIEW_BG_MONO		WHITE
#define DEF_DIFFVIEW_BORDERWIDTH	"2"
#define DEF_DIFFVIEW_CHGBG		"#ffffcc"
#define DEF_DIFFVIEW_CHGFG		BLACK
#define DEF_DIFFVIEW_CURSOR		""
#define DEF_DIFFVIEW_DELBG		"#ffcccc"
#define DEF_DIFFVIEW_DELFG		"#666666"
#define DEF_DIFFVIEW_DIFF		"1->2"
#define DEF_DIFFVIEW_FG			BLACK
#define DEF_DIFFVIEW_FONT		"Courier -12"
#define DEF_DIFFVIEW_HEIGHT		"2i"
#define DEF_DIFFVIEW_HIGHLIGHT_BG	NORMAL_BG
#define DEF_DIFFVIEW_HIGHLIGHT		BLACK
#define DEF_DIFFVIEW_HIGHLIGHT_WIDTH	"1"
#define DEF_DIFFVIEW_LAYOUT		"inline"
#define DEF_DIFFVIEW_OVERSTRDEL		"true"
#define DEF_DIFFVIEW_RELIEF		"sunken"
#define DEF_DIFFVIEW_TAKE_FOCUS		(char*)NULL
#define DEF_DIFFVIEW_WIDTH		"2i"
#define DEF_DIFFVIEW_SCROLL_CMD		""

/*
 * Configuration options for this widget.
 * (must be in alphabetical order)
 */
static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_COLOR, "-addedbackground", "addedBackground", "Background",
	 DEF_DIFFVIEW_ADDBG, -1, Tk_Offset(Diffview, addBgColorPtr),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-addedforeground", "addedForeground", "Foreground",
	 DEF_DIFFVIEW_ADDFG, -1, Tk_Offset(Diffview, addFgColorPtr),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BORDER, "-background", "background", "Background",
	 DEF_DIFFVIEW_BG_COLOR, -1, Tk_Offset(Diffview, normalBorder),
	 0, (ClientData) DEF_DIFFVIEW_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bd", (char*)NULL, (char*)NULL,
	 (char*)NULL, 0, -1, 0, (ClientData) "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", (char*)NULL, (char*)NULL,
	 (char*)NULL, 0, -1, 0, (ClientData) "-background", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	 DEF_DIFFVIEW_BORDERWIDTH, -1, Tk_Offset(Diffview, borderWidth),
	 0, 0, 0},
    {TK_OPTION_COLOR, "-changedbackground", "changedBackground", "Background",
	 DEF_DIFFVIEW_CHGBG, -1, Tk_Offset(Diffview, chgBgColorPtr),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-changedforeground", "changedForeground", "Foreground",
	 DEF_DIFFVIEW_CHGFG, -1, Tk_Offset(Diffview, chgFgColorPtr),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	 DEF_DIFFVIEW_CURSOR, -1, Tk_Offset(Diffview, cursor),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-deletedbackground", "deletedBackground", "Background",
	 DEF_DIFFVIEW_DELBG, -1, Tk_Offset(Diffview, delBgColorPtr),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-deletedforeground", "deletedForeground", "Foreground",
	 DEF_DIFFVIEW_DELFG, -1, Tk_Offset(Diffview, delFgColorPtr),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING_TABLE, "-diff", "diff", "Diff",
        DEF_DIFFVIEW_DIFF, -1, Tk_Offset(Diffview, diffdir),
        0, (ClientData)diffdirStrings, 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", (char*)NULL,
	 (char*)NULL, 0, -1, 0, (ClientData) "-foreground", 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
	 DEF_DIFFVIEW_FONT, -1, Tk_Offset(Diffview, tkfont), 0, 0, 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	 DEF_DIFFVIEW_FG, -1, Tk_Offset(Diffview, fgColorPtr), 0, 0, 0},
    {TK_OPTION_PIXELS, "-height", "height", "Height",
	 DEF_DIFFVIEW_HEIGHT, -1, Tk_Offset(Diffview, height), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
	 "HighlightBackground", DEF_DIFFVIEW_HIGHLIGHT_BG, -1, 
	 Tk_Offset(Diffview, highlightBgColor), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	 DEF_DIFFVIEW_HIGHLIGHT, -1, Tk_Offset(Diffview, highlightColor),
	 0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	 "HighlightThickness", DEF_DIFFVIEW_HIGHLIGHT_WIDTH, -1,
	 Tk_Offset(Diffview, highlightWidth), 0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-layout", "layout", "Layout",
        DEF_DIFFVIEW_LAYOUT, -1, Tk_Offset(Diffview, layout),
        0, (ClientData)layoutStyleStrings, 0},
    {TK_OPTION_BOOLEAN, "-overstrike", "overstrike", "Overstrike",
	 DEF_DIFFVIEW_OVERSTRDEL, -1, Tk_Offset(Diffview, overStrDel), 0, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	 DEF_DIFFVIEW_RELIEF, -1, Tk_Offset(Diffview, relief), 0, 0, 0},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	 DEF_DIFFVIEW_TAKE_FOCUS, -1, Tk_Offset(Diffview, takeFocus),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-width", "width", "Width",
	 DEF_DIFFVIEW_WIDTH, -1, Tk_Offset(Diffview, width), 0, 0, 0},
    {TK_OPTION_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	 DEF_DIFFVIEW_SCROLL_CMD, -1, Tk_Offset(Diffview, xScrollCmd),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	 DEF_DIFFVIEW_SCROLL_CMD, -1, Tk_Offset(Diffview, yScrollCmd),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, (char*)NULL, (char*)NULL, (char*)NULL,
	(char*)NULL, 0, -1, 0, 0, 0}
};

/*
 * Widget commands:
 */
static CONST char *commandNames[] = {
    "bbox", "cget", "configure",
    "diffs", "scan", "see",
    "text", "xview", "yview",
    (char*)NULL
};

enum command {
    COMMAND_BBOX, COMMAND_CGET, COMMAND_CONFIGURE,
    COMMAND_DIFFS, COMMAND_SCAN, COMMAND_SEE,
    COMMAND_TEXT, COMMAND_XVIEW, COMMAND_YVIEW
};

/*
 * Forward declarations for remaining procedures.
 */
static int		DiffviewObjCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		DiffviewWidgetObjCmd _ANSI_ARGS_((ClientData clientData,
	                    Tcl_Interp *interp, int objc,
	                    Tcl_Obj *CONST objv[]));
static int		DiffviewBboxSubCmd _ANSI_ARGS_ ((Tcl_Interp *interp,
			    Diffview *dvPtr, int objc, Tcl_Obj *CONST objv[]));
static int		DiffviewDiffsSubCmd _ANSI_ARGS_ ((Tcl_Interp *interp,
	                    Diffview *dvPtr, int objc, Tcl_Obj *CONST objv[]));
static int		DiffviewTextSubCmd _ANSI_ARGS_ ((Tcl_Interp *interp,
			    Diffview *dvPtr, int objc, Tcl_Obj *CONST objv[]));
static int		DiffviewXviewSubCmd _ANSI_ARGS_ ((Tcl_Interp *interp,
			    Diffview *dvPtr, int objc, Tcl_Obj *CONST objv[]));
static int		DiffviewYviewSubCmd _ANSI_ARGS_ ((Tcl_Interp *interp,
			    Diffview *dvPtr, int objc, Tcl_Obj *CONST objv[]));
static int		DiffviewGetIndex _ANSI_ARGS_ ((Tcl_Interp *interp,
			    Diffview *dvPtr, Tcl_Obj *objPtr, int *linePtr,
			    DiffviewDiffOp **diffOpPtrPtr));

static void		DestroyDiffview _ANSI_ARGS_((char *memPtr));
static int		ConfigureDiffview _ANSI_ARGS_((Tcl_Interp *interp,
			    Diffview *dvPtr, int objc, Tcl_Obj *CONST objv[],
			    int flags));
static void		DiffviewWorldChanged _ANSI_ARGS_((
			    ClientData instanceData));
static void		EventuallyRedraw _ANSI_ARGS_((Diffview *dvPtr));
static void		DisplayDiffview _ANSI_ARGS_((ClientData clientData));
static void		DiffviewComputeGeometry _ANSI_ARGS_((Diffview *dvPtr));
static void		DiffviewEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		DiffviewCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		ChangeDiffviewView _ANSI_ARGS_((Diffview *dvPtr,
			    int lnum));
static void		DiffviewScanTo _ANSI_ARGS_((Diffview *dvPtr,
			    int x, int y));
static void		DiffviewUpdateLayout _ANSI_ARGS_((
    			    Diffview *dvPtr));
static void		DiffviewUpdateVScrollbar _ANSI_ARGS_((
			    Diffview *dvPtr));
static void		DiffviewUpdateHScrollbar _ANSI_ARGS_((
    			    Diffview *dvPtr));
static DiffviewLines*	DiffviewLinesCreate _ANSI_ARGS_((char *textPtr,
			    int textLen));
static void		DiffviewLinesFree _ANSI_ARGS_((
			    DiffviewLines *lineLimitsPtr));
static DiffviewDiffs*	DiffviewDiffsCreate _ANSI_ARGS_((
			    char *textPtr1, DiffviewLines *limsPtr1,
			    char *textPtr2, DiffviewLines *limsPtr2));
static void		DiffviewDiffsAppend _ANSI_ARGS_((
			    DiffviewDiffs *diffsPtr, int op,
			    int fromIndex1, int toIndex1,
			    int fromIndex2, int toIndex2));
static void		DiffviewDiffsFree _ANSI_ARGS_((
			    DiffviewDiffs *diffsPtr));
static void		DiffviewLayoutAdd _ANSI_ARGS_((
			    DiffviewLayout *layoutPtr,
			    DiffviewLayoutLine *linePtr));
static void		DiffviewLayoutClear _ANSI_ARGS_((
			    DiffviewLayout *layoutPtr));
static void		DiffviewLayoutFree _ANSI_ARGS_((
			    DiffviewLayout *layoutPtr));

/*
 * Standard Tk procs invoked from generic window code.
 */
static Tk_ClassProcs diffviewClass = {
    sizeof(Tk_ClassProcs),	/* size */
    DiffviewWorldChanged,	/* worldChangedProc */
};

/*
 * ------------------------------------------------------------------------
 *  RpDiffview_Init --
 *
 *  Invoked when the Rappture GUI library is being initialized
 *  to install the "diffview" widget into the interpreter.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpDiffview_Init(interp)
    Tcl_Interp *interp;         /* interpreter being initialized */
{
    static char *script = "source [file join $RapptureGUI::library scripts diffview.tcl]";

    /* install the widget command */
    Tcl_CreateObjCommand(interp, "Rappture::Diffview", DiffviewObjCmd,
        NULL, NULL);

    /* load the default bindings */
    if (Tcl_Eval(interp, script) != TCL_OK) {
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewObjCmd()
 *
 * Called whenever a Diffview object is created to create the widget
 * and install the Tcl access command.
 * ----------------------------------------------------------------------
 */
static int
DiffviewObjCmd(clientData, interp, objc, objv)
    ClientData clientData;        /* not used */
    Tcl_Interp *interp;                /* current interpreter */
    int objc;                        /* number of command arguments */
    Tcl_Obj *CONST objv[];        /* command argument objects */
{
    Diffview *dvPtr;
    Tk_Window tkwin;

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "pathName ?options?");
        return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp),
                Tcl_GetString(objv[1]), (char*)NULL);

    if (tkwin == NULL) {
        return TCL_ERROR;
    }

    /*
     * Create a data structure for the widget objects and set good
     * initial values.
     */
    dvPtr = (Diffview*)ckalloc(sizeof(Diffview));
    memset((void*)dvPtr, 0, (sizeof(Diffview)));

    dvPtr->tkwin   = tkwin;
    dvPtr->display = Tk_Display(tkwin);
    dvPtr->interp  = interp;
    dvPtr->widgetCmd = Tcl_CreateObjCommand(interp,
            Tk_PathName(dvPtr->tkwin), DiffviewWidgetObjCmd,
            (ClientData)dvPtr, DiffviewCmdDeletedProc);
    dvPtr->optionTable = Tk_CreateOptionTable(interp, optionSpecs);

    dvPtr->buffer[0].textObj = NULL;
    dvPtr->buffer[0].lineLimits = NULL;
    dvPtr->buffer[1].textObj = NULL;
    dvPtr->buffer[1].lineLimits = NULL;
    dvPtr->diffdir = DIFF_1TO2;

    dvPtr->cursor = None;
    dvPtr->relief = TK_RELIEF_SUNKEN;
    dvPtr->normGC = None;
    dvPtr->addFgGC = None;
    dvPtr->delFgGC = None;
    dvPtr->chgFgGC = None;
    dvPtr->xScrollUnit = 1;
    dvPtr->yScrollUnit = 1;

    /*
     * Use reference counting to decide when this data should be
     * cleaned up.
     */
    Tcl_Preserve((ClientData) dvPtr->tkwin);

    Tk_SetClass(dvPtr->tkwin, "Diffview");
    Tk_SetClassProcs(dvPtr->tkwin, &diffviewClass, (ClientData)dvPtr);

    Tk_CreateEventHandler(dvPtr->tkwin,
            ExposureMask|StructureNotifyMask|FocusChangeMask,
            DiffviewEventProc, (ClientData)dvPtr);

    if (Tk_InitOptions(interp, (char*)dvPtr, dvPtr->optionTable, tkwin)
            != TCL_OK) {
        Tk_DestroyWindow(dvPtr->tkwin);
        return TCL_ERROR;
    }

    if (ConfigureDiffview(interp, dvPtr, objc-2, objv+2, 0) != TCL_OK) {
        Tk_DestroyWindow(dvPtr->tkwin);
        return TCL_ERROR;
    }

    Tcl_SetResult(interp, Tk_PathName(dvPtr->tkwin), TCL_STATIC);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewWidgetObjCmd()
 *
 * Called to process the operations associated with the widget.  Handles
 * the widget access command, using the operations defined in the
 * commandNames[] array.
 * ----------------------------------------------------------------------
 */
static int
DiffviewWidgetObjCmd(clientData, interp, objc, objv)
    ClientData clientData;    /* widget data */
    Tcl_Interp *interp;       /* interp handling this command */
    int objc;                 /* number of command arguments */
    Tcl_Obj *CONST objv[];    /* command arguments */
{
    Diffview *dvPtr = (Diffview*)clientData;
    int result = TCL_OK;
    int cmdToken;
    
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

    /* hang on to the widget data until we exit... */
    Tcl_Preserve((ClientData)dvPtr);

    switch (cmdToken) {
        case COMMAND_BBOX: {
            result = DiffviewBboxSubCmd(interp, dvPtr, objc, objv);
            break;
        }

        case COMMAND_CGET: {
            Tcl_Obj *objPtr;
            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 2, objv, "option");
                result = TCL_ERROR;
                break;
            }

            objPtr = Tk_GetOptionValue(interp, (char*)dvPtr,
                    dvPtr->optionTable, objv[2], dvPtr->tkwin);
            if (objPtr == NULL) {
                result = TCL_ERROR;
                break;
            }
            Tcl_SetObjResult(interp, objPtr);
            result = TCL_OK;
            break;
        }
        
        case COMMAND_CONFIGURE: {
            Tcl_Obj *objPtr;
            if (objc <= 3) {
                /* query the value of an option */
                objPtr = Tk_GetOptionInfo(interp, (char*)dvPtr,
                        dvPtr->optionTable,
                        (objc == 3) ? objv[2] : (Tcl_Obj*)NULL,
                        dvPtr->tkwin);
                if (objPtr == NULL) {
                    result = TCL_ERROR;
                    break;
                } else {
                    Tcl_SetObjResult(interp, objPtr);
                    result = TCL_OK;
                }
            } else {
                /* set one or more configuration options */
                result = ConfigureDiffview(interp, dvPtr, objc-2, objv+2, 0);
            }
            break;
        }

        case COMMAND_DIFFS: {
            result = DiffviewDiffsSubCmd(interp, dvPtr, objc, objv);
            break;
        }

        case COMMAND_SCAN: {
            int x, y, scanCmdIndex;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 2, objv, "mark|dragto x y");
                result = TCL_ERROR;
                break;
            }

            if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK
                    || Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
                result = TCL_ERROR;
                break;
            }

            result = Tcl_GetIndexFromObj(interp, objv[2], scanCommandNames,
                    "option", 0, &scanCmdIndex);
            if (result != TCL_OK) {
                break;
            }
            switch (scanCmdIndex) {
                case SCAN_MARK: {
                    dvPtr->scanMarkX = x;
                    dvPtr->scanMarkY = y;
                    dvPtr->scanMarkXStart = dvPtr->xOffset;
                    dvPtr->scanMarkYStart = dvPtr->yOffset;
                    break;
                }
                case SCAN_DRAGTO: {
                    DiffviewScanTo(dvPtr, x, y);
                    break;
                }
            }
            result = TCL_OK;
            break;
        }

        case COMMAND_SEE: {
            DiffviewDiffOp *diffOpPtr;
            int line1, line2, topline;

            if (objc != 3) {
                Tcl_WrongNumArgs(interp, 2, objv, "index");
                result = TCL_ERROR;
                break;
            }
            result = DiffviewGetIndex(interp, dvPtr, objv[2],
                &line1, &diffOpPtr);
            if (result != TCL_OK) {
                break;
            }

            if (line1 < 0 && diffOpPtr != NULL) {
                line1 = diffOpPtr->fromWorld;
                line2 = diffOpPtr->toWorld;
            } else {
                line2 = line1;
            }

            if (line1 >= dvPtr->worldview.numLines) {
                line1 = line2 = dvPtr->worldview.numLines-1;
            }
            if (line1 < 0) {
                line1 = line2 = 0;
            }

            if (line1 < dvPtr->topLine || line1 > dvPtr->btmLine) {
                /* location off screen? then center it in y-view */
                if (line2-line1 >= dvPtr->fullLines) {
                    topline = line1;
                } else {
                    topline = (line1+line2)/2 - dvPtr->fullLines/2;
                    if (topline < 0) topline = 0;
                }
                ChangeDiffviewView(dvPtr, topline);
            }
            result = TCL_OK;
            break;
        }

        case COMMAND_TEXT: {
            result = DiffviewTextSubCmd(interp, dvPtr, objc, objv);
            break;
        }

        case COMMAND_XVIEW: {
            result = DiffviewXviewSubCmd(interp, dvPtr, objc, objv);
            break;
        }
        
        case COMMAND_YVIEW: {
            result = DiffviewYviewSubCmd(interp, dvPtr, objc, objv);
            break;
        }
    }

    /* okay, release the widget data and return */
    Tcl_Release((ClientData)dvPtr);
    return result;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewBboxSubCmd()
 *
 * Handles the "bbox" operation on the widget.  Returns a bounding
 * box for the specified part of the widget.
 * ----------------------------------------------------------------------
 */
static int
DiffviewBboxSubCmd(interp, dvPtr, objc, objv)
    Tcl_Interp *interp;       /* interp handling this command */
    Diffview *dvPtr;          /* widget data */
    int objc;                 /* number of command arguments */
    Tcl_Obj *CONST objv[];    /* command arguments */
{
    char *opt, buf[256];
    DiffviewDiffOp *diffOpPtr;
    int line, bnum, bline, x1, y0, y1, wd;
    char *textPtr; int textLen;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "index");
        return TCL_ERROR;
    }

    /* make sure that our layout info is up to date for queries below */
    DiffviewUpdateLayout(dvPtr);

    opt = Tcl_GetString(objv[2]);
    if (*opt == 'a' && strcmp(opt,"all") == 0) {
        sprintf(buf, "0 0 %d %d", dvPtr->maxWidth, dvPtr->maxHeight);
        Tcl_SetResult(interp, buf, TCL_VOLATILE);
        return TCL_OK;
    }

    if (DiffviewGetIndex(interp, dvPtr, objv[2], &line, &diffOpPtr) != TCL_OK) {
        Tcl_AppendResult(interp, " or all", (char*)NULL);
        return TCL_ERROR;
    }

    /* return the bounding box for a line in world coords */
    if (line >= 0) {
        y0 = line*dvPtr->lineHeight;
        y1 = (line+1)*dvPtr->lineHeight;
        x1 = 0;
        if (line < dvPtr->worldview.numLines) {
            bnum = dvPtr->worldview.lines[line].bnum;
            bline = dvPtr->worldview.lines[line].bline;
            if (bline >= 0) {
                textPtr = dvPtr->buffer[bnum].lineLimits->startPtr[bline];
                textLen = dvPtr->buffer[bnum].lineLimits->lenPtr[bline];
                x1 = Tk_TextWidth(dvPtr->tkfont, textPtr, textLen);
            }
        }
        sprintf(buf, "0 %d %d %d", y0, x1, y1);
        Tcl_SetResult(interp, buf, TCL_VOLATILE);
        return TCL_OK;
    }

    /* return the bounding box for a diff in world coords */
    if (diffOpPtr) {
        y0 = diffOpPtr->fromWorld*dvPtr->lineHeight;
        y1 = (diffOpPtr->toWorld+1)*dvPtr->lineHeight;
        x1 = 0;

        for (line=diffOpPtr->fromWorld; line <= diffOpPtr->toWorld; line++) {
            if (line < dvPtr->worldview.numLines) {
                bnum = dvPtr->worldview.lines[line].bnum;
                bline = dvPtr->worldview.lines[line].bline;
                if (bline >= 0) {
                    textPtr = dvPtr->buffer[bnum].lineLimits->startPtr[bline];
                    textLen = dvPtr->buffer[bnum].lineLimits->lenPtr[bline];
                    wd = Tk_TextWidth(dvPtr->tkfont, textPtr, textLen);
                    if (wd > x1) {
                        x1 = wd;
                    }
                }
            }
        }
        sprintf(buf, "0 %d %d %d", y0, x1, y1);
        Tcl_SetResult(interp, buf, TCL_VOLATILE);
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewDiffsSubCmd()
 *
 * Handles the "diffs" operation on the widget with the following
 * syntax:
 *
 *   widget diffs ?-std|-debug|-number?
 *
 * The default is "diffs -std", which returns the usual output from
 * the Unix "diff" command.  The -debug option returns a more detailed
 * output that is useful for testing/debugging.  The -number option
 * returns the total number of diffs.  This is useful for commands
 * such as "see #2", where you can bring a particular diff into view.
 * ----------------------------------------------------------------------
 */
static int
DiffviewDiffsSubCmd(interp, dvPtr, objc, objv)
    Tcl_Interp *interp;       /* interp handling this command */
    Diffview *dvPtr;          /* widget data */
    int objc;                 /* number of command arguments */
    Tcl_Obj *CONST objv[];    /* command arguments */
{
    Tcl_Obj *resultPtr;
    char range1[128], range2[128];
    int i, n;

    static CONST char *options[] = {
        "-std",		"-debug",	"-number",	(char*)NULL
    };
    enum options {
        DIFFS_STD,	DIFFS_DEBUG,	DIFFS_NUMBER
    };
    int op = DIFFS_STD;


    if (objc > 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "?-std? ?-debug? ?-number?");
        return TCL_ERROR;
    }

    /* decode the option that controls the return result */
    if (objc > 2) {
        if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0,
                &op) != TCL_OK) {
            return TCL_ERROR;
        }
    }

    /* make sure that our layout info is up to date for queries below */
    DiffviewUpdateLayout(dvPtr);

    switch ((enum options)op) {
        case DIFFS_STD: {
            DiffviewDiffOp curr, *currOpPtr;
            DiffviewLines *lines1, *lines2;

            if (dvPtr->diffsPtr) {
                resultPtr = Tcl_GetObjResult(interp);
                for (i=0; i < dvPtr->diffsPtr->numDiffs; i++) {
                    currOpPtr = &dvPtr->diffsPtr->ops[i];

                    /* if we're diff'ing the other way, reverse the diff */
                    if (dvPtr->diffdir == DIFF_1TO2) {
                        memcpy((VOID*)&curr, (VOID*)currOpPtr,
                            sizeof(DiffviewDiffOp));
                        lines1 = dvPtr->buffer[0].lineLimits;
                        lines2 = dvPtr->buffer[1].lineLimits;
                    } else {
                        switch (currOpPtr->op) {
                            case 'a': curr.op = 'd'; break;
                            case 'd': curr.op = 'a'; break;
                            default:  curr.op = currOpPtr->op; break;
                        }
                        curr.fromIndex1 = currOpPtr->fromIndex2;
                        curr.toIndex1   = currOpPtr->toIndex2;
                        curr.fromIndex2 = currOpPtr->fromIndex1;
                        curr.toIndex2   = currOpPtr->toIndex1;
                        lines1 = dvPtr->buffer[1].lineLimits;
                        lines2 = dvPtr->buffer[0].lineLimits;
                    }

                    /* append the diff info onto the output */
                    if (curr.fromIndex1 == curr.toIndex1) {
                        if (curr.op == 'a') {
                            /* for append, use first index at raw value */
                            sprintf(range1, "%d", curr.fromIndex1);
                        } else {
                            sprintf(range1, "%d", curr.fromIndex1+1);
                        }
                    } else {
                        sprintf(range1, "%d,%d", curr.fromIndex1+1,
                            curr.toIndex1+1);
                    }

                    if (curr.fromIndex2 == curr.toIndex2) {
                        if (curr.op == 'd') {
                            /* for delete, use second index at raw value */
                            sprintf(range2, "%d", curr.fromIndex2);
                        } else {
                            sprintf(range2, "%d", curr.fromIndex2+1);
                        }
                    } else {
                        sprintf(range2, "%d,%d", curr.fromIndex2+1,
                            curr.toIndex2+1);
                    }

                    switch (curr.op) {
                        case 'a': {
                            Tcl_AppendToObj(resultPtr, range1, -1);
                            Tcl_AppendToObj(resultPtr, "a", 1);
                            Tcl_AppendToObj(resultPtr, range2, -1);
                            Tcl_AppendToObj(resultPtr, "\n", 1);
                            for (n=curr.fromIndex2; n <= curr.toIndex2; n++) {
                                Tcl_AppendToObj(resultPtr, "> ", 2);
                                Tcl_AppendToObj(resultPtr, lines2->startPtr[n],
                                    lines2->lenPtr[n]);
                                Tcl_AppendToObj(resultPtr, "\n", 1);
                            }
                            break;
                        }
                        case 'd': {
                            Tcl_AppendToObj(resultPtr, range1, -1);
                            Tcl_AppendToObj(resultPtr, "d", 1);
                            Tcl_AppendToObj(resultPtr, range2, -1);
                            Tcl_AppendToObj(resultPtr, "\n", 1);
                            for (n=curr.fromIndex1; n <= curr.toIndex1; n++) {
                                Tcl_AppendToObj(resultPtr, "< ", 2);
                                Tcl_AppendToObj(resultPtr, lines1->startPtr[n],
                                    lines1->lenPtr[n]);
                                Tcl_AppendToObj(resultPtr, "\n", 1);
                            }
                            break;
                        }
                        case 'c': {
                            Tcl_AppendToObj(resultPtr, range1, -1);
                            Tcl_AppendToObj(resultPtr, "c", 1);
                            Tcl_AppendToObj(resultPtr, range2, -1);
                            Tcl_AppendToObj(resultPtr, "\n", 1);
                            for (n=curr.fromIndex1; n <= curr.toIndex1; n++) {
                                Tcl_AppendToObj(resultPtr, "< ", 2);
                                Tcl_AppendToObj(resultPtr, lines1->startPtr[n],
                                    lines1->lenPtr[n]);
                                Tcl_AppendToObj(resultPtr, "\n", 1);
                            }
                            Tcl_AppendToObj(resultPtr, "---\n", 4);
                            for (n=curr.fromIndex2; n <= curr.toIndex2; n++) {
                                Tcl_AppendToObj(resultPtr, "> ", 2);
                                Tcl_AppendToObj(resultPtr, lines2->startPtr[n],
                                    lines2->lenPtr[n]);
                                Tcl_AppendToObj(resultPtr, "\n", 1);
                            }
                            break;
                        }
                    }
                }
            }
            break;
        }
        case DIFFS_DEBUG: {
            DiffviewDiffOp *c;
            char elem[256];

            if (dvPtr->diffsPtr) {
                for (i=0; i < dvPtr->diffsPtr->numDiffs; i++) {
                    c = &dvPtr->diffsPtr->ops[i];

                    /* append the diff info onto the output */
                    if (c->fromIndex1 == c->toIndex1) {
                        sprintf(range1, "%d", c->fromIndex1);
                    } else {
                        sprintf(range1, "%d,%d", c->fromIndex1, c->toIndex1);
                    }

                    if (c->fromIndex2 == c->toIndex2) {
                        sprintf(range2, "%d", c->fromIndex2);
                    } else {
                        sprintf(range2, "%d,%d", c->fromIndex2, c->toIndex2);
                    }

                    sprintf(elem, "%c %s %s", c->op, range1, range2);
                    Tcl_AppendElement(interp, elem);
                }
            }
            break;
        }
        case DIFFS_NUMBER: {
            int num = 0;
            if (dvPtr->diffsPtr) {
                num = dvPtr->diffsPtr->numDiffs;
            }
            resultPtr = Tcl_NewIntObj(num);
            Tcl_SetObjResult(interp, resultPtr);
            break;
        }
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewTextSubCmd()
 *
 * Handles the "text" operation on the widget with the following
 * syntax:
 *
 *   widget text 1 ?string?
 *   widget text 2 ?string?
 *
 * Used to get/set the text being diff'd and displayed in the widget.
 * ----------------------------------------------------------------------
 */
static int
DiffviewTextSubCmd(interp, dvPtr, objc, objv)
    Tcl_Interp *interp;       /* interp handling this command */
    Diffview *dvPtr;          /* widget data */
    int objc;                 /* number of command arguments */
    Tcl_Obj *CONST objv[];    /* command arguments */
{
    int bufferNum;
    DiffviewBuffer *slotPtr;

    if (objc < 2 || objc > 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "bufferNum ?string?");
        return TCL_ERROR;
    }

    /* decode the buffer number -- right now only buffers 1 & 2 */
    if (Tcl_GetIntFromObj(interp, objv[2], &bufferNum) != TCL_OK) {
        return TCL_ERROR;
    }
    if (bufferNum < 1 || bufferNum > 2) {
        Tcl_AppendResult(interp, "bad buffer number \"",
            Tcl_GetString(objv[2]), "\": should be 1 or 2",
            (char*)NULL);
        return TCL_ERROR;
    }
    slotPtr = &dvPtr->buffer[bufferNum-1];

    if (objc == 3) {
        /* return current contents of the buffer */
        if (slotPtr->textObj != NULL) {
            Tcl_SetObjResult(interp, slotPtr->textObj);
        } else {
            Tcl_ResetResult(interp);
        }
        return TCL_OK;
    }

    /* set and return the new contents of the buffer */
    if (slotPtr->textObj != NULL) {
        Tcl_DecrRefCount(slotPtr->textObj);
        DiffviewLinesFree(slotPtr->lineLimits);
        slotPtr->lineLimits = NULL;
    }
    slotPtr->textObj = objv[3];
    Tcl_IncrRefCount(slotPtr->textObj);

    EventuallyRedraw(dvPtr);

    Tcl_SetObjResult(interp, slotPtr->textObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewXviewSubCmd()
 *
 * Handles the "xview" operation on the widget.  Adjusts the x-axis view
 * according to some request, which usually comes from a scrollbar.
 * Supports the standard Tk operations for "xview".
 * ----------------------------------------------------------------------
 */
static int
DiffviewXviewSubCmd(interp, dvPtr, objc, objv)
    Tcl_Interp *interp;       /* interp handling this command */
    Diffview *dvPtr;          /* widget data */
    int objc;                 /* number of command arguments */
    Tcl_Obj *CONST objv[];    /* command arguments */
{

    int offset = 0;
    int index, count, type, viewWidth, windowUnits, maxOffset;
    double fraction, fraction2;
    
    viewWidth = Tk_Width(dvPtr->tkwin) - 2*dvPtr->inset;

    if (objc == 2) {
        /*
         * COMMAND: widget xview
         * return current limits as fractions 0-1
         */
        if (dvPtr->maxWidth == 0) {
            Tcl_SetResult(interp, "0 1", TCL_STATIC);
        } else {
            char buf[TCL_DOUBLE_SPACE*2+2];

            fraction = dvPtr->xOffset/((double)dvPtr->maxWidth);
            fraction2 = (dvPtr->xOffset + viewWidth)
                /((double)dvPtr->maxWidth);
            if (fraction2 > 1.0) {
                fraction2 = 1.0;
            }
            sprintf(buf, "%g %g", fraction, fraction2);
            Tcl_SetResult(interp, buf, TCL_VOLATILE);
        }
    } else {
        if (objc == 3) {
            /*
             * COMMAND: widget xview index
             * set left edge to character at index
             */
            if (Tcl_GetIntFromObj(interp, objv[2], &index) != TCL_OK) {
                return TCL_ERROR;
            }
            offset = index*dvPtr->xScrollUnit;
        } else {
            /*
             * COMMAND: widget xview moveto fraction
             * COMMAND: widget xview scroll number what
             * handles more complex scrolling movements
             */
            type = Tk_GetScrollInfoObj(interp, objc, objv, &fraction, &count);
            switch (type) {
                case TK_SCROLL_ERROR:
                    return TCL_ERROR;
                case TK_SCROLL_MOVETO:
                    offset = (int)(fraction*dvPtr->maxWidth + 0.5);
                    break;
                case TK_SCROLL_PAGES:
                    windowUnits = viewWidth/dvPtr->xScrollUnit;
                    if (windowUnits > 2) {
                        offset = dvPtr->xOffset
                            + count*dvPtr->xScrollUnit*(windowUnits-2);
                    } else {
                        offset = dvPtr->xOffset + count*dvPtr->xScrollUnit;
                    }
                    break;
                case TK_SCROLL_UNITS:
                    offset = dvPtr->xOffset + count*dvPtr->xScrollUnit;
                    break;
            }
        }
        maxOffset = dvPtr->maxWidth
            - (Tk_Width(dvPtr->tkwin) - 2*dvPtr->inset)
            + (dvPtr->xScrollUnit - 1);
        if (offset > maxOffset) { offset = maxOffset; }
        if (offset < 0) { offset = 0; }

        /* nudge back to an even boundary */
        offset -= offset % dvPtr->xScrollUnit;

        if (offset != dvPtr->xOffset) {
            dvPtr->xOffset = offset;
            dvPtr->flags |= UPDATE_H_SCROLLBAR;
            EventuallyRedraw(dvPtr);
        }
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewYviewSubCmd()
 *
 * Handles the "yview" operation on the widget.  Adjusts the y-axis view
 * according to some request, which usually comes from a scrollbar.
 * Supports the standard Tk operations for "yview".
 * ----------------------------------------------------------------------
 */
static int
DiffviewYviewSubCmd(interp, dvPtr, objc, objv)
    Tcl_Interp *interp;       /* interp handling this command */
    Diffview *dvPtr;          /* widget data */
    int objc;                 /* number of command arguments */
    Tcl_Obj *CONST objv[];    /* command arguments */
{
    int index, yLines, count, type;
    double fraction, fraction2;

    /* make sure that our layout info is up to date */
    DiffviewUpdateLayout(dvPtr);

    if (objc == 2) {
        /*
         * COMMAND: widget yview
         * return current limits as fractions 0-1
         */
        if (dvPtr->maxHeight == 0) {
            Tcl_SetResult(interp, "0 1", TCL_STATIC);
        } else {
            char buf[TCL_DOUBLE_SPACE * 2+2];

            yLines = dvPtr->maxHeight/dvPtr->lineHeight;
            fraction = dvPtr->topLine/((double)yLines);
            fraction2 = (dvPtr->topLine+dvPtr->fullLines)/((double)yLines);
            if (fraction2 > 1.0) {
                fraction2 = 1.0;
            }
            sprintf(buf, "%g %g", fraction, fraction2);
            Tcl_SetResult(interp, buf, TCL_VOLATILE);
        }
    } else if (objc == 3) {
        /*
         * COMMAND: widget yview index
         * set top edge to line at index
         */
        if (Tcl_GetIntFromObj(interp, objv[2], &index) != TCL_OK) {
            return TCL_ERROR;
        }
        ChangeDiffviewView(dvPtr, index);
    } else {
        /*
         * COMMAND: widget yview moveto fraction
         * COMMAND: widget yview scroll number what
         * handles more complex scrolling movements
         */
        type = Tk_GetScrollInfoObj(interp, objc, objv, &fraction, &count);
        switch (type) {
            case TK_SCROLL_ERROR:
                return TCL_ERROR;
            case TK_SCROLL_MOVETO:
                yLines = dvPtr->maxHeight/dvPtr->lineHeight;
                index = (int)(yLines*fraction + 0.5);
                break;
            case TK_SCROLL_PAGES:
                yLines = dvPtr->maxHeight/dvPtr->lineHeight;
                if (dvPtr->fullLines > 2) {
                    index = dvPtr->topLine + count*(dvPtr->fullLines-2);
                } else {
                    index = dvPtr->topLine + count;
                }
                break;
            case TK_SCROLL_UNITS:
                index = dvPtr->topLine + count;
                break;
        }
        ChangeDiffviewView(dvPtr, index);
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewGetIndex()
 *
 * Used by many of the widget operations to parse an index into the
 * widget view.  Handles the following syntax:
 *   #NN ...... particular diff number, starting from 1
 *   NN ....... particular line number in world view, starting from 0
 *
 * Returns TCL_OK if successful, and TCL_ERROR (along with an error
 * message) if anything goes wrong.
 * ----------------------------------------------------------------------
 */
static int
DiffviewGetIndex(interp, dvPtr, objPtr, linePtr, diffOpPtrPtr)
    Tcl_Interp *interp;             /* interp handling this command */
    Diffview *dvPtr;                /* widget data */
    Tcl_Obj *objPtr;                /* argument being parsed */
    int *linePtr;                   /* returns: line number or -1 */
    DiffviewDiffOp **diffOpPtrPtr;  /* returns: specific diff or NULL */
{
    int result = TCL_OK;
    char *opt = Tcl_GetString(objPtr);
    char *tail;
    int dnum;

    /* clear the return values */
    if (linePtr) *linePtr = -1;
    if (diffOpPtrPtr) *diffOpPtrPtr = NULL;

    if (*opt == '#') {
        dnum = strtol(opt+1, &tail, 10);
        if (*tail != '\0' || dnum <= 0) {
           result = TCL_ERROR;
        }
        else if (dvPtr->diffsPtr == NULL
                   || dnum > dvPtr->diffsPtr->numDiffs) {
           Tcl_AppendResult(interp, "diff \"", opt, "\" doesn't exist",
               (char*)NULL);
           return TCL_ERROR;
        }
        *diffOpPtrPtr = &dvPtr->diffsPtr->ops[dnum-1];
    } else {
        result = Tcl_GetIntFromObj(interp, objPtr, linePtr);
    }

    if (result != TCL_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "bad index \"", opt, "\": should be "
            "\"#n\" for diff, or an integer value for line number",
            (char*)NULL);
    }
    return result;
}

/*
 * ----------------------------------------------------------------------
 * DestroyDiffview()
 *
 * Used by Tcl_Release/Tcl_EventuallyFree to clean up the data associated
 * with a widget when it is no longer being used.  This happens at the
 * end of a widget destruction sequence.
 * ----------------------------------------------------------------------
 */
static void
DestroyDiffview(memPtr)
    char *memPtr;             /* widget data */
{
    Diffview *dvPtr = (Diffview*)memPtr;
    int bnum;

    /*
     * Clean up text associated with the diffs.
     */
    for (bnum=0; bnum < 2; bnum++) {
        if (dvPtr->buffer[bnum].textObj != NULL) {
            Tcl_DecrRefCount(dvPtr->buffer[bnum].textObj);
        }
        if (dvPtr->buffer[bnum].lineLimits != NULL) {
            DiffviewLinesFree(dvPtr->buffer[bnum].lineLimits);
        }
    }

    if (dvPtr->diffsPtr) {
        DiffviewDiffsFree(dvPtr->diffsPtr);
    }
    DiffviewLayoutFree(&dvPtr->worldview);

    /*
     * Free up GCs and configuration options.
     */
    if (dvPtr->normGC != None) {
        Tk_FreeGC(dvPtr->display, dvPtr->normGC);
    }
    if (dvPtr->addFgGC != None) {
        Tk_FreeGC(dvPtr->display, dvPtr->addFgGC);
    }
    if (dvPtr->delFgGC != None) {
        Tk_FreeGC(dvPtr->display, dvPtr->delFgGC);
    }
    if (dvPtr->chgFgGC != None) {
        Tk_FreeGC(dvPtr->display, dvPtr->chgFgGC);
    }

    Tk_FreeConfigOptions((char*)dvPtr, dvPtr->optionTable, dvPtr->tkwin);
    Tcl_Release((ClientData) dvPtr->tkwin);
    dvPtr->tkwin = NULL;
    ckfree((char*)dvPtr);
}

/*
 * ----------------------------------------------------------------------
 * ConfigureDiffview()
 *
 * Takes a list of configuration options in objc/objv format and applies
 * the settings to the widget.  This is the underlying "configure"
 * operation for the widget.
 * ----------------------------------------------------------------------
 */
static int
ConfigureDiffview(interp, dvPtr, objc, objv, flags)
    Tcl_Interp *interp;       /* interp handling this command */
    Diffview *dvPtr;          /* widget data */
    int objc;                 /* number of configuration option words */
    Tcl_Obj *CONST objv[];    /* configuration option words */
    int flags;                /* flags for Tk_ConfigureWidget */
{
    Tcl_Obj *errorResult = NULL;
    int status;
    Tk_SavedOptions savedOptions;

    status = Tk_SetOptions(interp, (char*)dvPtr, dvPtr->optionTable,
        objc, objv, dvPtr->tkwin, &savedOptions, (int*)NULL);

    if (status != TCL_OK) {
        errorResult = Tcl_GetObjResult(interp);
        Tcl_IncrRefCount(errorResult);
        Tk_RestoreSavedOptions(&savedOptions);

        Tcl_SetObjResult(interp, errorResult);
        Tcl_DecrRefCount(errorResult);
        return TCL_ERROR;
    }

    /*
     * Fix up the widget to react to any new configuration values.
     */
    Tk_SetBackgroundFromBorder(dvPtr->tkwin, dvPtr->normalBorder);

    if (dvPtr->highlightWidth < 0) {
        dvPtr->highlightWidth = 0;
    }
    dvPtr->inset = dvPtr->highlightWidth + dvPtr->borderWidth;

    if (dvPtr->tklastfont != dvPtr->tkfont) {
        /* the font changed, so re-measure everything */
        dvPtr->flags |= FONT_CHANGED;
        dvPtr->tklastfont = dvPtr->tkfont;
    }

    Tk_FreeSavedOptions(&savedOptions);

    DiffviewWorldChanged((ClientData)dvPtr);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewWorldChanged()
 *
 * Called by Tk when the world has changed in some way that causes
 * all GCs and other things to be recomputed.  Reinitializes the
 * widget and gets it ready to draw.
 * ----------------------------------------------------------------------
 */
static void
DiffviewWorldChanged(cdata)
    ClientData cdata;        /* widget data */
{
    Diffview *dvPtr = (Diffview*)cdata;
    GC gc; XGCValues gcValues;
    unsigned long mask;

    /* GC for normal widget text */
    gcValues.foreground = dvPtr->fgColorPtr->pixel;
    gcValues.graphics_exposures = False;
    gcValues.font = Tk_FontId(dvPtr->tkfont);
    mask = GCForeground | GCFont | GCGraphicsExposures;

    gc = Tk_GetGC(dvPtr->tkwin, mask, &gcValues);
    if (dvPtr->normGC != None) {
        Tk_FreeGC(dvPtr->display, dvPtr->normGC);
    }
    dvPtr->normGC = gc;

    /* GC for added diff text */
    gcValues.foreground = dvPtr->addFgColorPtr->pixel;
    gcValues.graphics_exposures = False;
    gcValues.font = Tk_FontId(dvPtr->tkfont);
    mask = GCForeground | GCFont | GCGraphicsExposures;

    gc = Tk_GetGC(dvPtr->tkwin, mask, &gcValues);
    if (dvPtr->addFgGC != None) {
        Tk_FreeGC(dvPtr->display, dvPtr->addFgGC);
    }
    dvPtr->addFgGC = gc;

    /* GC for deleted diff text */
    gcValues.foreground = dvPtr->delFgColorPtr->pixel;
    gcValues.graphics_exposures = False;
    gcValues.font = Tk_FontId(dvPtr->tkfont);
    mask = GCForeground | GCFont | GCGraphicsExposures;

    gc = Tk_GetGC(dvPtr->tkwin, mask, &gcValues);
    if (dvPtr->delFgGC != None) {
        Tk_FreeGC(dvPtr->display, dvPtr->delFgGC);
    }
    dvPtr->delFgGC = gc;

    /* GC for changed diff text */
    gcValues.foreground = dvPtr->chgFgColorPtr->pixel;
    gcValues.graphics_exposures = False;
    gcValues.font = Tk_FontId(dvPtr->tkfont);
    mask = GCForeground | GCFont | GCGraphicsExposures;

    gc = Tk_GetGC(dvPtr->tkwin, mask, &gcValues);
    if (dvPtr->chgFgGC != None) {
        Tk_FreeGC(dvPtr->display, dvPtr->chgFgGC);
    }
    dvPtr->chgFgGC = gc;

    /* this call comes when the font changes -- fix the layout */
    dvPtr->flags |= FONT_CHANGED;

    /* get ready to redraw */
    DiffviewComputeGeometry(dvPtr);
    dvPtr->flags |= UPDATE_V_SCROLLBAR|UPDATE_H_SCROLLBAR;
    EventuallyRedraw(dvPtr);
}

/*
 * ----------------------------------------------------------------------
 * EventuallyRedraw()
 *
 * Arranges for the widget to redraw itself at the next idle point.
 * ----------------------------------------------------------------------
 */
static void
EventuallyRedraw(dvPtr)
    Diffview *dvPtr;          /* widget data */
{
    if ((dvPtr->flags & REDRAW_PENDING) || (dvPtr->flags & WIDGET_DELETED)
            || !Tk_IsMapped(dvPtr->tkwin)) {
        return;  /* no need to redraw */
    }

    dvPtr->flags |= REDRAW_PENDING;
    Tcl_DoWhenIdle(DisplayDiffview, (ClientData)dvPtr);
}

/*
 * ----------------------------------------------------------------------
 * DisplayDiffview()
 *
 * Redraws the widget based on the current view and all data.
 * ----------------------------------------------------------------------
 */
static void
DisplayDiffview(cdata)
    ClientData cdata;        /* widget data */
{
    Diffview *dvPtr = (Diffview*)cdata;
    Tk_Window tkwin = dvPtr->tkwin;

    int i, bnum, bline, x, xw, y, ymid, width;
    char *textPtr; int textLen;
    Pixmap pixmap;
    GC bg, fg;

    /* handling redraw now -- no longer pending */
    dvPtr->flags &= ~REDRAW_PENDING;

    if (dvPtr->flags & WIDGET_DELETED) {
        /* widget is being torn down -- bail out! */
        return;
    }

    /* handle any pending layout changes */
    DiffviewUpdateLayout(dvPtr);

    /*
     * Hang onto the widget data until we're done updating scrollbars.
     * The -xscrollcommand and -yscrollcommand options may take us into
     * code that deletes the widget.
     */
    Tcl_Preserve((ClientData)dvPtr);

    if (dvPtr->flags & UPDATE_V_SCROLLBAR) {
        DiffviewUpdateVScrollbar(dvPtr);
        if ((dvPtr->flags & WIDGET_DELETED) || !Tk_IsMapped(tkwin)) {
            Tcl_Release((ClientData)dvPtr);
            return;
        }
    }
    if (dvPtr->flags & UPDATE_H_SCROLLBAR) {
        DiffviewUpdateHScrollbar(dvPtr);
        if ((dvPtr->flags & WIDGET_DELETED) || !Tk_IsMapped(tkwin)) {
            Tcl_Release((ClientData)dvPtr);
            return;
        }
    }
    dvPtr->flags &= ~(REDRAW_PENDING|UPDATE_V_SCROLLBAR|UPDATE_H_SCROLLBAR);
    Tcl_Release((ClientData)dvPtr);

#ifndef TK_NO_DOUBLE_BUFFERING
    /*
     * Best solution is to draw everything into a temporary pixmap
     * and copy that to the screen in one shot.  That avoids flashing.
     */
    pixmap = Tk_GetPixmap(dvPtr->display, Tk_WindowId(tkwin),
            Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
#else
    pixmap = Tk_WindowId(tkwin);
#endif

    Tk_Fill3DRectangle(tkwin, pixmap, dvPtr->normalBorder, 0, 0,
            Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

    /*
     * Find the top line in the view in each buffer
     */
    y = dvPtr->inset + dvPtr->topLine*dvPtr->lineHeight + dvPtr->lineAscent
          - dvPtr->yOffset;
    x = dvPtr->inset - dvPtr->xOffset;

    if (Tk_Width(tkwin) > dvPtr->maxWidth) {
        width = Tk_Width(tkwin)+10;
    } else {
        width = dvPtr->maxWidth+10;
    }

    for (i=dvPtr->topLine;
         i <= dvPtr->btmLine && i < dvPtr->worldview.numLines;
         i++) {

        /* draw any diff rectangle for this line */
        fg = dvPtr->normGC;
        bg = None;

        switch (dvPtr->worldview.lines[i].style) {
            case 'a': {
                fg = dvPtr->addFgGC;
                bg = Tk_GCForColor(dvPtr->addBgColorPtr, pixmap);
                break;
            }
            case 'd': {
                fg = dvPtr->delFgGC;
                bg = Tk_GCForColor(dvPtr->delBgColorPtr, pixmap);
                break;
            }
            case 'c': {
                fg = dvPtr->chgFgGC;
                bg = Tk_GCForColor(dvPtr->chgBgColorPtr, pixmap);
                break;
            }
        }

        if (bg != None) {
            XFillRectangle(Tk_Display(tkwin), pixmap, bg,
                x, y - dvPtr->lineAscent,
                (unsigned int)width, (unsigned int)dvPtr->lineHeight);
        }

        bnum = dvPtr->worldview.lines[i].bnum;
        bline = dvPtr->worldview.lines[i].bline;

        /* a negative number means "leave the line blank" */
        if (bline >= 0) {
            textPtr = dvPtr->buffer[bnum].lineLimits->startPtr[bline];
            textLen = dvPtr->buffer[bnum].lineLimits->lenPtr[bline];

            /* Draw the actual text of this item */
            Tk_DrawChars(dvPtr->display, pixmap, fg, dvPtr->tkfont,
                textPtr, textLen, x, y);

            /* Draw an overstrike on deleted text */
            if (dvPtr->worldview.lines[i].style == 'd' && dvPtr->overStrDel) {
                xw = Tk_TextWidth(dvPtr->tkfont, textPtr, textLen) + 5;
                ymid = y - dvPtr->lineAscent/2;
                XDrawLine(Tk_Display(tkwin), pixmap, fg, 0, ymid, xw, ymid);
            }
        }

        y += dvPtr->lineHeight;
    }

    /*
     * Draw the border on top so that it covers any characters that
     * have scrolled off screen.
     */
    Tk_Draw3DRectangle(tkwin, pixmap, dvPtr->normalBorder,
            dvPtr->highlightWidth, dvPtr->highlightWidth,
            Tk_Width(tkwin) - 2*dvPtr->highlightWidth,
            Tk_Height(tkwin) - 2*dvPtr->highlightWidth,
            dvPtr->borderWidth, dvPtr->relief);

    if (dvPtr->highlightWidth > 0) {
        XColor *color; GC gc;

        color = (dvPtr->flags & GOT_FOCUS)
            ? dvPtr->highlightColor : dvPtr->highlightBgColor;
        gc = Tk_GCForColor(color, pixmap);
        Tk_DrawFocusHighlight(dvPtr->tkwin, gc, dvPtr->highlightWidth, pixmap);
    }

#ifndef TK_NO_DOUBLE_BUFFERING
    XCopyArea(dvPtr->display, pixmap, Tk_WindowId(tkwin),
            dvPtr->normGC, 0, 0, (unsigned) Tk_Width(tkwin),
            (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(dvPtr->display, pixmap);
#endif
}

/*
 * ----------------------------------------------------------------------
 * DiffviewComputeGeometry()
 *
 * Recomputes the overall dimensions of the widget when the overall
 * size or the borderwidth is changed.  Registers a new size for the
 * widget.
 * ----------------------------------------------------------------------
 */
static void
DiffviewComputeGeometry(dvPtr)
    Diffview *dvPtr;          /* widget data */
{
    int pixelWidth, pixelHeight;

    dvPtr->xScrollUnit = Tk_TextWidth(dvPtr->tkfont, "0", 1);
    if (dvPtr->xScrollUnit == 0) {
        dvPtr->xScrollUnit = 1;
    }

    pixelWidth = dvPtr->width + 2*dvPtr->inset;
    pixelHeight = dvPtr->height + 2*dvPtr->inset;
    Tk_GeometryRequest(dvPtr->tkwin, pixelWidth, pixelHeight);
    Tk_SetInternalBorder(dvPtr->tkwin, dvPtr->inset);
}

/*
 * ----------------------------------------------------------------------
 * DiffviewEventProc()
 *
 * Catches important events on the widget and causes it to redraw
 * itself and react to focus changes.
 * ----------------------------------------------------------------------
 */
static void
DiffviewEventProc(cdata, eventPtr)
    ClientData cdata;        /* widget data */
    XEvent *eventPtr;        /* event information coming from X11 */
{
    Diffview *dvPtr = (Diffview*)cdata;
    
    if (eventPtr->type == Expose) {
        EventuallyRedraw(dvPtr);
    }
    else if (eventPtr->type == DestroyNotify) {
        if (!(dvPtr->flags & WIDGET_DELETED)) {
            dvPtr->flags |= WIDGET_DELETED;
            Tcl_DeleteCommandFromToken(dvPtr->interp, dvPtr->widgetCmd);
            if (dvPtr->flags & REDRAW_PENDING) {
                Tcl_CancelIdleCall(DisplayDiffview, cdata);
            }
            Tcl_EventuallyFree(cdata, DestroyDiffview);
        }
    }
    else if (eventPtr->type == ConfigureNotify) {
        dvPtr->flags |= UPDATE_V_SCROLLBAR|UPDATE_H_SCROLLBAR;
        ChangeDiffviewView(dvPtr, dvPtr->topLine);
        EventuallyRedraw(dvPtr);
    }
    else if (eventPtr->type == FocusIn) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            dvPtr->flags |= GOT_FOCUS;
            EventuallyRedraw(dvPtr);
        }
    }
    else if (eventPtr->type == FocusOut) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            dvPtr->flags &= ~GOT_FOCUS;
            EventuallyRedraw(dvPtr);
        }
    }
}

/*
 * ----------------------------------------------------------------------
 * DiffviewCmdDeletedProc --
 *
 * Invoked whenever the command associated with the widget is deleted.
 * If the widget is not already being destroyed, this starts the process.
 * ----------------------------------------------------------------------
 */
static void
DiffviewCmdDeletedProc(cdata)
    ClientData cdata;        /* widget data */
{
    Diffview *dvPtr = (Diffview*)cdata;

    /*
     * This gets invoked whenever the command is deleted.  If we haven't
     * started destroying the widget yet, then do it now to clean up.
     */
    if (!(dvPtr->flags & WIDGET_DELETED)) {
        Tk_DestroyWindow(dvPtr->tkwin);
    }
}

/*
 * ----------------------------------------------------------------------
 * ChangeDiffviewView()
 *
 * Changes the y-view of the widget for scrolling operations.  Sets the
 * given line to the top of the view.
 * ----------------------------------------------------------------------
 */
static void
ChangeDiffviewView(dvPtr, lnum)
    Diffview *dvPtr;          /* widget data */
    int lnum;                 /* this line should appear at the top */
{
    int maxLines, ySize, yLines;

    DiffviewUpdateLayout(dvPtr);
    ySize = Tk_Height(dvPtr->tkwin) - 2*dvPtr->inset;
    yLines = ySize/dvPtr->lineHeight;
    maxLines = dvPtr->maxHeight/dvPtr->lineHeight;

    if (lnum >= (maxLines - yLines)) {
        lnum = maxLines - yLines;
    }
    if (lnum < 0) {
        lnum = 0;
    }
    if (dvPtr->topLine != lnum) {
        dvPtr->topLine = lnum;
        dvPtr->yOffset = lnum * dvPtr->lineHeight;
        EventuallyRedraw(dvPtr);
        dvPtr->flags |= UPDATE_V_SCROLLBAR;
    }
}

/*
 * ----------------------------------------------------------------------
 * DiffviewScanTo()
 *
 * Implements the "scan dragto" operation on the widget.  Given a
 * starting point from the "scan mark" operation, this adjusts the
 * view to the given location.
 * ----------------------------------------------------------------------
 */
static void
DiffviewScanTo(dvPtr, x, y)
    Diffview *dvPtr;          /* widget data */
    int x;                    /* x-coord for drag-to location */
    int y;                    /* y-coord for drag-to location */
{
    int newTopLine, newOffset, maxLine, maxOffset;

    /*
     * Compute the top line for the display by amplifying the difference
     * between the current "drag" point and the original "start" point.
     */
    maxLine = dvPtr->maxHeight/dvPtr->lineHeight - dvPtr->fullLines;

    newTopLine = dvPtr->scanMarkYStart
            - (10*(y - dvPtr->scanMarkY))/dvPtr->lineHeight;

    if (newTopLine > maxLine) {
        newTopLine = dvPtr->scanMarkYStart = maxLine;
        dvPtr->scanMarkY = y;
    }
    else if (newTopLine < 0) {
        newTopLine = dvPtr->scanMarkYStart = 0;
        dvPtr->scanMarkY = y;
    }
    ChangeDiffviewView(dvPtr, newTopLine);

    /*
     * Compute the left edge for the display by amplifying the difference
     * between the current "drag" point and the original "start" point.
     */
    maxOffset = dvPtr->maxWidth - (Tk_Width(dvPtr->tkwin) - 2*dvPtr->inset);
    newOffset = dvPtr->scanMarkXStart - (10*(x - dvPtr->scanMarkX));
    if (newOffset > maxOffset) {
        newOffset = dvPtr->scanMarkXStart = maxOffset;
        dvPtr->scanMarkX = x;
    } else if (newOffset < 0) {
        newOffset = dvPtr->scanMarkXStart = 0;
        dvPtr->scanMarkX = x;
    }
    newOffset -= newOffset % dvPtr->xScrollUnit;

    if (newOffset != dvPtr->xOffset) {
        dvPtr->xOffset = newOffset;
        dvPtr->flags |= UPDATE_H_SCROLLBAR;
        EventuallyRedraw(dvPtr);
    }
}

/*
 * ----------------------------------------------------------------------
 * DiffviewUpdateLayout()
 *
 * Called whenever the widget is about to access layout information
 * to make sure that the latest info is in place and up-to-date.
 * Computes the line start/end for each line in each buffer, then
 * computes the diffs and figures out how many lines to show.
 * ----------------------------------------------------------------------
 */
static void
DiffviewUpdateLayout(dvPtr)
    Diffview *dvPtr;          /* widget data */
{
    int changes = 0;   /* no layout changes yet */
    DiffviewBuffer* bufferPtr;
    DiffviewDiffOp *currOp;
    DiffviewLayoutLine line;
    int i, bnum, bline, pixelWidth, maxWidth, ySize;
    int numLines1, numLines2, lnum1, lnum2, dnum, lastdnum, pastDiff;
    char *textPtr, *textPtr2; int textLen;
    Tk_FontMetrics fm;

    /* if the font changed, then re-measure everything */
    changes = ((dvPtr->flags & FONT_CHANGED) != 0);

    /* fill in line limits for any new data */
    for (bnum=0; bnum < 2; bnum++) {
        bufferPtr = &dvPtr->buffer[bnum];

        if (bufferPtr->textObj != NULL && bufferPtr->lineLimits == NULL) {
            textPtr = Tcl_GetStringFromObj(bufferPtr->textObj, &textLen);
            bufferPtr->lineLimits = DiffviewLinesCreate(textPtr, textLen);
            changes = 1;
        }
    }

    if (changes) {
        /* recompute the diffs between the buffers */
        if (dvPtr->diffsPtr) {
            DiffviewDiffsFree(dvPtr->diffsPtr);
            dvPtr->diffsPtr = NULL;
        }

        if (dvPtr->buffer[0].textObj && dvPtr->buffer[1].textObj) {
            textPtr = Tcl_GetStringFromObj(dvPtr->buffer[0].textObj, &textLen);
            textPtr2 = Tcl_GetStringFromObj(dvPtr->buffer[1].textObj, &textLen);

            dvPtr->diffsPtr = DiffviewDiffsCreate(
                textPtr, dvPtr->buffer[0].lineLimits,
                textPtr2, dvPtr->buffer[1].lineLimits);
        }

        /*
         * Compute the layout of all lines according to the current
         * view mode.  Each line in the world view is stored in the
         * array dvPtr->worldview.lines.  Each line has a style (color
         * for normal, add, delete, etc.) and an indication of which
         * buffer it comes from.
         */
        DiffviewLayoutClear(&dvPtr->worldview);

        /*
         * March through the lines and figure out the source and style
         * of each line based on the diffs:
         *   'n' = normal (common) line
         *   'a' = draw with the "added" style
         *   'd' = draw with the "deleted" style
         *   'c' = draw with the "changed" style
         */
        numLines1 = (dvPtr->buffer[0].lineLimits)
                       ? dvPtr->buffer[0].lineLimits->numLines : 0;
        numLines2 = (dvPtr->buffer[1].lineLimits)
                       ? dvPtr->buffer[1].lineLimits->numLines : 0;

        dnum = 0;  /* current difference in diffsPtr */
        lnum1 = lnum2 = 0;

        while (lnum1 < numLines1 || lnum2 < numLines2) {
            /* assume it's a normal-looking line (buffers are the same) */
            line.style = 'n';
            line.bnum = 0;
            line.bline = lnum1;
            line.diffNum = -1;

            /* is there a diff that contains this line? */
            if (dvPtr->diffsPtr && dnum < dvPtr->diffsPtr->numDiffs) {
              currOp = &dvPtr->diffsPtr->ops[dnum];

              if ( (lnum1 >= currOp->fromIndex1
                     && lnum1 <= currOp->toIndex1)
                || (lnum2 >= currOp->fromIndex2
                     && lnum2 <= currOp->toIndex2) ) {

                line.diffNum = dnum;  /* line is part of this diff */

                switch (currOp->op) {
                    case 'c': {
                        if (dvPtr->layout == LAYOUT_INLINE) {
                            if (dvPtr->diffdir == DIFF_1TO2) {
                                /* show the buffer #1 lines first, then #2 */
                                if (lnum1 <= currOp->toIndex1) {
                                    line.style = 'd';
                                    line.bnum = 0;
                                    line.bline = lnum1++;
                                } else {
                                    line.style = 'a';
                                    line.bnum = 1;
                                    line.bline = lnum2++;
                                }
                            } else {
                                /* reverse -- show #2 lines first, then #1 */
                                if (lnum2 <= currOp->toIndex2) {
                                    line.style = 'd';
                                    line.bnum = 1;
                                    line.bline = lnum2++;
                                } else {
                                    line.style = 'a';
                                    line.bnum = 0;
                                    line.bline = lnum1++;
                                }
                            }
                        } else {
                            if (dvPtr->diffdir == DIFF_1TO2) {
                                /* show final lines in buf #2 as "changed" */
                                line.style = 'c';
                                line.bnum = 1;
                                line.bline = lnum2++;
                                lnum1 = currOp->toIndex1+1;
                            } else {
                                /* show final lines in buf #1 as "changed" */
                                line.style = 'c';
                                line.bnum = 0;
                                line.bline = lnum1++;
                                lnum2 = currOp->toIndex2+1;
                            }
                        }
                        break;
                    }
                    case 'a': {
                        if (dvPtr->diffdir == DIFF_1TO2) {
                            /* show lines in buffer #2 as 'added' */
                            line.style = 'a';
                            line.bnum = 1;
                            line.bline = lnum2++;
                            lnum1 = currOp->toIndex1;
                        } else {
                            /* reverse diff -- like we're deleting lines */
                            if (dvPtr->layout == LAYOUT_INLINE) {
                                line.style = 'd';
                                line.bnum = 1;
                                line.bline = lnum2++;
                                lnum1 = currOp->toIndex1;
                            } else {
                                /* show lines from buffer #2 as empty */
                                line.style = 'd';
                                line.bnum = 0;
                                line.bline = -1;
                                lnum2++;
                                lnum1 = currOp->toIndex1;
                            }
                        }
                        break;
                    }
                    case 'd': {
                        if (dvPtr->diffdir == DIFF_1TO2) {
                            if (dvPtr->layout == LAYOUT_INLINE) {
                                line.style = 'd';
                                line.bnum = 0;
                                line.bline = lnum1++;
                                lnum2 = currOp->toIndex2;
                            } else {
                                /* show lines from buffer #2 as empty */
                                line.style = 'd';
                                line.bnum = 0;
                                line.bline = -1;
                                lnum1++;
                                lnum2 = currOp->toIndex2;
                            }
                        } else {
                            /* reverse diff -- like we're adding lines */
                            line.style = 'a';
                            line.bnum = 0;
                            line.bline = lnum1++;
                            lnum2 = currOp->toIndex2;
                        }
                        break;
                    }
                    default: {
                        Tcl_Panic("bad diff type '%c' found in layout",
                            currOp->op);
                        break;
                    }
                }
              } else {
                /* normal line -- keep moving forward */
                lnum1++; lnum2++;
              }

              /* have we reached the end of the diff? then move on */
              pastDiff = 0;
              switch (currOp->op) {
                  case 'c':
                      pastDiff = (lnum1 > currOp->toIndex1
                               && lnum2 > currOp->toIndex2);
                      break;
                  case 'a':
                      pastDiff = (lnum2 > currOp->toIndex2);
                      break;
                  case 'd':
                      pastDiff = (lnum1 > currOp->toIndex1);
                      break;
              }
              if (pastDiff) {
                  dnum++;
              }
            } else {
                /* no more diffs -- keep moving forward */
                lnum1++; lnum2++;
            }

            /* add this new line to the layout */
            DiffviewLayoutAdd(&dvPtr->worldview, &line);
        }

        /*
         * Figure out where the diffs are located, and put that info
         * back into the diffs.  This makes it easy later to refer to
         * diff "#3" and understand what lines we're talking about.
         */
        lastdnum = -1;
        for (i=0; i < dvPtr->worldview.numLines; i++) {
            dnum = dvPtr->worldview.lines[i].diffNum;
            if (dnum != lastdnum) {
                if (lastdnum < 0) {
                    /* leading edge -- catch the "from" line */
                    lnum1 = i;
                } else {
                    /* trailing edge -- save diff info */
                    dvPtr->diffsPtr->ops[lastdnum].fromWorld = lnum1;
                    dvPtr->diffsPtr->ops[lastdnum].toWorld = i-1;
                }
            }
            lastdnum = dnum;
        }
        if (lastdnum >= 0) {
            dvPtr->diffsPtr->ops[lastdnum].fromWorld = lnum1;
            dvPtr->diffsPtr->ops[lastdnum].toWorld = i-1;
        }

        /* compute overall text width for all lines */
        maxWidth = 0;
        for (i=0; i < dvPtr->worldview.numLines; i++) {
            bline = dvPtr->worldview.lines[i].bline;
            if (bline >= 0) {
                bnum = dvPtr->worldview.lines[i].bnum;
                bufferPtr = &dvPtr->buffer[bnum];

                if (bufferPtr->lineLimits) {
                    textPtr = bufferPtr->lineLimits->startPtr[bline];
                    textLen = bufferPtr->lineLimits->lenPtr[bline];
                    pixelWidth = Tk_TextWidth(dvPtr->tkfont, textPtr, textLen);

                    if (pixelWidth > maxWidth) {
                        maxWidth = pixelWidth;
                    }
                }
            }
        }
        dvPtr->maxWidth = maxWidth;

        /* compute overall height of the widget */
        Tk_GetFontMetrics(dvPtr->tkfont, &fm);
        dvPtr->lineHeight = fm.linespace;
        dvPtr->lineAscent = fm.ascent;
        dvPtr->maxHeight = dvPtr->worldview.numLines * fm.linespace;

        /* figure out the limits of the current view */
        ySize = Tk_Height(dvPtr->tkwin) - 2*dvPtr->inset;
        dvPtr->topLine = dvPtr->yOffset/fm.linespace;
        dvPtr->btmLine = (dvPtr->yOffset + ySize)/fm.linespace + 1;
        dvPtr->fullLines = ySize/fm.linespace;

        dvPtr->flags &= ~FONT_CHANGED;
    }
}

/*
 * ----------------------------------------------------------------------
 * DiffviewUpdateVScrollbar()
 *
 * Used to invoke the -yscrollcommand whenever the y-view has changed.
 * This updates the scrollbar so that it shows the proper bubble.
 * If there is no -yscrollcommand option, this does nothing.
 * ----------------------------------------------------------------------
 */
static void
DiffviewUpdateVScrollbar(dvPtr)
    Diffview *dvPtr;          /* widget data */
{
    char string[TCL_DOUBLE_SPACE*2 + 2];
    double first, last;
    int result, ySize;

    /* make sure the layout info is up to date */
    DiffviewUpdateLayout(dvPtr);

    /* update the limits of the current view */
    ySize = Tk_Height(dvPtr->tkwin) - 2*dvPtr->inset;
    if (dvPtr->lineHeight > 0) {
        dvPtr->topLine = dvPtr->yOffset/dvPtr->lineHeight;
        dvPtr->btmLine = (dvPtr->yOffset + ySize)/dvPtr->lineHeight + 1;
        dvPtr->fullLines = ySize/dvPtr->lineHeight;
    }

    /* if there's no scroll command, then there's nothing left to do */
    if (dvPtr->yScrollCmd == NULL) {
        return;
    }

    if (dvPtr->maxHeight == 0) {
        first = 0.0;
        last = 1.0;
    } else {
        first = dvPtr->yOffset/((double)dvPtr->maxHeight);
        last = (dvPtr->yOffset + ySize)/((double)dvPtr->maxHeight);
        if (last > 1.0) {
            last = 1.0;
        }
    }
    sprintf(string, " %g %g", first, last);

    /* preserve/release the interp, in case it gets destroyed during the call */
    Tcl_Preserve((ClientData)dvPtr->interp);

    result = Tcl_VarEval(dvPtr->interp, dvPtr->yScrollCmd, string,
            (char*)NULL);
    if (result != TCL_OK) {
        Tcl_AddErrorInfo(dvPtr->interp,
                "\n    (vertical scrolling command executed by diffview)");
        Tcl_BackgroundError(dvPtr->interp);
    }

    Tcl_Release((ClientData)dvPtr->interp);
}

/*
 * ----------------------------------------------------------------------
 * DiffviewUpdateHScrollbar()
 *
 * Used to invoke the -xscrollcommand whenever the x-view has changed.
 * This updates the scrollbar so that it shows the proper bubble.
 * If there is no -xscrollcommand option, this does nothing.
 * ----------------------------------------------------------------------
 */
static void
DiffviewUpdateHScrollbar(dvPtr)
    Diffview *dvPtr;          /* widget data */
{
    char string[TCL_DOUBLE_SPACE*2 + 3];
    int result, viewWidth;
    double first, last;

    /* make sure the layout info is up to date */
    DiffviewUpdateLayout(dvPtr);

    /* if there's no scroll command, then there's nothing left to do */
    if (dvPtr->xScrollCmd == NULL) {
        return;
    }

    viewWidth = Tk_Width(dvPtr->tkwin) - 2*dvPtr->inset;
    if (dvPtr->maxWidth == 0) {
        first = 0;
        last = 1.0;
    } else {
        first = dvPtr->xOffset/((double)dvPtr->maxWidth);
        last = (dvPtr->xOffset + viewWidth)/((double) dvPtr->maxWidth);
        if (last > 1.0) {
            last = 1.0;
        }
    }
    sprintf(string, " %g %g", first, last);

    /* preserve/release the interp, in case it gets destroyed during the call */
    Tcl_Preserve((ClientData)dvPtr->interp);

    result = Tcl_VarEval(dvPtr->interp, dvPtr->xScrollCmd, string,
            (char*)NULL);
    if (result != TCL_OK) {
        Tcl_AddErrorInfo(dvPtr->interp,
                "\n    (horizontal scrolling command executed by diffview)");
        Tcl_BackgroundError(dvPtr->interp);
    }

    Tcl_Release((ClientData)dvPtr->interp);
}

/*
 * ----------------------------------------------------------------------
 * DiffviewLinesCreate()
 *
 * Used to breakup a Tcl string object into the indices for the start
 * and end of each line.  This is the first step in the computation
 * of diffs and the world view of the widget.
 *
 * Returns a pointer to a description of the line geometry, which should
 * be freed by calling DiffviewLinesFree() when it is no longer needed.
 * ----------------------------------------------------------------------
 */
static DiffviewLines*
DiffviewLinesCreate(textPtr, textLen)
    char *textPtr;                  /* text string being broken up */
    int textLen;                    /* length of the string */
{
    int numLines = 0;
    DiffviewLines *lineLimitsPtr;
    char **newStarts;
    int *newLens;
    unsigned int len;

    lineLimitsPtr = (DiffviewLines*)ckalloc(sizeof(DiffviewLines));
    lineLimitsPtr->maxLines = 0;
    lineLimitsPtr->startPtr = NULL;
    lineLimitsPtr->lenPtr = NULL;

    while (textLen > 0) {
        /*
         * If we're out of space, double the size and copy existing
         * info over.
         */
        if (numLines >= lineLimitsPtr->maxLines) {
            if (lineLimitsPtr->maxLines == 0) {
                lineLimitsPtr->maxLines = 100;
            } else {
                lineLimitsPtr->maxLines *= 2;
            }

            /* resize the start point array */
            newStarts = (char**)ckalloc(lineLimitsPtr->maxLines*sizeof(char*));
            if (lineLimitsPtr->startPtr) {
                memcpy((VOID*)newStarts, (VOID*)lineLimitsPtr->startPtr,
                    numLines*sizeof(char*));
                ckfree((char*)lineLimitsPtr->startPtr);
            }
            lineLimitsPtr->startPtr = newStarts;

            /* resize the end point array */
            newLens = (int*)ckalloc(lineLimitsPtr->maxLines*sizeof(int));
            if (lineLimitsPtr->lenPtr) {
                memcpy((VOID*)newLens, (VOID*)lineLimitsPtr->lenPtr,
                    numLines*sizeof(int));
                ckfree((char*)lineLimitsPtr->lenPtr);
            }
            lineLimitsPtr->lenPtr = newLens;
        }

        /* mark the start of this line, then search for the end */
        lineLimitsPtr->startPtr[numLines] = textPtr;
        lineLimitsPtr->lenPtr[numLines] = 0;

        while (*textPtr != '\n' && textLen > 0) {
            textPtr++;  textLen--;
        }
        if (textPtr > lineLimitsPtr->startPtr[numLines]) {
            len = textPtr - lineLimitsPtr->startPtr[numLines];
            lineLimitsPtr->lenPtr[numLines] = len;
        }
        numLines++;

        /* skip over the newline and start with next line */
        if (*textPtr == '\n') {
            textPtr++;  textLen--;
        }
    }
    lineLimitsPtr->numLines = numLines;

    return lineLimitsPtr;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewLinesFree()
 *
 * Used to free up the data structure created by DiffviewLinesCreate().
 * ----------------------------------------------------------------------
 */
static void
DiffviewLinesFree(lineLimitsPtr)
    DiffviewLines *lineLimitsPtr;   /* data structure being freed */
{
    if (lineLimitsPtr) {
        if (lineLimitsPtr->startPtr) {
            ckfree((char*)lineLimitsPtr->startPtr);
        }
        if (lineLimitsPtr->lenPtr) {
            ckfree((char*)lineLimitsPtr->lenPtr);
        }
        ckfree((char*)lineLimitsPtr);
    }
}

/*
 * ----------------------------------------------------------------------
 * DiffviewDiffsCreate()
 *
 * Considers two strings textPtr1 and textPtr2 (divided into segments
 * according to limsPtr1 and limsPtr2), and computes the longest common
 * subsequences between the two strings.  This is the first step in
 * computing the differences between the two strings.
 *
 * Returns a data structure that contains a series of "diff" operations.
 * This should be freed when it is no longer needed by calling
 * DiffviewDiffsFree().
 *
 *   REFERENCE:
 *   J. W. Hunt and M. D. McIlroy, "An algorithm for differential
 *   file comparison," Comp. Sci. Tech. Rep. #41, Bell Telephone
 *   Laboratories (1976). Available on the Web at the second
 *   author's personal site: http://www.cs.dartmouth.edu/~doug/
 *
 * ----------------------------------------------------------------------
 */
static DiffviewDiffs*
DiffviewDiffsCreate(textPtr1, limsPtr1, textPtr2, limsPtr2)
    char *textPtr1;           /* text from buffer #1 */
    DiffviewLines *limsPtr1;  /* limits of individual strings in textPtr1 */
    char *textPtr2;           /* text from buffer #2 */
    DiffviewLines *limsPtr2;  /* limits of individual strings in textPtr2 */
{
    DiffviewDiffs *diffPtr = NULL;

    Tcl_HashTable eqv;
    Tcl_HashSearch iter;
    Tcl_HashEntry *entryPtr;
    Tcl_DString buffer;
    DiffviewSubseq *K, *newK, newCandidate; 
    int Kmax; int Klen;
    Tcl_Obj *listPtr, **objv;
    int i, j, candidateIdx, subseqLen, longestMatch;
    int max, min, mid, midval, sLen, del;
    int len, created, o, objc, index1, *lcsIndex1, index2, *lcsIndex2;
    char *key;

    newCandidate.index1 = -1;		/* Suppress compiler warning. */
    newCandidate.index2 = -1;
    newCandidate.next = -1;

    /*
     * Build a set of equivalence classes.  Scan through all of
     * buffer #2 and map each string to a list of indices for the
     * lines that have that string.
     */
    Tcl_DStringInit(&buffer);
    Tcl_InitHashTable(&eqv, TCL_STRING_KEYS);
    for (i=0; i < limsPtr2->numLines; i++) {
        len = limsPtr2->lenPtr[i];
        Tcl_DStringSetLength(&buffer, len);
        key = Tcl_DStringValue(&buffer);
        memcpy((VOID*)key, (VOID*)limsPtr2->startPtr[i], len);

        entryPtr = Tcl_CreateHashEntry(&eqv, key, &created);
        if (created) {
            listPtr = Tcl_NewListObj(0, (Tcl_Obj**)NULL);
            Tcl_IncrRefCount(listPtr);
            Tcl_SetHashValue(entryPtr, (ClientData)listPtr);
        }

        listPtr = (Tcl_Obj*)Tcl_GetHashValue(entryPtr);
        Tcl_ListObjAppendElement((Tcl_Interp*)NULL, listPtr, Tcl_NewIntObj(i));
    }

    /*
     * Build a list K that holds the descriptions of the common
     * subsequences.  At first, there is one common subsequence of
     * length 0 with a fence that includes line -1 of both files.
     */
    Kmax = 10;
    K = (DiffviewSubseq*)ckalloc(Kmax*sizeof(DiffviewSubseq));
    K[0].index1 = -1;
    K[0].index2 = -1;
    K[0].next = -1;
    K[1].index1 = limsPtr1->numLines;
    K[1].index2 = limsPtr2->numLines;
    K[1].next = -1;
    Klen = 2;
    longestMatch = 0;

    /*
     * Step through the first buffer line by line.
     */
    for (i=0; i < limsPtr1->numLines; i++) {
        len = limsPtr1->lenPtr[i];
        Tcl_DStringSetLength(&buffer, len);
        key = Tcl_DStringValue(&buffer);
        memcpy((VOID*)key, (VOID*)limsPtr1->startPtr[i], len);

        /* look at each possible line j in second buffer */
        entryPtr = Tcl_FindHashEntry(&eqv, key);
        if (entryPtr) {
            subseqLen = 0;
            candidateIdx = 0;

            listPtr = (Tcl_Obj*)Tcl_GetHashValue(entryPtr);
            Tcl_ListObjGetElements((Tcl_Interp*)NULL, listPtr, &objc, &objv);
            for (o=0; o < objc; o++) {
                Tcl_GetIntFromObj((Tcl_Interp*)NULL, objv[o], &j);

                /*
                 * Binary search to find a candidate common subsequence.
                 * This match may get appended to that.
                 */
                max  = longestMatch;
                min  = subseqLen;
                mid  = subseqLen;
                sLen = longestMatch + 1;
                while (max >= min) {
                    mid = (max+min)/2;
                    midval = K[mid].index2;
                    if (j == midval) {
                        break;
                    } else if (j < midval) {
                        max = mid-1;
                    } else {
                        sLen = mid;
                        min = mid+1;
                    }
                }

                /* no good candidate? then go to the next match point */
                if (j == K[mid].index2 || sLen > longestMatch) {
                    continue;
                }

                /*
                 * sLen = sequence length of the longest sequence that
                 *        this match point can be appended to
                 *
                 * Make a new candidate match and store the old on in K.
                 * Set subseqLen to the length of the new candidate match.
                 */
                if (subseqLen >= 0) {
                    if (candidateIdx >= 0) {
                        K[subseqLen].index1 = K[candidateIdx].index1;
                        K[subseqLen].index2 = K[candidateIdx].index2;
                        K[subseqLen].next   = K[candidateIdx].next;
                    } else {
                        K[subseqLen].index1 = newCandidate.index1;
                        K[subseqLen].index2 = newCandidate.index2;
                        K[subseqLen].next   = newCandidate.next;
                    }
                }
                newCandidate.index1 = i;
                newCandidate.index2 = j;
                newCandidate.next = sLen;
                candidateIdx = -1;  /* use newCandidate info */
                subseqLen = sLen + 1;

                /*
                 * If we've extended the length of the longest match,
                 * we don't need to keep checking candidate lines.
                 * We're done.  Move the fence.
                 */
                if (sLen >= longestMatch) {
                    if (Klen >= Kmax) {
                        Kmax *= 2;
                        newK = (DiffviewSubseq*)ckalloc(Kmax*sizeof(DiffviewSubseq));
                        memcpy((VOID*)newK, (VOID*)K, Klen*sizeof(DiffviewSubseq));
                        ckfree((char*)K);
                        K = newK;
                    }
                    K[Klen].index1 = K[Klen-1].index1;
                    K[Klen].index2 = K[Klen-1].index2;
                    K[Klen].next   = K[Klen-1].next;
                    Klen++;

                    longestMatch++;
                    break;
                }
            }

            /* put the last candidate into the array */
            if (candidateIdx >= 0) {
                K[subseqLen].index1 = K[candidateIdx].index1;
                K[subseqLen].index2 = K[candidateIdx].index2;
                K[subseqLen].next   = K[candidateIdx].next;
            } else {
                K[subseqLen].index1 = newCandidate.index1;
                K[subseqLen].index2 = newCandidate.index2;
                K[subseqLen].next   = newCandidate.next;
            }
        }
    }

    /*
     * Translate the resulting info into a list of common subseq indices.
     */
    lcsIndex1 = (int*)ckalloc(longestMatch*sizeof(int));
    memset((void*)lcsIndex1, 0, (longestMatch*sizeof(int)));
    lcsIndex2 = (int*)ckalloc(longestMatch*sizeof(int));
    memset((void*)lcsIndex2, 0, (longestMatch*sizeof(int)));
    len = longestMatch;

    candidateIdx = longestMatch;
    while (K[candidateIdx].index1 >= 0) {
        if (longestMatch-- < 0) {
            Tcl_Panic("internal mismatch in diff algorithm");
        }
        lcsIndex1[longestMatch] = K[candidateIdx].index1;
        lcsIndex2[longestMatch] = K[candidateIdx].index2;
        candidateIdx = K[candidateIdx].next;
    }

    /*
     * Now, march through all lines in both buffers and convert the
     * lcs indices into a sequence of diff-style operations.
     */
    diffPtr = (DiffviewDiffs*)ckalloc(sizeof(DiffviewDiffs));
    diffPtr->maxDiffs = 0;
    diffPtr->numDiffs = 0;
    diffPtr->ops = NULL;

    i = j = 0;
    for (o=0; o < len; o++) {
        index1 = lcsIndex1[o];
        index2 = lcsIndex2[o];

        if (index1-i == index2-j && index1-i > 0) {
            del = index1-i;
            DiffviewDiffsAppend(diffPtr, 'c', i, i+del-1, j, j+del-1);
            i += del; j += del;
        } else {
            del = index1-i;
            if (del > 0) {
                DiffviewDiffsAppend(diffPtr, 'd', i, i+del-1, j, j);
                i += del;
            }

            del = index2-j;
            if (del > 0) {
                DiffviewDiffsAppend(diffPtr, 'a', i, i, j, j+del-1);
                j += del;
            }
        }
        i++; j++;
    }

    /* lines left over? mark as many "changed" as possible */
    if (limsPtr1->numLines-i > 0 && limsPtr2->numLines-j > 0) {
        del = (limsPtr1->numLines-i < limsPtr2->numLines-j)
                ? limsPtr1->numLines-i : limsPtr2->numLines-j;
        DiffviewDiffsAppend(diffPtr, 'c', i, i+del-1, j, j+del-1);
        i += del;
        j += del;
    }

    /* still have lines left over? then mark them added or deleted */
    if (i < limsPtr1->numLines) {
        del = limsPtr1->numLines - i;
        DiffviewDiffsAppend(diffPtr, 'd', i, i+del-1, j, j);
        i += del;
    }
    if (j < limsPtr2->numLines) {
        del = limsPtr2->numLines - j;
        DiffviewDiffsAppend(diffPtr, 'a', i, i, j, j+del-1);
        j += del;
    }

    /*
     * Clean up all memory used during the diff.
     */
    ckfree((char*)K);
    ckfree((char*)lcsIndex1);
    ckfree((char*)lcsIndex2);

    entryPtr = Tcl_FirstHashEntry(&eqv, &iter);
    while (entryPtr) {
        listPtr = (Tcl_Obj*)Tcl_GetHashValue(entryPtr);
        Tcl_DecrRefCount(listPtr);
        entryPtr = Tcl_NextHashEntry(&iter);
    }
    Tcl_DeleteHashTable(&eqv);

    return diffPtr;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewDiffsAppend()
 *
 * Appends a "diff" operation onto a list of diffs.  The list is extended
 * automatically, if need be, to store the new element.
 * ----------------------------------------------------------------------
 */
static void
DiffviewDiffsAppend(diffsPtr, op, fromIndex1, toIndex1, fromIndex2, toIndex2)
    DiffviewDiffs *diffsPtr;  /* diffs being updated */
    int op;                   /* diff operation: a/d/c */
    int fromIndex1;           /* starts at this line in buffer 1 */
    int toIndex1;             /* ends at this line in buffer 1 */
    int fromIndex2;           /* starts at this line in buffer 2 */
    int toIndex2;             /* ends at this line in buffer 2 */
{
    DiffviewDiffOp *newOpArray;
    int last;

    if (diffsPtr->numDiffs >= diffsPtr->maxDiffs) {
        if (diffsPtr->maxDiffs == 0) {
            diffsPtr->maxDiffs = 10;
        } else {
            diffsPtr->maxDiffs *= 2;
        }
        newOpArray = (DiffviewDiffOp*)ckalloc(
            diffsPtr->maxDiffs*sizeof(DiffviewDiffOp));

        if (diffsPtr->ops) {
            memcpy((VOID*)newOpArray, (VOID*)diffsPtr->ops,
                diffsPtr->numDiffs*sizeof(DiffviewDiffOp));
            ckfree((char*)diffsPtr->ops);
        }
        diffsPtr->ops = newOpArray;
    }

    last = diffsPtr->numDiffs;
    diffsPtr->ops[last].op         = op;
    diffsPtr->ops[last].fromIndex1 = fromIndex1;
    diffsPtr->ops[last].toIndex1   = toIndex1;
    diffsPtr->ops[last].fromIndex2 = fromIndex2;
    diffsPtr->ops[last].toIndex2   = toIndex2;
    diffsPtr->numDiffs++;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewDiffsFree()
 *
 * Frees up the storage previously created by calling
 * DiffviewDiffsAppend().
 * ----------------------------------------------------------------------
 */
static void
DiffviewDiffsFree(diffsPtr)
    DiffviewDiffs *diffsPtr;  /* diffs being freed */
{
    if (diffsPtr->ops) {
        ckfree((char*)diffsPtr->ops);
    }
    ckfree((char*)diffsPtr);
}

/*
 * ----------------------------------------------------------------------
 * DiffviewLayoutAdd()
 *
 * Clears out the layout storage in preparation for building another
 * layout.  If the previous number of lines was much less than the
 * maximum allocated, then this routine reallocates a smaller storage
 * space.  This provides a way to give back a large chunk of memory
 * if the contents of the widget is nulled out, for example.
 * ----------------------------------------------------------------------
 */
static void
DiffviewLayoutAdd(layoutPtr, linePtr)
    DiffviewLayout *layoutPtr;    /* line layout being updated */
    DiffviewLayoutLine *linePtr;  /* line being added to layout */
{
    DiffviewLayoutLine *newLineArray;

    /* expand the array as needed */
    if (layoutPtr->numLines >= layoutPtr->maxLines) {
        if (layoutPtr->maxLines == 0) {
            layoutPtr->maxLines = 100;
        } else {
            layoutPtr->maxLines *= 2;
        }
        newLineArray = (DiffviewLayoutLine*)ckalloc(
            (unsigned)(layoutPtr->maxLines*sizeof(DiffviewLayoutLine)));

        if (layoutPtr->lines) {
            memcpy((VOID*)newLineArray, (VOID*)layoutPtr->lines,
                layoutPtr->numLines*sizeof(DiffviewLayoutLine));
            ckfree((char*)layoutPtr->lines);
        }
        layoutPtr->lines = newLineArray;
    }

    /* copy the latest line in and bump the count */
    memcpy((VOID*)&layoutPtr->lines[layoutPtr->numLines],
        (VOID*)linePtr, sizeof(DiffviewLayoutLine));

    layoutPtr->numLines++;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewLayoutClear()
 *
 * Clears out the layout storage in preparation for building another
 * layout.  If the previous number of lines was much less than the
 * maximum allocated, then this routine reallocates a smaller storage
 * space.  This provides a way to give back a large chunk of memory
 * if the contents of the widget is nulled out, for example.
 * ----------------------------------------------------------------------
 */
static void
DiffviewLayoutClear(layoutPtr)
    DiffviewLayout *layoutPtr;  /* line layout being freed */
{
    if (layoutPtr->numLines > 0
          && layoutPtr->numLines < layoutPtr->maxLines/3
          && layoutPtr->maxLines > 100) {
        ckfree((char*)layoutPtr->lines);
        layoutPtr->lines = (DiffviewLayoutLine*)ckalloc(
            (unsigned)(layoutPtr->numLines * sizeof(DiffviewLayoutLine)));
        layoutPtr->maxLines = layoutPtr->numLines;
    }
    layoutPtr->numLines = 0;
}

/*
 * ----------------------------------------------------------------------
 * DiffviewLayoutFree()
 *
 * Frees the storage associated with the layout of all lines within
 * the widget.
 * ----------------------------------------------------------------------
 */
static void
DiffviewLayoutFree(layoutPtr)
    DiffviewLayout *layoutPtr;  /* line layout being freed */
{
    if (layoutPtr->lines) {
        ckfree((char*)layoutPtr->lines);
        layoutPtr->lines = NULL;
    }
}
