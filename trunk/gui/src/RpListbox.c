/* 
 * ----------------------------------------------------------------------
 *  RpListbox - listbox with a few extra features
 *
 *  Upgrade of the usual Tk listbox, to handle special icons and indents
 *  for each item.  This makes it more useful as a file selection box.
 *
 *  Rappture::listbox .lb
 *  .lb insert end "text" -image icon -indent num -data xyz
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 *  Based on the original Tk listbox widget, which is licensed as
 *  follows...
 *
 * tkListbox.c --
 *
 *	This module implements listbox widgets for the Tk
 *	toolkit.  A listbox displays a collection of strings,
 *	one per line, and provides scrolling and selection.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: tkListbox.c,v 1.29.2.5 2007/04/29 02:24:02 das Exp $
 */
#include "tkPort.h"
#include "default.h"
#include "tkInt.h"

#ifdef WIN32
#include "tkWinInt.h"
#endif

typedef struct {
    Tk_OptionTable listboxOptionTable;	/* Table defining configuration options
					 * available for the listbox */
    Tk_OptionTable itemAttrOptionTable;	/* Table definining configuration
					 * options available for listbox
					 * items */
} ListboxOptionTables;

/*
 * A data structure of the following type is kept for each listbox
 * widget managed by this file:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the listbox.  NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up.*/
    Display *display;		/* Display containing widget.  Used, among
				 * other things, so that resources can be
				 * freed even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with listbox. */
    Tcl_Command widgetCmd;	/* Token for listbox's widget command. */
    Tk_OptionTable optionTable;	/* Table that defines configuration options
				 * available for this widget. */
    Tk_OptionTable itemAttrOptionTable;	/* Table that defines configuration
					 * options available for listbox
					 * items */
    Tcl_Obj *listObj;           /* Pointer to the list object being used */
    int nElements;              /* Holds the current count of elements */
    Tcl_HashTable *selection;   /* Tracks selection */
    Tcl_HashTable *itemAttrTable;	/* Tracks item attributes */

    /*
     * Information used when displaying widget:
     */

    Tk_3DBorder normalBorder;	/* Used for drawing border around whole
				 * window, plus used for background. */
    int borderWidth;		/* Width of 3-D border around window. */
    int relief;			/* 3-D effect: TK_RELIEF_RAISED, etc. */
    int highlightWidth;		/* Width in pixels of highlight to draw
				 * around widget when it has the focus.
				 * <= 0 means don't draw a highlight. */
    XColor *highlightBgColorPtr;
				/* Color for drawing traversal highlight
				 * area when highlight is off. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    int inset;			/* Total width of all borders, including
				 * traversal highlight and 3-D border.
				 * Indicates how much interior stuff must
				 * be offset from outside edges to leave
				 * room for borders. */
    Tk_Font tkfont;		/* Information about text font, or NULL. */
    XColor *fgColorPtr;		/* Text color in normal mode. */
    XColor *dfgColorPtr;	/* Text color in disabled mode. */
    GC textGC;			/* For drawing normal text. */
    Tk_3DBorder selBorder;	/* Borders and backgrounds for selected
				 * elements. */
    int selBorderWidth;		/* Width of border around selection. */
    XColor *selFgColorPtr;	/* Foreground color for selected elements. */
    GC selTextGC;		/* For drawing selected text. */
    int width;			/* Desired width of window, in characters. */
    int height;			/* Desired height of window, in lines. */
    int lineHeight;		/* Number of pixels allocated for each line
				 * in display. */
    int topIndex;		/* Index of top-most element visible in
				 * window. */
    int setGrid;		/* Non-zero means pass gridding information
				 * to window manager. */

    int orient;			/* Orientation: horizontal (multi columns)
				 * or vertical (single vertical column) */
    int elemsPerColumn;         /* number of elements in each column
                                 * (in horizontal orientation) */
    int numColumns;             /* number of horizontal columns */
    int *xColumnMax;            /* x-coord for right edge of each column */
    int xColumnSpace[10];       /* built-in space for xColumnMax */

    /*
     * Information to support horizontal scrolling:
     */

    int maxWidth;		/* Width (in pixels) of widest string in
				 * listbox. */
    int imageWidth;		/* Width (in pixels) of widest icon in
				 * listbox. */
    int imageHeight;		/* Height (in pixels) of tallest icon in
				 * listbox. */
    int xScrollUnit;		/* Number of pixels in one "unit" for
				 * horizontal scrolling (window scrolls
				 * horizontally in increments of this size).
				 * This is an average character size. */
    int xOffset;		/* The left edge of each string in the
				 * listbox is offset to the left by this
				 * many pixels (0 means no offset, positive
				 * means there is an offset). */
    int yOffset;		/* The top edge of all elements in the
				 * listbox is offset to the bottom by this
				 * many pixels (0 means no offset, positive
				 * means there is an offset). */

    /*
     * Information about what's selected or active, if any.
     */

    Tk_Uid selectMode;		/* Selection style: single, browse, multiple,
				 * or extended.  This value isn't used in C
				 * code, but the Tcl bindings use it. */
    int numSelected;		/* Number of elements currently selected. */
    int selectAnchor;		/* Fixed end of selection (i.e. element
				 * at which selection was started.) */
    int exportSelection;	/* Non-zero means tie internal listbox
				 * to X selection. */
    int active;			/* Index of "active" element (the one that
				 * has been selected by keyboard traversal).
				 * -1 means none. */
    int activeStyle;		/* style in which to draw the active element.
				 * One of: underline, none, dotbox */

    /*
     * Information for scanning:
     */

    int scanMarkX;		/* X-position at which scan started (e.g.
				 * button was pressed here). */
    int scanMarkY;		/* Y-position at which scan started (e.g.
				 * button was pressed here). */
    int scanMarkXOffset;	/* Value of "xOffset" field when scan
				 * started. */
    int scanMarkYOffset;	/* Value of "yOffset" field when scan
				 * started. */

    /*
     * Miscellaneous information:
     */

    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */
    char *yScrollCmd;		/* Command prefix for communicating with
				 * vertical scrollbar.  NULL means no command
				 * to issue.  Malloc'ed. */
    char *xScrollCmd;		/* Command prefix for communicating with
				 * horizontal scrollbar.  NULL means no command
				 * to issue.  Malloc'ed. */
    int state;			/* Listbox state. */
    Pixmap gray;		/* Pixmap for displaying disabled text. */
    int flags;			/* Various flag bits:  see below for
				 * definitions. */
} Listbox;

/*
 * ItemAttr structures are used to store item configuration information for
 * the items in a listbox
 */
typedef struct {
    Tk_3DBorder border;		/* Used for drawing background around text */
    Tk_3DBorder selBorder;	/* Used for selected text */
    XColor *fgColor;		/* Text color in normal mode. */
    XColor *selFgColor;		/* Text color in selected mode. */
    char *data;      		/* Extra data string for this item */
    char *imagePtr;		/* Value of -image option (icon for item) */
    Tk_Image image;    		/* Derived from imagePtr for actual image */
    int indent;      		/* Indent item by this many pixels */
} ItemAttr;    

/*
 * Flag bits for listboxes:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler
 *				has already been queued to redraw
 *				this window.
 * UPDATE_V_SCROLLBAR:		Non-zero means vertical scrollbar needs
 *				to be updated.
 * UPDATE_H_SCROLLBAR:		Non-zero means horizontal scrollbar needs
 *				to be updated.
 * GOT_FOCUS:			Non-zero means this widget currently
 *				has the input focus.
 * GEOMETRY_IS_STALE:           Stored maxWidth/imageWidth/imageHeight
 * 				and lineHeight may be out-of-date
 * LISTBOX_DELETED:		This listbox has been effectively destroyed.
 */

#define REDRAW_PENDING		1
#define UPDATE_V_SCROLLBAR	2
#define UPDATE_H_SCROLLBAR	4
#define GOT_FOCUS		8
#define GEOMETRY_IS_STALE	16
#define LISTBOX_DELETED		32

/*
 * The following enum is used to define a type for the -state option
 * of the Entry widget.  These values are used as indices into the 
 * string table below.
 */

enum state {
    STATE_DISABLED, STATE_NORMAL
};

static char *stateStrings[] = {
    "disabled", "normal", (char *) NULL
};

enum activeStyle {
    ACTIVE_STYLE_DOTBOX, ACTIVE_STYLE_NONE, ACTIVE_STYLE_UNDERLINE
};

static char *activeStyleStrings[] = {
    "dotbox", "none", "underline", (char *) NULL
};

enum orientation {
    ORIENT_VERTICAL, ORIENT_HORIZONTAL
};

static char *orientStrings[] = {
    "vertical", "horizontal", (char*)NULL
};


/*
 * The optionSpecs table defines the valid configuration options for the
 * listbox widget
 */
static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_STRING_TABLE, "-activestyle", "activeStyle", "ActiveStyle",
	DEF_LISTBOX_ACTIVE_STYLE, -1, Tk_Offset(Listbox, activeStyle),
        0, (ClientData) activeStyleStrings, 0},
    {TK_OPTION_BORDER, "-background", "background", "Background",
	 DEF_LISTBOX_BG_COLOR, -1, Tk_Offset(Listbox, normalBorder),
	 0, (ClientData) DEF_LISTBOX_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bd", (char *) NULL, (char *) NULL,
	 (char *) NULL, 0, -1, 0, (ClientData) "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", (char *) NULL, (char *) NULL,
	 (char *) NULL, 0, -1, 0, (ClientData) "-background", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	 DEF_LISTBOX_BORDER_WIDTH, -1, Tk_Offset(Listbox, borderWidth),
	 0, 0, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	 DEF_LISTBOX_CURSOR, -1, Tk_Offset(Listbox, cursor),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-disabledforeground", "disabledForeground",
	 "DisabledForeground", DEF_LISTBOX_DISABLED_FG, -1,
	 Tk_Offset(Listbox, dfgColorPtr), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-exportselection", "exportSelection",
	 "ExportSelection", DEF_LISTBOX_EXPORT_SELECTION, -1,
	 Tk_Offset(Listbox, exportSelection), 0, 0, 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", (char *) NULL,
	 (char *) NULL, 0, -1, 0, (ClientData) "-foreground", 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
	 DEF_LISTBOX_FONT, -1, Tk_Offset(Listbox, tkfont), 0, 0, 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	 DEF_LISTBOX_FG, -1, Tk_Offset(Listbox, fgColorPtr), 0, 0, 0},
    {TK_OPTION_INT, "-height", "height", "Height",
	 DEF_LISTBOX_HEIGHT, -1, Tk_Offset(Listbox, height), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
	 "HighlightBackground", DEF_LISTBOX_HIGHLIGHT_BG, -1, 
	 Tk_Offset(Listbox, highlightBgColorPtr), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	 DEF_LISTBOX_HIGHLIGHT, -1, Tk_Offset(Listbox, highlightColorPtr),
	 0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	 "HighlightThickness", DEF_LISTBOX_HIGHLIGHT_WIDTH, -1,
	 Tk_Offset(Listbox, highlightWidth), 0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-orient", "orient", "Orient", "vertical", -1,
	 Tk_Offset(Listbox, orient), 0, (ClientData)orientStrings, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	 DEF_LISTBOX_RELIEF, -1, Tk_Offset(Listbox, relief), 0, 0, 0},
    {TK_OPTION_BORDER, "-selectbackground", "selectBackground", "Foreground",
	 DEF_LISTBOX_SELECT_COLOR, -1, Tk_Offset(Listbox, selBorder),
	 0, (ClientData) DEF_LISTBOX_SELECT_MONO, 0},
    {TK_OPTION_PIXELS, "-selectborderwidth", "selectBorderWidth",
	 "BorderWidth", DEF_LISTBOX_SELECT_BD, -1,
	 Tk_Offset(Listbox, selBorderWidth), 0, 0, 0},
    {TK_OPTION_COLOR, "-selectforeground", "selectForeground", "Background",
	 DEF_LISTBOX_SELECT_FG_COLOR, -1, Tk_Offset(Listbox, selFgColorPtr),
	 TK_CONFIG_NULL_OK, (ClientData) DEF_LISTBOX_SELECT_FG_MONO, 0},
    {TK_OPTION_STRING, "-selectmode", "selectMode", "SelectMode",
	 DEF_LISTBOX_SELECT_MODE, -1, Tk_Offset(Listbox, selectMode),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-setgrid", "setGrid", "SetGrid",
	 DEF_LISTBOX_SET_GRID, -1, Tk_Offset(Listbox, setGrid), 0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-state", "state", "State",
	DEF_LISTBOX_STATE, -1, Tk_Offset(Listbox, state), 
        0, (ClientData) stateStrings, 0},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	 DEF_LISTBOX_TAKE_FOCUS, -1, Tk_Offset(Listbox, takeFocus),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_INT, "-width", "width", "Width",
	 DEF_LISTBOX_WIDTH, -1, Tk_Offset(Listbox, width), 0, 0, 0},
    {TK_OPTION_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	 DEF_LISTBOX_SCROLL_COMMAND, -1, Tk_Offset(Listbox, xScrollCmd),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	 DEF_LISTBOX_SCROLL_COMMAND, -1, Tk_Offset(Listbox, yScrollCmd),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	 (char *) NULL, 0, -1, 0, 0, 0}
};

/*
 * The itemAttrOptionSpecs table defines the valid configuration options for
 * listbox items
 */
static Tk_OptionSpec itemAttrOptionSpecs[] = {
    {TK_OPTION_BORDER, "-background", "background", "Background",
     (char *)NULL, -1, Tk_Offset(ItemAttr, border),
     TK_OPTION_NULL_OK|TK_OPTION_DONT_SET_DEFAULT,
     (ClientData) DEF_LISTBOX_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bg", (char *) NULL, (char *) NULL,
     (char *) NULL, 0, -1, 0, (ClientData) "-background", 0},
    {TK_OPTION_STRING, "-data", "data", "Data",
     (char *) NULL, -1, Tk_Offset(ItemAttr, data),
     TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", (char *) NULL,
     (char *) NULL, 0, -1, 0, (ClientData) "-foreground", 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
     (char *) NULL, -1, Tk_Offset(ItemAttr, fgColor),
     TK_OPTION_NULL_OK|TK_OPTION_DONT_SET_DEFAULT, 0, 0},
    {TK_OPTION_STRING, "-image", "image", "Image",
     (char *) NULL, -1, Tk_Offset(ItemAttr, imagePtr),
     TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_INT, "-indent", "indent", "Indent",
      "0", -1, Tk_Offset(ItemAttr, indent), 0, 0, 0},
    {TK_OPTION_BORDER, "-selectbackground", "selectBackground", "Foreground",
     (char *) NULL, -1, Tk_Offset(ItemAttr, selBorder),
     TK_OPTION_NULL_OK|TK_OPTION_DONT_SET_DEFAULT,
     (ClientData) DEF_LISTBOX_SELECT_MONO, 0},
    {TK_OPTION_COLOR, "-selectforeground", "selectForeground", "Background",
     (char *) NULL, -1, Tk_Offset(ItemAttr, selFgColor),
     TK_OPTION_NULL_OK|TK_OPTION_DONT_SET_DEFAULT,
     (ClientData) DEF_LISTBOX_SELECT_FG_MONO, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
     (char *) NULL, 0, -1, 0, 0, 0}
};

/*
 * The following tables define the listbox widget commands (and sub-
 * commands) and map the indexes into the string tables into 
 * enumerated types used to dispatch the listbox widget command.
 */
static CONST char *commandNames[] = {
    "activate", "bbox", "cget", "configure", "curselection", "delete", "get",
    "index", "insert", "itemcget", "itemconfigure", "nearest", "scan",
    "see", "selection", "size", "xview", "yview",
    (char *) NULL
};

enum command {
    COMMAND_ACTIVATE, COMMAND_BBOX, COMMAND_CGET, COMMAND_CONFIGURE,
    COMMAND_CURSELECTION, COMMAND_DELETE, COMMAND_GET, COMMAND_INDEX,
    COMMAND_INSERT, COMMAND_ITEMCGET, COMMAND_ITEMCONFIGURE,
    COMMAND_NEAREST, COMMAND_SCAN, COMMAND_SEE, COMMAND_SELECTION,
    COMMAND_SIZE, COMMAND_XVIEW, COMMAND_YVIEW
};

static CONST char *selCommandNames[] = {
    "anchor", "clear", "includes", "set", (char *) NULL
};

enum selcommand {
    SELECTION_ANCHOR, SELECTION_CLEAR, SELECTION_INCLUDES, SELECTION_SET
};

static CONST char *scanCommandNames[] = {
    "mark", "dragto", (char *) NULL
};

enum scancommand {
    SCAN_MARK, SCAN_DRAGTO
};

static CONST char *indexNames[] = {
    "active", "anchor", "end", (char *)NULL
};

enum indices {
    INDEX_ACTIVE, INDEX_ANCHOR, INDEX_END
};

static CONST char *bboxOptionNames[] = {
    "-all", "-icon", "-text", (char *) NULL
};

enum bboxoption {
    BBOX_ALL, BBOX_ICON, BBOX_TEXT
};


/* Declarations for procedures defined later in this file */
static void		RpChangeListboxXOffset _ANSI_ARGS_((Listbox *listPtr,
			    int offset));
static void		RpChangeListboxYOffset _ANSI_ARGS_((Listbox *listPtr,
			    int offset));
static void		RpChangeListboxView _ANSI_ARGS_((Listbox *listPtr,
			    int index));
static int		RpConfigureListbox _ANSI_ARGS_((Tcl_Interp *interp,
			    Listbox *listPtr, int objc, Tcl_Obj *CONST objv[],
			    int flags));
static int		RpConfigureListboxItem _ANSI_ARGS_ ((Tcl_Interp *interp,
			    Listbox *listPtr, ItemAttr *attrs, int objc,
			    Tcl_Obj *CONST objv[], int index));
static int		RpListboxDeleteSubCmd _ANSI_ARGS_((Listbox *listPtr,
			    int first, int last));
static void		RpDestroyListbox _ANSI_ARGS_((char *memPtr));
static void		RpDestroyListboxOptionTables _ANSI_ARGS_ (
			    (ClientData clientData, Tcl_Interp *interp));
static void		RpDisplayListbox _ANSI_ARGS_((ClientData clientData));
static void		RpDisplayImage _ANSI_ARGS_((Pixmap pixmap,
			    int w, int h, Tk_Image image, int x, int y));
static void		RpListboxImageProc _ANSI_ARGS_((ClientData clientData,
			    int x, int y, int width, int height,
			    int imgWidth, int imgHeight));
static int		RpGetListboxIndex _ANSI_ARGS_((Tcl_Interp *interp,
			    Listbox *listPtr, Tcl_Obj *index, int endIsSize,
			    int *indexPtr));
static void		RpGetListboxPos _ANSI_ARGS_((Listbox *listPtr,
			    int index, int *rowPtr, int *colPtr));
static int		RpListboxInsertSubCmd _ANSI_ARGS_((Listbox *listPtr,
			    int index, int objc, Tcl_Obj *CONST objv[]));
static void		RpListboxCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		RpListboxComputeGeometry _ANSI_ARGS_((Listbox *listPtr,
			    int fontChanged, int maxIsStale, int updateGrid));
static void		RpListboxEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		RpListboxFetchSelection _ANSI_ARGS_((
			    ClientData clientData, int offset, char *buffer,
			    int maxBytes));
static void		RpListboxLostSelection _ANSI_ARGS_((
			    ClientData clientData));
static void		RpEventuallyRedrawRange _ANSI_ARGS_((Listbox *listPtr,
			    int first, int last));
static void		RpListboxScanTo _ANSI_ARGS_((Listbox *listPtr,
			    int x, int y));
static int		RpListboxSelect _ANSI_ARGS_((Listbox *listPtr,
			    int first, int last, int select));
static void		RpListboxUpdateHScrollbar _ANSI_ARGS_(
    			    (Listbox *listPtr));
static void		RpListboxUpdateVScrollbar _ANSI_ARGS_(
			    (Listbox *listPtr));
static int		RpListboxWidgetObjCmd _ANSI_ARGS_((ClientData clientData,
	                    Tcl_Interp *interp, int objc,
	                    Tcl_Obj *CONST objv[]));
static int		RpListboxBboxSubCmd _ANSI_ARGS_ ((Tcl_Interp *interp,
	                    Listbox *listPtr, int index, int what));
static int		RpListboxSelectionSubCmd _ANSI_ARGS_ (
			    (Tcl_Interp *interp, Listbox *listPtr, int objc,
			    Tcl_Obj *CONST objv[]));
static int		RpListboxXviewSubCmd _ANSI_ARGS_ ((Tcl_Interp *interp,
			    Listbox *listPtr, int objc,
			    Tcl_Obj *CONST objv[]));
static int		RpListboxYviewSubCmd _ANSI_ARGS_ ((Tcl_Interp *interp,
			    Listbox *listPtr, int objc,
			    Tcl_Obj *CONST objv[]));
static ItemAttr *	RpListboxGetItemAttributes _ANSI_ARGS_ (
    			    (Tcl_Interp *interp, Listbox *listPtr, int index));
static void		RpListboxWorldChanged _ANSI_ARGS_((
			    ClientData instanceData));
static int		RpNearestListboxElement _ANSI_ARGS_((Listbox *listPtr,
			    int x, int y));
static void		RpMigrateHashEntries _ANSI_ARGS_ ((Tcl_HashTable *table,
			    int first, int last, int offset));
/*
 * The structure below defines button class behavior by means of procedures
 * that can be invoked from generic window code.
 */

static Tk_ClassProcs listboxClass = {
    sizeof(Tk_ClassProcs),	/* size */
    RpListboxWorldChanged,	/* worldChangedProc */
};

extern Tcl_ObjCmdProc RpListboxObjCmd;


/*
 * ------------------------------------------------------------------------
 *  RpListbox_Init --
 *
 *  Invoked when the Rappture GUI library is being initialized
 *  to install the Rappture "listbox" widget.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpListbox_Init(interp)
    Tcl_Interp *interp;         /* interpreter being initialized */
{
    Tcl_CreateObjCommand(interp, "Rappture::listbox", RpListboxObjCmd,
        (ClientData)NULL, NULL);

    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * RpListboxObjCmd --
 *
 *	This procedure is invoked to process the "listbox" Tcl
 *	command.  See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
RpListboxObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* NULL. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    register Listbox *listPtr;
    Tk_Window tkwin;
    ListboxOptionTables *optionTables;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?options?");
	return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp),
	    Tcl_GetString(objv[1]), (char *) NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    optionTables = (ListboxOptionTables *)
	Tcl_GetAssocData(interp, "RpListboxOptionTables", NULL);
    if (optionTables == NULL) {
	/*
	 * We haven't created the option tables for this widget class yet.
	 * Do it now and save the a pointer to them as the ClientData for
	 * the command, so future invocations will have access to it.
	 */

	optionTables = (ListboxOptionTables *)
	    ckalloc(sizeof(ListboxOptionTables));
	/* Set up an exit handler to free the optionTables struct */
	Tcl_SetAssocData(interp, "RpListboxOptionTables",
		RpDestroyListboxOptionTables, (ClientData) optionTables);

	/* Create the listbox option table and the listbox item option table */
	optionTables->listboxOptionTable =
	    Tk_CreateOptionTable(interp, optionSpecs);
	optionTables->itemAttrOptionTable =
	    Tk_CreateOptionTable(interp, itemAttrOptionSpecs);
    }

    /*
     * Initialize the fields of the structure that won't be initialized
     * by RpConfigureListbox, or that RpConfigureListbox requires to be
     * initialized already (e.g. resource pointers).
     */
    listPtr 				= (Listbox *) ckalloc(sizeof(Listbox));
    memset((void *) listPtr, 0, (sizeof(Listbox)));

    listPtr->tkwin 			= tkwin;
    listPtr->display 			= Tk_Display(tkwin);
    listPtr->interp 			= interp;
    listPtr->widgetCmd 			= Tcl_CreateObjCommand(interp,
	    Tk_PathName(listPtr->tkwin), RpListboxWidgetObjCmd,
	    (ClientData) listPtr, RpListboxCmdDeletedProc);
    listPtr->optionTable 		= optionTables->listboxOptionTable;
    listPtr->itemAttrOptionTable	= optionTables->itemAttrOptionTable;
    listPtr->selection 			=
	(Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(listPtr->selection, TCL_ONE_WORD_KEYS);
    listPtr->itemAttrTable 		=
	(Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(listPtr->itemAttrTable, TCL_ONE_WORD_KEYS);
    listPtr->relief 			= TK_RELIEF_RAISED;
    listPtr->textGC 			= None;
    listPtr->selFgColorPtr 		= None;
    listPtr->selTextGC 			= None;
    listPtr->xScrollUnit 		= 1;
    listPtr->exportSelection 		= 1;
    listPtr->cursor 			= None;
    listPtr->state			= STATE_NORMAL;
    listPtr->gray			= None;
    listPtr->orient			= ORIENT_VERTICAL;

    /*
     * Keep a hold of the associated tkwin until we destroy the listbox,
     * otherwise Tk might free it while we still need it.
     */

    Tcl_Preserve((ClientData) listPtr->tkwin);

    Tk_SetClass(listPtr->tkwin, "Listbox");
    Tk_SetClassProcs(listPtr->tkwin, &listboxClass, (ClientData) listPtr);
    Tk_CreateEventHandler(listPtr->tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    RpListboxEventProc, (ClientData) listPtr);
    Tk_CreateSelHandler(listPtr->tkwin, XA_PRIMARY, XA_STRING,
	    RpListboxFetchSelection, (ClientData) listPtr, XA_STRING);
    if (Tk_InitOptions(interp, (char *)listPtr,
	    optionTables->listboxOptionTable, tkwin) != TCL_OK) {
	Tk_DestroyWindow(listPtr->tkwin);
	return TCL_ERROR;
    }

    if (RpConfigureListbox(interp, listPtr, objc-2, objv+2, 0) != TCL_OK) {
	Tk_DestroyWindow(listPtr->tkwin);
	return TCL_ERROR;
    }

    Tcl_SetResult(interp, Tk_PathName(listPtr->tkwin), TCL_STATIC);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxWidgetObjCmd --
 *
 *	This Tcl_Obj based procedure is invoked to process the Tcl command
 *      that corresponds to a widget managed by this module.  See the user
 *      documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
RpListboxWidgetObjCmd(clientData, interp, objc, objv)
    ClientData clientData;		/* Information about listbox widget. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Arguments as Tcl_Obj's. */
{
    register Listbox *listPtr = (Listbox *) clientData;
    int cmdIndex, index;
    int result = TCL_OK;
    
    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }

    /*
     * Parse the command by looking up the second argument in the list
     * of valid subcommand names
     */
    result = Tcl_GetIndexFromObj(interp, objv[1], commandNames,
	    "option", 0, &cmdIndex);
    if (result != TCL_OK) {
	return result;
    }

    Tcl_Preserve((ClientData)listPtr);
    /* The subcommand was valid, so continue processing */
    switch (cmdIndex) {
	case COMMAND_ACTIVATE: {
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "index");
		result = TCL_ERROR;
		break;
	    }
	    result = RpGetListboxIndex(interp, listPtr, objv[2], 0, &index);
	    if (result != TCL_OK) {
		break;
	    }

	    if (!(listPtr->state & STATE_NORMAL)) {
		break;
	    }

	    if (index >= listPtr->nElements) {
		index = listPtr->nElements-1;
	    }
	    if (index < 0) {
		index = 0;
	    }
	    listPtr->active = index;
	    RpEventuallyRedrawRange(listPtr, listPtr->active, listPtr->active);
	    result = TCL_OK;
	    break;
	}

	case COMMAND_BBOX: {
            int bboxOpt = BBOX_ALL;
	    if (objc < 3 || objc > 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "index ?what?");
		result = TCL_ERROR;
		break;
	    }
	    result = RpGetListboxIndex(interp, listPtr, objv[2], 0, &index);
	    if (result != TCL_OK) {
		break;
	    }

            if (objc > 3) {
	        result = Tcl_GetIndexFromObj(interp, objv[3], bboxOptionNames,
		        "what", 0, &bboxOpt);
	        if (result != TCL_OK) {
		    break;
	        }
            }
	    result = RpListboxBboxSubCmd(interp, listPtr, index, bboxOpt);
	    break;
	}

	case COMMAND_CGET: {
	    Tcl_Obj *objPtr;
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "option");
		result = TCL_ERROR;
		break;
	    }

	    objPtr = Tk_GetOptionValue(interp, (char *)listPtr,
		    listPtr->optionTable, objv[2], listPtr->tkwin);
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
		objPtr = Tk_GetOptionInfo(interp, (char *) listPtr,
			listPtr->optionTable,
			(objc == 3) ? objv[2] : (Tcl_Obj *) NULL,
			listPtr->tkwin);
		if (objPtr == NULL) {
		    result = TCL_ERROR;
		    break;
		} else {
		    Tcl_SetObjResult(interp, objPtr);
		    result = TCL_OK;
		}
	    } else {
		result = RpConfigureListbox(interp, listPtr, objc-2, objv+2, 0);
	    }
	    break;
	}

	case COMMAND_CURSELECTION: {
	    char indexStringRep[TCL_INTEGER_SPACE];
	    int i;
	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		result = TCL_ERROR;
		break;
	    }
	    /*
	     * Of course, it would be more efficient to use the Tcl_HashTable
	     * search functions (Tcl_FirstHashEntry, Tcl_NextHashEntry), but
	     * then the result wouldn't be in sorted order.  So instead we
	     * loop through the indices in order, adding them to the result
	     * if they are selected
	     */
	    for (i = 0; i < listPtr->nElements; i++) {
		if (Tcl_FindHashEntry(listPtr->selection, (char *)i) != NULL) {
		    sprintf(indexStringRep, "%d", i);
		    Tcl_AppendElement(interp, indexStringRep);
		}
	    }
	    result = TCL_OK;
	    break;
	}
	
	case COMMAND_DELETE: {
	    int first, last;
	    if ((objc < 3) || (objc > 4)) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"firstIndex ?lastIndex?");
		result = TCL_ERROR;
		break;
	    }

	    result = RpGetListboxIndex(interp, listPtr, objv[2], 0, &first);
	    if (result != TCL_OK) {
		break;
	    }

	    if (!(listPtr->state & STATE_NORMAL)) {
		break;
	    }

	    if (first < listPtr->nElements) {
		/*
		 * if a "last index" was given, get it now; otherwise, use the
		 * first index as the last index
		 */
		if (objc == 4) {
		    result = RpGetListboxIndex(interp, listPtr,
			    objv[3], 0, &last);
		    if (result != TCL_OK) {
			break;
		    }
		} else {
		    last = first;
		}
		if (last >= listPtr->nElements) {
		    last = listPtr->nElements - 1;
		}
		result = RpListboxDeleteSubCmd(listPtr, first, last);
	    } else {
		result = TCL_OK;
	    }
	    break;
	}

	case COMMAND_GET: {
	    int first, last;
	    Tcl_Obj **elemPtrs;
	    int listLen;
	    if (objc != 3 && objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "firstIndex ?lastIndex?");
		result = TCL_ERROR;
		break;
	    }
	    result = RpGetListboxIndex(interp, listPtr, objv[2], 0, &first);
	    if (result != TCL_OK) {
		break;
	    }
	    last = first;
	    if (objc == 4) {
		result = RpGetListboxIndex(interp, listPtr, objv[3], 0, &last);
		if (result != TCL_OK) {
		    break;
		}
	    }
	    if (first >= listPtr->nElements) {
		result = TCL_OK;
		break;
	    }
	    if (last >= listPtr->nElements) {
		last = listPtr->nElements - 1;
	    }
	    if (first < 0) {
		first = 0;
	    }
	    if (first > last) {
		result = TCL_OK;
		break;
	    }
	    result = Tcl_ListObjGetElements(interp, listPtr->listObj, &listLen,
		    &elemPtrs);
	    if (result != TCL_OK) {
		break;
	    }
	    if (objc == 3) {
		/*
		 * One element request - we return a string
		 */
		Tcl_SetObjResult(interp, elemPtrs[first]);
	    } else {
		Tcl_SetListObj(Tcl_GetObjResult(interp), (last - first + 1),
			&(elemPtrs[first]));
	    }
	    result = TCL_OK;
	    break;
	}

	case COMMAND_INDEX:{
	    char buf[TCL_INTEGER_SPACE];
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "index");
		result = TCL_ERROR;
		break;
	    }
	    result = RpGetListboxIndex(interp, listPtr, objv[2], 1, &index);
	    if (result != TCL_OK) {
		break;
	    }
	    sprintf(buf, "%d", index);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    result = TCL_OK;
	    break;
	}

	case COMMAND_INSERT: {
	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"index ?element element ...?");
		result = TCL_ERROR;
		break;
	    }

	    result = RpGetListboxIndex(interp, listPtr, objv[2], 1, &index);
	    if (result != TCL_OK) {
		break;
	    }

	    if (!(listPtr->state & STATE_NORMAL)) {
		break;
	    }

	    result = RpListboxInsertSubCmd(listPtr, index, objc-3, objv+3);
	    break;
	}

	case COMMAND_ITEMCGET: {
	    Tcl_Obj *objPtr;
	    ItemAttr *attrPtr;
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "index option");
		result = TCL_ERROR;
		break;
	    }

	    result = RpGetListboxIndex(interp, listPtr, objv[2], 0, &index);
	    if (result != TCL_OK) {
		break;
	    }

	    if (index < 0 || index >= listPtr->nElements) {
		Tcl_AppendResult(interp, "item number \"",
			Tcl_GetString(objv[2]), "\" out of range",
			(char *)NULL);
		result = TCL_ERROR;
		break;
	    }
	    
	    attrPtr = RpListboxGetItemAttributes(interp, listPtr, index);

	    objPtr = Tk_GetOptionValue(interp, (char *)attrPtr,
		    listPtr->itemAttrOptionTable, objv[3], listPtr->tkwin);
	    if (objPtr == NULL) {
		result = TCL_ERROR;
		break;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	    result = TCL_OK;
	    break;
	}

	case COMMAND_ITEMCONFIGURE: {
	    Tcl_Obj *objPtr;
	    ItemAttr *attrPtr;
	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"index ?option? ?value? ?option value ...?");
		result = TCL_ERROR;
		break;
	    }

	    result = RpGetListboxIndex(interp, listPtr, objv[2], 0, &index);
	    if (result != TCL_OK) {
		break;
	    }
	    
	    if (index < 0 || index >= listPtr->nElements) {
		Tcl_AppendResult(interp, "item number \"",
			Tcl_GetString(objv[2]), "\" out of range",
			(char *)NULL);
		result = TCL_ERROR;
		break;
	    }
	    
	    attrPtr = RpListboxGetItemAttributes(interp, listPtr, index);
	    if (objc <= 4) {
		objPtr = Tk_GetOptionInfo(interp, (char *)attrPtr,
			listPtr->itemAttrOptionTable,
			(objc == 4) ? objv[3] : (Tcl_Obj *) NULL,
			listPtr->tkwin);
		if (objPtr == NULL) {
		    result = TCL_ERROR;
		    break;
		} else {
		    Tcl_SetObjResult(interp, objPtr);
		    result = TCL_OK;
		}
	    } else {
		result = RpConfigureListboxItem(interp, listPtr, attrPtr,
			objc-3, objv+3, index);
	    }
	    break;
	}
	
	case COMMAND_NEAREST: {
	    char buf[TCL_INTEGER_SPACE];
	    int x, y;
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "x y");
		result = TCL_ERROR;
		break;
	    }
	    
	    result = Tcl_GetIntFromObj(interp, objv[2], &x);
	    if (result != TCL_OK) {
		break;
	    }
	    result = Tcl_GetIntFromObj(interp, objv[3], &y);
	    if (result != TCL_OK) {
		break;
	    }

	    index = RpNearestListboxElement(listPtr, x, y);
	    sprintf(buf, "%d", index);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    result = TCL_OK;
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
		    listPtr->scanMarkX = x;
		    listPtr->scanMarkY = y;
		    listPtr->scanMarkXOffset = listPtr->xOffset;
		    listPtr->scanMarkYOffset = listPtr->yOffset;
		    break;
		}
		case SCAN_DRAGTO: {
		    RpListboxScanTo(listPtr, x, y);
		    break;
		}
	    }
	    result = TCL_OK;
	    break;
	}

	case COMMAND_SEE: {
	    int first, last, nrow, ncol, x0, y0, x1, y1;
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "index");
		result = TCL_ERROR;
		break;
	    }
	    result = RpGetListboxIndex(interp, listPtr, objv[2], 0, &index);
	    if (result != TCL_OK) {
		break;
	    }
	    if (index >= listPtr->nElements) {
		index = listPtr->nElements - 1;
	    }
	    if (index < 0) {
		index = 0;
	    }

            /* find the first element that is completely on screen */
            first = RpNearestListboxElement(listPtr,
                listPtr->inset, listPtr->inset);

            RpGetListboxPos(listPtr, first, &nrow, &ncol);
            x0 = (ncol > 0) ? listPtr->xColumnMax[ncol-1] : 0;
            y0 = nrow*listPtr->lineHeight;
            if (x0 < listPtr->xOffset) {
                first += listPtr->elemsPerColumn;
            }
            if (y0 < listPtr->yOffset) {
                first++;
            }

            last = RpNearestListboxElement(listPtr,
                Tk_Width(listPtr->tkwin)-listPtr->inset,
                Tk_Height(listPtr->tkwin)-listPtr->inset);

            RpGetListboxPos(listPtr, last, &nrow, &ncol);
            x1 = listPtr->xColumnMax[ncol];
            y1 = (nrow+1)*listPtr->lineHeight;
            if (x1 > listPtr->xOffset + Tk_Width(listPtr->tkwin)) {
                last -= listPtr->elemsPerColumn;
            }
            if (y1 > listPtr->yOffset + Tk_Height(listPtr->tkwin)) {
                last--;
            }

            if (index < first || index > last) {
                if (listPtr->orient == ORIENT_VERTICAL) {
                    /* center requested element in view */
                    if (listPtr->lineHeight > 0) {
                        index -= Tk_Height(listPtr->tkwin)
                                  / listPtr->lineHeight / 2;
                    }
                }
		RpChangeListboxView(listPtr, index);
	    }
	    result = TCL_OK;
	    break;
	}

	case COMMAND_SELECTION: {
	    result = RpListboxSelectionSubCmd(interp, listPtr, objc, objv);
	    break;
	}

	case COMMAND_SIZE: {
	    char buf[TCL_INTEGER_SPACE];
	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		result = TCL_ERROR;
		break;
	    }
	    sprintf(buf, "%d", listPtr->nElements);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    result = TCL_OK;
	    break;
	}

	case COMMAND_XVIEW: {
	    result = RpListboxXviewSubCmd(interp, listPtr, objc, objv);
	    break;
	}
	
	case COMMAND_YVIEW: {
	    result = RpListboxYviewSubCmd(interp, listPtr, objc, objv);
	    break;
	}
    }
    Tcl_Release((ClientData)listPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxBboxSubCmd --
 *
 *	This procedure is invoked to process a listbox bbox request.
 *      See the user documentation for more information.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	For valid indices, places the bbox of the requested element in
 *      the interpreter's result.
 *
 *----------------------------------------------------------------------
 */

static int
RpListboxBboxSubCmd(interp, listPtr, index, what)
    Tcl_Interp *interp;          /* Pointer to the calling Tcl interpreter */
    Listbox *listPtr;            /* Information about the listbox */
    int index;                   /* Index of the element to get bbox info on */
    int what;                    /* Return bbox around icon, text, or all */
{
    int nrow, ncol, x0, y0, width, height;
    ItemAttr *attrPtr;
    char buf[TCL_INTEGER_SPACE * 4];
    Tcl_Obj *el;
    char *stringRep;
    int stringLen, result;
    Tk_FontMetrics fm;

    result = Tcl_ListObjIndex(interp, listPtr->listObj, index, &el);
    if (result != TCL_OK) {
        return result;
    }
    stringRep = Tcl_GetStringFromObj(el, &stringLen);

    RpGetListboxPos(listPtr, index, &nrow, &ncol);
    x0 = listPtr->inset + ((ncol > 0) ? listPtr->xColumnMax[ncol-1] : 0);
    y0 = listPtr->inset + nrow*listPtr->lineHeight;
    width = 0;
    height = listPtr->lineHeight;

    Tk_GetFontMetrics(listPtr->tkfont, &fm);

    attrPtr = RpListboxGetItemAttributes((Tcl_Interp*)NULL, listPtr, index);
    if (attrPtr) {
        x0 += attrPtr->indent;
    }

    switch (what) {
        case BBOX_ICON:
            if (attrPtr->image) {
                width = listPtr->imageWidth;
            } else {
                width = height = 0;
            }
            break;

        case BBOX_TEXT:
            /* Compute the pixel width of the requested element */
            x0 += listPtr->imageWidth+1;
            width += Tk_TextWidth(listPtr->tkfont, stringRep, stringLen)+2;
            height = fm.linespace;
            y0 += (listPtr->lineHeight - fm.linespace)/2;
            break;

        case BBOX_ALL:
            if (attrPtr->image) {
                width += listPtr->imageWidth + 4;
            } else if (listPtr->imageWidth > 0) {
                x0 += listPtr->imageWidth + 2;
            }

            /* Compute the pixel width of the requested element */
            width += Tk_TextWidth(listPtr->tkfont, stringRep, stringLen)+2;
            break;
    }

    sprintf(buf, "%d %d %d %d", x0-listPtr->xOffset, y0-listPtr->yOffset,
        width, height);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxSelectionSubCmd --
 *
 *	This procedure is invoked to process the selection sub command
 *      for listbox widgets.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	May set the interpreter's result field.
 *
 *----------------------------------------------------------------------
 */

static int
RpListboxSelectionSubCmd(interp, listPtr, objc, objv)
    Tcl_Interp *interp;          /* Pointer to the calling Tcl interpreter */
    Listbox *listPtr;            /* Information about the listbox */
    int objc;                    /* Number of arguments in the objv array */
    Tcl_Obj *CONST objv[];       /* Array of arguments to the procedure */
{
    int selCmdIndex, first, last;
    int result = TCL_OK;
    if (objc != 4 && objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "option index ?index?");
	return TCL_ERROR;
    }
    result = RpGetListboxIndex(interp, listPtr, objv[3], 0, &first);
    if (result != TCL_OK) {
	return result;
    }
    last = first;
    if (objc == 5) {
	result = RpGetListboxIndex(interp, listPtr, objv[4], 0, &last);
	if (result != TCL_OK) {
	    return result;
	}
    }
    result = Tcl_GetIndexFromObj(interp, objv[2], selCommandNames,
	    "option", 0, &selCmdIndex);
    if (result != TCL_OK) {
	return result;
    }

    /*
     * Only allow 'selection includes' to respond if disabled. [Bug #632514]
     */

    if ((listPtr->state == STATE_DISABLED)
	    && (selCmdIndex != SELECTION_INCLUDES)) {
	return TCL_OK;
    }

    switch (selCmdIndex) {
	case SELECTION_ANCHOR: {
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "index");
		return TCL_ERROR;
	    }
	    if (first >= listPtr->nElements) {
		first = listPtr->nElements - 1;
	    }
	    if (first < 0) {
		first = 0;
	    }
	    listPtr->selectAnchor = first;
	    result = TCL_OK;
	    break;
	}
	case SELECTION_CLEAR: {
	    result = RpListboxSelect(listPtr, first, last, 0);
	    break;
	}
	case SELECTION_INCLUDES: {
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "index");
		return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp,
		    Tcl_NewBooleanObj((Tcl_FindHashEntry(listPtr->selection,
			    (char *)first) != NULL)));
	    result = TCL_OK;
	    break;
	}
	case SELECTION_SET: {
	    result = RpListboxSelect(listPtr, first, last, 1);
	    break;
	}
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxXviewSubCmd --
 *
 *	Process the listbox "xview" subcommand.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	May change the listbox viewing area; may set the interpreter's result.
 *
 *----------------------------------------------------------------------
 */

static int
RpListboxXviewSubCmd(interp, listPtr, objc, objv)
    Tcl_Interp *interp;          /* Pointer to the calling Tcl interpreter */
    Listbox *listPtr;            /* Information about the listbox */
    int objc;                    /* Number of arguments in the objv array */
    Tcl_Obj *CONST objv[];       /* Array of arguments to the procedure */
{

    int index, count, type, windowWidth, windowUnits;
    int offset = 0;		/* Initialized to stop gcc warnings. */
    double fraction, fraction2;
    
    windowWidth = Tk_Width(listPtr->tkwin) - 2*listPtr->inset;

    if (objc == 2) {
	if (listPtr->maxWidth < 5) {
	    Tcl_SetResult(interp, "0 1", TCL_STATIC);
	} else {
	    char buf[TCL_DOUBLE_SPACE * 2];
	    
	    fraction = listPtr->xOffset/((double)listPtr->maxWidth);
	    fraction2 = (listPtr->xOffset + windowWidth)
		/((double)listPtr->maxWidth);
	    if (fraction2 > 1.0) {
		fraction2 = 1.0;
	    }
	    sprintf(buf, "%g %g", fraction, fraction2);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	}
    } else if (objc == 3) {
	if (Tcl_GetIntFromObj(interp, objv[2], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	RpChangeListboxXOffset(listPtr, index*listPtr->xScrollUnit);
    } else {
	type = Tk_GetScrollInfoObj(interp, objc, objv, &fraction, &count);
	switch (type) {
	    case TK_SCROLL_ERROR:
		return TCL_ERROR;
	    case TK_SCROLL_MOVETO:
		offset = (int) (fraction*listPtr->maxWidth + 0.5);
		break;
	    case TK_SCROLL_PAGES:
		windowUnits = windowWidth/listPtr->xScrollUnit;
		if (windowUnits > 2) {
		    offset = listPtr->xOffset
			+ count*listPtr->xScrollUnit*(windowUnits-2);
		} else {
		    offset = listPtr->xOffset + count*listPtr->xScrollUnit;
		}
		break;
	    case TK_SCROLL_UNITS:
		offset = listPtr->xOffset + count*listPtr->xScrollUnit;
		break;
	}
	RpChangeListboxXOffset(listPtr, offset);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxYviewSubCmd --
 *
 *	Process the listbox "yview" subcommand.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	May change the listbox viewing area; may set the interpreter's result.
 *
 *----------------------------------------------------------------------
 */

static int
RpListboxYviewSubCmd(interp, listPtr, objc, objv)
    Tcl_Interp *interp;          /* Pointer to the calling Tcl interpreter */
    Listbox *listPtr;            /* Information about the listbox */
    int objc;                    /* Number of arguments in the objv array */
    Tcl_Obj *CONST objv[];       /* Array of arguments to the procedure */
{
    int index, count, type, worldHeight, yOffset;
    double fraction, fraction2;

    /* make sure that layout is fresh -- we'll need it below */
    if (listPtr->flags & GEOMETRY_IS_STALE) {
        RpListboxComputeGeometry(listPtr, 0, 1, 0);
        listPtr->flags &= ~GEOMETRY_IS_STALE;
    }
    worldHeight = listPtr->elemsPerColumn*listPtr->lineHeight;

    if (objc == 2) {
	if (listPtr->nElements == 0) {
	    Tcl_SetResult(interp, "0 1", TCL_STATIC);
	} else {
	    char buf[TCL_DOUBLE_SPACE * 2];

	    fraction = listPtr->yOffset/((double)worldHeight);
	    fraction2 = (listPtr->yOffset+Tk_Height(listPtr->tkwin))
                          / ((double)worldHeight);
	    if (fraction2 > 1.0) {
		fraction2 = 1.0;
	    }
	    sprintf(buf, "%g %g", fraction, fraction2);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	}
    } else if (objc == 3) {
	if (RpGetListboxIndex(interp, listPtr, objv[2], 0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	RpChangeListboxView(listPtr, index);
    } else {
        yOffset = 0;
	type = Tk_GetScrollInfoObj(interp, objc, objv, &fraction, &count);
	switch (type) {
	    case TK_SCROLL_ERROR:
		return TCL_ERROR;
	    case TK_SCROLL_MOVETO:
                yOffset = fraction*worldHeight;
		break;
	    case TK_SCROLL_PAGES:
                yOffset = listPtr->yOffset + count*Tk_Height(listPtr->tkwin);
		break;
	    case TK_SCROLL_UNITS:
		yOffset = listPtr->yOffset + count*listPtr->lineHeight;
		break;
	}
        RpChangeListboxYOffset(listPtr, yOffset);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxGetItemAttributes --
 *
 *	Returns a pointer to the ItemAttr record for a given index,
 *	creating one if it does not already exist.
 *
 * Results:
 *	Pointer to an ItemAttr record.
 *
 * Side effects:
 *	Memory may be allocated for the ItemAttr record.
 *
 *----------------------------------------------------------------------
 */

static ItemAttr *
RpListboxGetItemAttributes(interp, listPtr, index)
    Tcl_Interp *interp;          /* Pointer to the calling Tcl interpreter */
    Listbox *listPtr;            /* Information about the listbox */
    int index;                   /* Index of the item to retrieve attributes
				  * for */
{
    int new;
    Tcl_HashEntry *entry;
    ItemAttr *attrs;

    entry = Tcl_CreateHashEntry(listPtr->itemAttrTable, (char *)index, &new);
    if (new) {
	attrs = (ItemAttr *) ckalloc(sizeof(ItemAttr));
	attrs->border = NULL;
	attrs->selBorder = NULL;
	attrs->fgColor = NULL;
	attrs->selFgColor = NULL;
	attrs->data = NULL;
	attrs->indent = 0;
	attrs->imagePtr = NULL;
	attrs->image = NULL;
	Tk_InitOptions(interp, (char *)attrs, listPtr->itemAttrOptionTable,
		listPtr->tkwin);
	Tcl_SetHashValue(entry, (ClientData) attrs);
    }
    attrs = (ItemAttr *)Tcl_GetHashValue(entry);
    return attrs;
}

/*
 *----------------------------------------------------------------------
 *
 * RpDestroyListbox --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a listbox at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the listbox is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
RpDestroyListbox(memPtr)
    char *memPtr;	/* Info about listbox widget. */
{
    register Listbox *listPtr = (Listbox *) memPtr;
    Tcl_HashEntry *entry;
    Tcl_HashSearch search;
    ItemAttr *attrPtr;

    /* If we have an internal list object, free it */
    if (listPtr->listObj != NULL) {
	Tcl_DecrRefCount(listPtr->listObj);
	listPtr->listObj = NULL;
    }

    /* Free the selection hash table */
    Tcl_DeleteHashTable(listPtr->selection);
    ckfree((char *)listPtr->selection);

    /* Free the item attribute hash table */
    for (entry = Tcl_FirstHashEntry(listPtr->itemAttrTable, &search);
	 entry != NULL; entry = Tcl_NextHashEntry(&search)) {

        attrPtr = (ItemAttr *)Tcl_GetHashValue(entry);

        Tk_FreeConfigOptions((char*)attrPtr, listPtr->itemAttrOptionTable,
            listPtr->tkwin);

        if (attrPtr->image != NULL) {
            Tk_FreeImage(attrPtr->image);
        }
	ckfree((char *)attrPtr);
    }
    Tcl_DeleteHashTable(listPtr->itemAttrTable);
    ckfree((char *)listPtr->itemAttrTable);

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeOptions handle all the standard option-related
     * stuff.
     */

    if (listPtr->textGC != None) {
	Tk_FreeGC(listPtr->display, listPtr->textGC);
    }
    if (listPtr->selTextGC != None) {
	Tk_FreeGC(listPtr->display, listPtr->selTextGC);
    }
    if (listPtr->gray != None) {
	Tk_FreeBitmap(Tk_Display(listPtr->tkwin), listPtr->gray);
    }

    Tk_FreeConfigOptions((char *)listPtr, listPtr->optionTable,
	    listPtr->tkwin);
    Tcl_Release((ClientData) listPtr->tkwin);
    listPtr->tkwin = NULL;
    ckfree((char *) listPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * RpDestroyListboxOptionTables --
 *
 *	This procedure is registered as an exit callback when the listbox
 *	command is first called.  It cleans up the OptionTables structure
 *	allocated by that command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 *----------------------------------------------------------------------
 */

static void
RpDestroyListboxOptionTables(clientData, interp)
    ClientData clientData;	/* Pointer to the OptionTables struct */
    Tcl_Interp *interp;		/* Pointer to the calling interp */
{
    ckfree((char *)clientData);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * RpConfigureListbox --
 *
 *	This procedure is called to process an objv/objc list, plus
 *	the Tk option database, in order to configure (or reconfigure)
 *	a listbox widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for listPtr;  old resources get freed,
 *	if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
RpConfigureListbox(interp, listPtr, objc, objv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register Listbox *listPtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int objc;			/* Number of valid entries in argv. */
    Tcl_Obj *CONST objv[];	/* Arguments. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    int oldExport, error;

    oldExport = listPtr->exportSelection;

    for (error = 0; error <= 1; error++) {
	if (!error) {
	    /*
	     * First pass: set options to new values.
	     */

	    if (Tk_SetOptions(interp, (char *) listPtr,
		    listPtr->optionTable, objc, objv,
		    listPtr->tkwin, &savedOptions, (int *) NULL) != TCL_OK) {
		continue;
	    }
	} else {
	    /*
	     * Second pass: restore options to old values.
	     */

	    errorResult = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);
	}

	/*
	 * A few options need special processing, such as setting the
	 * background from a 3-D border.
	 */

	Tk_SetBackgroundFromBorder(listPtr->tkwin, listPtr->normalBorder);

	if (listPtr->highlightWidth < 0) {
	    listPtr->highlightWidth = 0;
	}
	listPtr->inset = listPtr->highlightWidth + listPtr->borderWidth;

	/*
	 * Claim the selection if we've suddenly started exporting it and
	 * there is a selection to export.
	 */

	if (listPtr->exportSelection && !oldExport
		&& (listPtr->numSelected != 0)) {
	    Tk_OwnSelection(listPtr->tkwin, XA_PRIMARY, RpListboxLostSelection,
		    (ClientData) listPtr);
	}

	if (listPtr->listObj == NULL) {
	    listPtr->listObj = Tcl_NewObj();
	}
	Tcl_IncrRefCount(listPtr->listObj);
	break;
    }
    if (!error) {
	Tk_FreeSavedOptions(&savedOptions);
    }

    /* Make sure that the list length is correct */
    Tcl_ListObjLength(listPtr->interp, listPtr->listObj, &listPtr->nElements);
    
    if (error) {
        Tcl_SetObjResult(interp, errorResult);
	Tcl_DecrRefCount(errorResult);
	return TCL_ERROR;
    } else {
	RpListboxWorldChanged((ClientData) listPtr);
	return TCL_OK;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RpConfigureListboxItem --
 *
 *	This procedure is called to process an objv/objc list, plus
 *	the Tk option database, in order to configure (or reconfigure)
 *	a listbox item.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for a listbox item;  old resources get freed,
 *	if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
RpConfigureListboxItem(interp, listPtr, attrs, objc, objv, index)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register Listbox *listPtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    ItemAttr *attrs;		/* Information about the item to configure */
    int objc;			/* Number of valid entries in argv. */
    Tcl_Obj *CONST objv[];	/* Arguments. */
    int index;			/* Index of the listbox item being configure */
{
    Tk_SavedOptions savedOptions;
    Tk_Image image;

    if (Tk_SetOptions(interp, (char *)attrs,
	    listPtr->itemAttrOptionTable, objc, objv, listPtr->tkwin,
	    &savedOptions, (int *)NULL) != TCL_OK) {
	Tk_RestoreSavedOptions(&savedOptions);
	return TCL_ERROR;
    }
    Tk_FreeSavedOptions(&savedOptions);

    /*
     *  If this item has an image name, then translate it to an
     *  image that we can draw.
     */
    if (attrs->imagePtr != NULL) {
        image = Tk_GetImage(interp, listPtr->tkwin,
                attrs->imagePtr, RpListboxImageProc,
                (ClientData) attrs);
    } else {
        image = NULL;
    }
    if (attrs->image != NULL) {
        Tk_FreeImage(attrs->image);
    }
    attrs->image = image;

    /*
     * If this item has an image or an indent, then we should recompute
     * the geometry and all bets are off.
     */
    if (attrs->image || attrs->indent > 0) {
        listPtr->flags |= GEOMETRY_IS_STALE;
        RpEventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
    } else {
        /* okay to redraw just this index */
        RpEventuallyRedrawRange(listPtr, index, index);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RpListboxWorldChanged --
 *
 *      This procedure is called when the world has changed in some
 *      way and the widget needs to recompute all its graphics contexts
 *	and determine its new geometry.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Listbox will be relayed out and redisplayed.
 *
 *---------------------------------------------------------------------------
 */
 
static void
RpListboxWorldChanged(instanceData)
    ClientData instanceData;	/* Information about widget. */
{
    XGCValues gcValues;
    GC gc;
    unsigned long mask;
    Listbox *listPtr;
    
    listPtr = (Listbox *) instanceData;

    if (listPtr->state & STATE_NORMAL) {
	gcValues.foreground = listPtr->fgColorPtr->pixel;
	gcValues.graphics_exposures = False;
	mask = GCForeground | GCFont | GCGraphicsExposures;
    } else {
	if (listPtr->dfgColorPtr != NULL) {
	    gcValues.foreground = listPtr->dfgColorPtr->pixel;
	    gcValues.graphics_exposures = False;
	    mask = GCForeground | GCFont | GCGraphicsExposures;
	} else {
	    gcValues.foreground = listPtr->fgColorPtr->pixel;
	    mask = GCForeground | GCFont;
	    if (listPtr->gray == None) {
		listPtr->gray = Tk_GetBitmap(NULL, listPtr->tkwin, "gray50");
	    }
	    if (listPtr->gray != None) {
		gcValues.fill_style = FillStippled;
		gcValues.stipple = listPtr->gray;
		mask |= GCFillStyle | GCStipple;
	    }
	}
    }

    gcValues.font = Tk_FontId(listPtr->tkfont);
    gc = Tk_GetGC(listPtr->tkwin, mask, &gcValues);
    if (listPtr->textGC != None) {
	Tk_FreeGC(listPtr->display, listPtr->textGC);
    }
    listPtr->textGC = gc;

    if (listPtr->selFgColorPtr != NULL) {
	gcValues.foreground = listPtr->selFgColorPtr->pixel;
    }
    gcValues.font = Tk_FontId(listPtr->tkfont);
    mask = GCForeground | GCFont;
    gc = Tk_GetGC(listPtr->tkwin, mask, &gcValues);
    if (listPtr->selTextGC != None) {
	Tk_FreeGC(listPtr->display, listPtr->selTextGC);
    }
    listPtr->selTextGC = gc;

    /*
     * Register the desired geometry for the window and arrange for
     * the window to be redisplayed.
     */

    RpListboxComputeGeometry(listPtr, 1, 1, 1);
    listPtr->flags |= UPDATE_V_SCROLLBAR|UPDATE_H_SCROLLBAR;
    RpEventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
}

/*
 *--------------------------------------------------------------
 *
 * RpDisplayListbox --
 *
 *	This procedure redraws the contents of a listbox window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

static void
RpDisplayListbox(clientData)
    ClientData clientData;	/* Information about window. */
{
    register Listbox *listPtr = (Listbox *) clientData;
    register Tk_Window tkwin = listPtr->tkwin;
    GC gc;
    int i, first, last, ncol, nrow, x0, y0, x, y, width, freeGC;
    int indent, imageWidth, imageHeight;
    Tk_FontMetrics fm;
    Tcl_Obj *curElement;
    char *stringRep;
    int stringLen;
    ItemAttr *attrs;
    Tk_3DBorder selectedBg;
    XGCValues gcValues;
    unsigned long mask;
    Pixmap pixmap;

    listPtr->flags &= ~REDRAW_PENDING;
    if (listPtr->flags & LISTBOX_DELETED) {
	return;
    }

    if (listPtr->flags & GEOMETRY_IS_STALE) {
	RpListboxComputeGeometry(listPtr, 0, 1, 0);
	listPtr->flags &= ~GEOMETRY_IS_STALE;
	listPtr->flags |= UPDATE_H_SCROLLBAR;
    }

    Tcl_Preserve((ClientData) listPtr);
    if (listPtr->flags & UPDATE_V_SCROLLBAR) {
	RpListboxUpdateVScrollbar(listPtr);
	if ((listPtr->flags & LISTBOX_DELETED) || !Tk_IsMapped(tkwin)) {
	    Tcl_Release((ClientData) listPtr);
	    return;
	}
    }
    if (listPtr->flags & UPDATE_H_SCROLLBAR) {
	RpListboxUpdateHScrollbar(listPtr);
	if ((listPtr->flags & LISTBOX_DELETED) || !Tk_IsMapped(tkwin)) {
	    Tcl_Release((ClientData) listPtr);
	    return;
	}
    }
    listPtr->flags &= ~(REDRAW_PENDING|UPDATE_V_SCROLLBAR|UPDATE_H_SCROLLBAR);
    Tcl_Release((ClientData) listPtr);

#ifndef TK_NO_DOUBLE_BUFFERING
    /*
     * Redrawing is done in a temporary pixmap that is allocated
     * here and freed at the end of the procedure.  All drawing is
     * done to the pixmap, and the pixmap is copied to the screen
     * at the end of the procedure.  This provides the smoothest
     * possible visual effects (no flashing on the screen).
     */

    pixmap = Tk_GetPixmap(listPtr->display, Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
#else
    pixmap = Tk_WindowId(tkwin);
#endif /* TK_NO_DOUBLE_BUFFERING */
    Tk_Fill3DRectangle(tkwin, pixmap, listPtr->normalBorder, 0, 0,
	    Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

    /* Display each item in the current view */
    first = RpNearestListboxElement(listPtr, listPtr->inset, listPtr->inset);
    last = RpNearestListboxElement(listPtr,
        Tk_Width(tkwin)-listPtr->inset, Tk_Height(tkwin)-listPtr->inset);

    for (i=first; i <= last; i++) {
        if (listPtr->elemsPerColumn == 0) {
            ncol = nrow = 0;
        } else {
            ncol = i / listPtr->elemsPerColumn;
            nrow = i % listPtr->elemsPerColumn;
        }
	x0 = listPtr->inset + ((ncol > 0) ? listPtr->xColumnMax[ncol-1] : 0)
                 - listPtr->xOffset;
	y0 = listPtr->inset + nrow*listPtr->lineHeight
                 - listPtr->yOffset;

        if (ncol == listPtr->numColumns-1) {
            width = Tk_Width(listPtr->tkwin) - listPtr->inset - x0;
        } else if (ncol > 0) {
            width = listPtr->xColumnMax[ncol] - listPtr->xColumnMax[ncol-1];
        } else {
            width = listPtr->xColumnMax[0];
        }

	gc = listPtr->textGC;
	freeGC = 0;

	/*
	 * Lookup this item in the item attributes table, to see if it has
	 * special foreground/background colors
	 */
        attrs = RpListboxGetItemAttributes((Tcl_Interp*)NULL, listPtr, i);

	/*
	 * If the listbox is enabled, items may be drawn differently;
	 * they may be drawn selected, or they may have special foreground
	 * or background colors.
	 */
	if (listPtr->state & STATE_NORMAL) {
            if (Tcl_FindHashEntry(listPtr->selection, (char *)i) != NULL) {
		/* Selected items are drawn differently. */
		gc = listPtr->selTextGC;
		selectedBg = listPtr->selBorder;
		
		/* If there is attribute information for this item,
		 * adjust the drawing accordingly */
		if (attrs) {
		    /* Default GC has the values from the widget at large */
		    if (listPtr->selFgColorPtr) {
			gcValues.foreground = listPtr->selFgColorPtr->pixel;
		    } else {
			gcValues.foreground = listPtr->fgColorPtr->pixel;
		    }
		    gcValues.font = Tk_FontId(listPtr->tkfont);
		    gcValues.graphics_exposures = False;
		    mask = GCForeground | GCFont | GCGraphicsExposures;
		    
		    if (attrs->selBorder != NULL) {
			selectedBg = attrs->selBorder;
		    }
		    
		    if (attrs->selFgColor != NULL) {
			gcValues.foreground = attrs->selFgColor->pixel;
			gc = Tk_GetGC(listPtr->tkwin, mask, &gcValues);
			freeGC = 1;
		    }
		}

                /*
                 * Don't bother joining the selection rectangles around
                 * multiple selected elements.  Just put a beveled
                 * rectangle around each one.
                 */
		Tk_Fill3DRectangle(tkwin, pixmap, selectedBg, x0, y0,
		    width, listPtr->lineHeight,
                    listPtr->selBorderWidth, TK_RELIEF_RAISED);
	    } else {
		/*
		 * If there is an item attributes record for this item, draw
		 * the background box and set the foreground color accordingly
		 */
		if (attrs) {
		    gcValues.foreground = listPtr->fgColorPtr->pixel;
		    gcValues.font = Tk_FontId(listPtr->tkfont);
		    gcValues.graphics_exposures = False;
		    mask = GCForeground | GCFont | GCGraphicsExposures;
		    
		    /*
		     * If the item has its own background color, draw it now.
		     */
		    
		    if (attrs->border != NULL) {
			Tk_Fill3DRectangle(tkwin, pixmap, attrs->border,
                            x0, y0, width, listPtr->lineHeight,
                            0, TK_RELIEF_FLAT);
		    }
		    
		    /*
		     * If the item has its own foreground, use it to override
		     * the value in the gcValues structure.
		     */
		    
		    if ((listPtr->state & STATE_NORMAL)
			    && attrs->fgColor != NULL) {
			gcValues.foreground = attrs->fgColor->pixel;
			gc = Tk_GetGC(listPtr->tkwin, mask, &gcValues);
			freeGC = 1;
		    }
		}
	    }
	}

	/* Find any extra indent */
        indent = 0;
        if (attrs && attrs->indent > 0) {
            indent = attrs->indent;
        }

	/* Draw any icon for the item */
	x = x0 + indent + listPtr->imageWidth + listPtr->selBorderWidth;
        if (attrs && attrs->image) {
            Tk_SizeOfImage(attrs->image, &imageWidth, &imageHeight);

            RpDisplayImage(pixmap, Tk_Width(tkwin), Tk_Height(tkwin),
                attrs->image,
                x-imageWidth-2, y0+(listPtr->lineHeight-imageHeight)/2);
        }

	/* Draw the actual text of this item */
	Tk_GetFontMetrics(listPtr->tkfont, &fm);
	y = y0 + (listPtr->lineHeight-fm.linespace)/2
               + fm.ascent + listPtr->selBorderWidth;

        stringRep = NULL;
	Tcl_ListObjIndex(listPtr->interp, listPtr->listObj, i, &curElement);
        if (curElement) {
	    stringRep = Tcl_GetStringFromObj(curElement, &stringLen);
	    Tk_DrawChars(listPtr->display, pixmap, gc, listPtr->tkfont,
		stringRep, stringLen, x+2, y);
        }

	/* If this is the active element, apply the activestyle to it. */
	if ((i == listPtr->active) && (listPtr->flags & GOT_FOCUS) && stringRep) {
	    if (listPtr->activeStyle == ACTIVE_STYLE_UNDERLINE) {
		/* Underline the text. */
		Tk_UnderlineChars(listPtr->display, pixmap, gc,
			listPtr->tkfont, stringRep, x+2, y, 0, stringLen);
	    } else if (listPtr->activeStyle == ACTIVE_STYLE_DOTBOX) {
#ifdef WIN32
		/*
		 * This provides for exact default look and feel on Windows.
		 */
		TkWinDCState state;
		HDC dc;
		RECT rect;

		dc = TkWinGetDrawableDC(listPtr->display, pixmap, &state);
		rect.left   = listPtr->inset;
		rect.top    = y0;
		rect.right  = rect.left + width;
		rect.bottom = rect.top + listPtr->lineHeight;
		DrawFocusRect(dc, &rect);
		TkWinReleaseDrawableDC(pixmap, dc, &state);
#else
		/*
		 * Draw a dotted box around the text.
		 */
		x = x0 + indent + listPtr->imageWidth;
		y = y0 + listPtr->lineHeight;

		gcValues.line_style  = LineOnOffDash;
		gcValues.line_width  = listPtr->selBorderWidth;
		if (gcValues.line_width <= 0) {
		    gcValues.line_width  = 1;
		}
		gcValues.dash_offset = 0;
		gcValues.dashes      = 1;
		/*
		 * You would think the XSetDashes was necessary, but it
		 * appears that the default dotting for just saying we
		 * want dashes appears to work correctly.
		 static char dashList[] = { 1 };
		 static int  dashLen    = sizeof(dashList);
		 XSetDashes(listPtr->display, gc, 0, dashList, dashLen);
		 */
		mask = GCLineWidth | GCLineStyle | GCDashList | GCDashOffset;
		XChangeGC(listPtr->display, gc, mask, &gcValues);
		XDrawRectangle(listPtr->display, pixmap, gc, x, y,
			(unsigned) width, (unsigned) listPtr->lineHeight - 1);
		if (!freeGC) {
		    /* Don't bother changing if it is about to be freed. */
		    gcValues.line_style = LineSolid;
		    XChangeGC(listPtr->display, gc, GCLineStyle, &gcValues);
		}
#endif
	    }
	}

	if (freeGC) {
	    Tk_FreeGC(listPtr->display, gc);
	}
    }

    /*
     * Redraw the border for the listbox to make sure that it's on top
     * of any of the text of the listbox entries.
     */

    Tk_Draw3DRectangle(tkwin, pixmap, listPtr->normalBorder,
	    listPtr->highlightWidth, listPtr->highlightWidth,
	    Tk_Width(tkwin) - 2*listPtr->highlightWidth,
	    Tk_Height(tkwin) - 2*listPtr->highlightWidth,
	    listPtr->borderWidth, listPtr->relief);
    if (listPtr->highlightWidth > 0) {
	GC fgGC, bgGC;

	bgGC = Tk_GCForColor(listPtr->highlightBgColorPtr, pixmap);
	if (listPtr->flags & GOT_FOCUS) {
	    fgGC = Tk_GCForColor(listPtr->highlightColorPtr, pixmap);
	    TkpDrawHighlightBorder(tkwin, fgGC, bgGC, 
	            listPtr->highlightWidth, pixmap);
	} else {
	    TkpDrawHighlightBorder(tkwin, bgGC, bgGC, 
	            listPtr->highlightWidth, pixmap);
	}
    }
#ifndef TK_NO_DOUBLE_BUFFERING
    XCopyArea(listPtr->display, pixmap, Tk_WindowId(tkwin),
	    listPtr->textGC, 0, 0, (unsigned) Tk_Width(tkwin),
	    (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(listPtr->display, pixmap);
#endif /* TK_NO_DOUBLE_BUFFERING */
}

/*
 *--------------------------------------------------------------
 *
 * RpDisplayImage --
 *
 *	This procedure draws a Tk image at the given (x,y)
 *	coordinate, being careful to clip the image if it
 *	falls off an edge.  If we're not careful, then Tk
 *	simply ignores the drawing command and the whole
 *	image disappears.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws the image into the pixmap.
 *
 *--------------------------------------------------------------
 */

static void
RpDisplayImage(pixmap, w, h, image, x, y)
    Pixmap pixmap;	/* Draw into this drawable */
    int w, h;           /* overall size of pixmap */
    Tk_Image image;	/* Draw this image into drawable */
    int x, y;		/* put upper-left corner at this (x,y) */
{
    int xoffs, yoffs, imageWidth, imageHeight;

    xoffs = yoffs = 0;
    Tk_SizeOfImage(image, &imageWidth, &imageHeight);

    if (x+imageWidth < 0 || x > w || y+imageHeight < 0 || y > h) {
        return;  /* completely off screen */
    }

    if (x < 0) {                  /* falling off on the left side */
        xoffs = -x;
        imageWidth -= xoffs;
        x = 0;
    }
    else if (x+imageWidth > w) {  /* falling off on the right side */
        imageWidth = w-x;
    }

    if (y < 0) {                  /* falling off on the top side */
        yoffs = -y;
        imageHeight -= yoffs;
        y = 0;
    }
    else if (y+imageHeight > h) { /* falling off on the bottom side */
        imageHeight = h-y;
    }

    Tk_RedrawImage(image, xoffs, yoffs, imageWidth, imageHeight, pixmap, x, y);
}


/*
 *----------------------------------------------------------------------
 *
 * RpListboxImageProc --
 *
 *	This procedure is invoked by the image code whenever the manager
 *	for an image does something that affects the size or contents
 *	of an image displayed in a listbox entry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the listbox to get redisplayed.
 *
 *----------------------------------------------------------------------
 */

static void
RpListboxImageProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;		/* Pointer to widget record. */
    int x, y;				/* Upper left pixel (within image)
					 * that must be redisplayed. */
    int width, height;			/* Dimensions of area to redisplay
					 * (may be <= 0). */
    int imgWidth, imgHeight;		/* New dimensions of image. */
{
    register Listbox *listPtr = (Listbox *) clientData;

    if (listPtr->tkwin != NULL) {
        RpListboxComputeGeometry(listPtr, 1, 1, 0);
	if (Tk_IsMapped(listPtr->tkwin) && !(listPtr->flags & REDRAW_PENDING)) {
            Tcl_DoWhenIdle(RpDisplayListbox, (ClientData) listPtr);
	    listPtr->flags |= REDRAW_PENDING;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxComputeGeometry --
 *
 *	This procedure is invoked to recompute geometry information
 *	such as the sizes of the elements and the overall dimensions
 *	desired for the listbox.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Geometry information is updated and a new requested size is
 *	registered for the widget.  Internal border and gridding
 *	information is also set.
 *
 *----------------------------------------------------------------------
 */

static void
RpListboxComputeGeometry(listPtr, fontChanged, maxIsStale, updateGrid)
    Listbox *listPtr;		/* Listbox whose geometry is to be
				 * recomputed. */
    int fontChanged;		/* Non-zero means the font may have changed
				 * so per-element width information also
				 * has to be computed. */
    int maxIsStale;		/* Non-zero means the "maxWidth" field may
				 * no longer be up-to-date and must
				 * be recomputed.  If fontChanged is 1 then
				 * this must be 1. */
    int updateGrid;		/* Non-zero means call Tk_SetGrid or
				 * Tk_UnsetGrid to update gridding for
				 * the window. */
{
    int width, height, pixelWidth, pixelHeight;
    int imageWidth, imageHeight, maxWidth, reqHeight, nrow, ncol;
    Tk_FontMetrics fm;
    Tcl_Obj *element;
    int textLength;
    char *text;
    int i, result;
    ItemAttr *attrPtr;

    if (listPtr->flags & LISTBOX_DELETED) {
        return;
    }

    Tk_GetFontMetrics(listPtr->tkfont, &fm);

    /*
     * Figure out max icon size and the overall height of each line
     */
    if (fontChanged || maxIsStale) {
	listPtr->xScrollUnit = Tk_TextWidth(listPtr->tkfont, "0", 1);
	if (listPtr->xScrollUnit == 0) {
	    listPtr->xScrollUnit = 1;
	}

	listPtr->imageWidth = 0;
	listPtr->imageHeight = 0;
	for (i = 0; i < listPtr->nElements; i++) {
	    /* Compute the pixel width of the current element */
	    result = Tcl_ListObjIndex(listPtr->interp, listPtr->listObj, i,
		    &element);
	    if (result != TCL_OK) {
		continue;
	    }
	    attrPtr = RpListboxGetItemAttributes(listPtr->interp, listPtr, i);

            /* compute the max image width/height */
            if (attrPtr && attrPtr->image) {
                Tk_SizeOfImage(attrPtr->image, &imageWidth, &imageHeight);
                imageWidth += 4;  /* add padding on either side */
                imageHeight += 4;
                if (imageWidth > listPtr->imageWidth) {
                    listPtr->imageWidth = imageWidth;
                }
                if (imageHeight > listPtr->imageHeight) {
                    listPtr->imageHeight = imageHeight;
                }
            }
	}
    }

    listPtr->lineHeight = fm.linespace + 1 + 2*listPtr->selBorderWidth;
    if (listPtr->imageHeight > listPtr->lineHeight) {
        listPtr->lineHeight = listPtr->imageHeight;
    }

    /*
     * Compute a new layout -- either vertical or horizontal
     */
    if (fontChanged  || maxIsStale) {
        if (listPtr->xColumnMax
              && listPtr->xColumnMax != listPtr->xColumnSpace) {
            ckfree((char*)listPtr->xColumnMax);
            listPtr->xColumnMax = NULL;
        }
        if (listPtr->orient == ORIENT_HORIZONTAL) {
            reqHeight = listPtr->height * listPtr->lineHeight;
            if (Tk_Height(listPtr->tkwin) > reqHeight) {
                reqHeight = Tk_Height(listPtr->tkwin);
            }
            listPtr->elemsPerColumn = reqHeight / listPtr->lineHeight;
            if (listPtr->elemsPerColumn == 0) {
                listPtr->elemsPerColumn = 1;
            }

            listPtr->numColumns = listPtr->nElements / listPtr->elemsPerColumn;
            if (listPtr->nElements % listPtr->elemsPerColumn != 0) {
                listPtr->numColumns++;
            }

            if (listPtr->numColumns <= 10) {
                listPtr->xColumnMax = listPtr->xColumnSpace;
            } else {
                listPtr->xColumnMax = (int*)ckalloc(listPtr->numColumns*sizeof(int));
            }
        } else {
            listPtr->elemsPerColumn = listPtr->nElements;
            listPtr->numColumns = 1;
            listPtr->xColumnMax = listPtr->xColumnSpace;
        }

	maxWidth = 0;
        nrow = ncol = 0;
	for (i = 0; i < listPtr->nElements; i++) {
	    /* Compute the pixel width of the current element */
	    result = Tcl_ListObjIndex(listPtr->interp, listPtr->listObj, i,
		    &element);
	    if (result != TCL_OK) {
		continue;
	    }
	    text = Tcl_GetStringFromObj(element, &textLength);
	    attrPtr = RpListboxGetItemAttributes(listPtr->interp, listPtr, i);

	    pixelWidth = Tk_TextWidth(listPtr->tkfont, text, textLength);
            if (attrPtr && attrPtr->indent > 0) {
                pixelWidth += attrPtr->indent;
            }
	    if (pixelWidth > maxWidth) {
		maxWidth = pixelWidth;
	    }

            if (++nrow >= listPtr->elemsPerColumn) {
                listPtr->xColumnMax[ncol] =
                    ((ncol > 0) ? listPtr->xColumnMax[ncol-1] : 0)
                    + maxWidth + listPtr->imageWidth + 10;

                maxWidth = 0;
                nrow = 0; ncol++;
            }
	}

        if (nrow > 0) {
            /* finalize last column if it was in progress */
            listPtr->xColumnMax[ncol] =
                ((ncol > 0) ? listPtr->xColumnMax[ncol-1] : 0)
                + maxWidth + listPtr->imageWidth + 4;
        }

        listPtr->maxWidth = listPtr->xColumnMax[listPtr->numColumns-1];

        /*
         * If the scroll offsets are outside the view, reset them.
         */
        if (listPtr->xOffset > 0.8*listPtr->xOffset) {
            listPtr->xOffset = 0;
            RpEventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
        }
        if (listPtr->orient == ORIENT_HORIZONTAL) {
            listPtr->yOffset = 0;
        } else {
            height = listPtr->nElements*listPtr->lineHeight
                       - Tk_Height(listPtr->tkwin) + 2*listPtr->inset;
            if (height < 0) {
                height = 0;
            }
            if (listPtr->yOffset > height) {
                listPtr->yOffset = height;
            }
        }
    }

    /*
     * Make sure that topIndex is still at top of column.
     */
    if (listPtr->elemsPerColumn == 0) {
        ncol = 0;
    } else {
        ncol = listPtr->topIndex / listPtr->elemsPerColumn;
    }
    listPtr->topIndex = ncol*listPtr->elemsPerColumn;

    /*
     * Snap to nearest scroll increment and request geometry size.
     */
    width = listPtr->width;
    if (width <= 0) {
	width = (listPtr->maxWidth + listPtr->xScrollUnit - 1)
		/listPtr->xScrollUnit;
	if (width < 1) {
	    width = 1;
	}
    }
    pixelWidth = width*listPtr->xScrollUnit + 2*listPtr->inset
	    + 2*listPtr->selBorderWidth;

    height = listPtr->height;
    if (listPtr->height <= 0) {
	height = listPtr->elemsPerColumn;
	if (height < 1) {
	    height = 1;
	}
    }
    pixelHeight = height*listPtr->lineHeight + 2*listPtr->inset;

    Tk_GeometryRequest(listPtr->tkwin, pixelWidth, pixelHeight);
    Tk_SetInternalBorder(listPtr->tkwin, listPtr->inset);
    if (updateGrid) {
	if (listPtr->setGrid) {
	    Tk_SetGrid(listPtr->tkwin, width, height, listPtr->xScrollUnit,
		    listPtr->lineHeight);
	} else {
	    Tk_UnsetGrid(listPtr->tkwin);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxInsertSubCmd --
 *
 *	This procedure is invoked to handle the listbox "insert"
 *      subcommand.  It's a little different from the usual Listbox
 *	insert.  It takes a single value and all configuration options
 *	for that one value.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	New elements are added to the listbox pointed to by listPtr;
 *      a refresh callback is registered for the listbox.
 *
 *----------------------------------------------------------------------
 */

static int
RpListboxInsertSubCmd(listPtr, index, objc, objv)
    register Listbox *listPtr;	/* Listbox that is to get the new
				 * elements. */
    int index;			/* Add the new elements before this
				 * element. */
    int objc;			/* Number of new elements to add. */
    Tcl_Obj *CONST objv[];	/* New elements (one per entry). */
{
    Tcl_Obj *newListObj;
    int result;
    ItemAttr *attrPtr;
    
    /* Adjust selection and attribute information beyond the current index */
    RpMigrateHashEntries(listPtr->selection, index, listPtr->nElements-1, 1);
    RpMigrateHashEntries(listPtr->itemAttrTable, index, listPtr->nElements-1, 1);
    
    /* If the object is shared, duplicate it before writing to it */
    if (Tcl_IsShared(listPtr->listObj)) {
        newListObj = Tcl_DuplicateObj(listPtr->listObj);
    } else {
        newListObj = listPtr->listObj;
    }

    /* insert the specified element (first arg to command) */
    result = Tcl_ListObjReplace(listPtr->interp, newListObj,
        index, 0, 1, objv);

    if (result != TCL_OK) {
	return result;
    }

    /*
     * Replace the current object and set attached listvar, if any.
     * This may error if listvar points to a var in a deleted namespace, but
     * we ignore those errors.  If the namespace is recreated, it will
     * auto-sync with the current value. [Bug 1424513]
     */
    Tcl_IncrRefCount(newListObj);
    Tcl_DecrRefCount(listPtr->listObj);
    listPtr->listObj = newListObj;
    listPtr->nElements++;

    /*
     * Update the "special" indices (anchor, topIndex, active) to account
     * for the renumbering that just occurred.  Then arrange for the new
     * information to be displayed.
     */
    listPtr->active = listPtr->nElements-1;
    if (index <= listPtr->selectAnchor) {
	listPtr->selectAnchor += 1;
    }
    if (index < listPtr->topIndex) {
	listPtr->topIndex += 1;
    }
    if (index <= listPtr->active) {
	listPtr->active += 1;
	if ((listPtr->active >= listPtr->nElements) &&
		(listPtr->nElements > 0)) {
	    listPtr->active = listPtr->nElements-1;
	}
    }
    listPtr->flags |= UPDATE_V_SCROLLBAR | UPDATE_H_SCROLLBAR;
    RpListboxComputeGeometry(listPtr, 0, 1, 0);
    RpEventuallyRedrawRange(listPtr, index, listPtr->nElements-1);

    /* apply any additional options as configuration options */
    if (objc > 1) {
        attrPtr = RpListboxGetItemAttributes(listPtr->interp, listPtr, index);
        result = RpConfigureListboxItem(listPtr->interp, listPtr, attrPtr,
            objc-1, objv+1, index);

        if (result != TCL_OK) {
	    return result;
        }
    }

    /* return the index of the new item */
    Tcl_SetObjResult(listPtr->interp, Tcl_NewIntObj(index));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxDeleteSubCmd --
 *
 *	Process a listbox "delete" subcommand by removing one or more
 *      elements from a listbox widget.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	The listbox will be modified and (eventually) redisplayed.
 *
 *----------------------------------------------------------------------
 */

static int
RpListboxDeleteSubCmd(listPtr, first, last)
    register Listbox *listPtr;	/* Listbox widget to modify. */
    int first;			/* Index of first element to delete. */
    int last;			/* Index of last element to delete. */
{
    int count, i, pageLines;
    Tcl_Obj *newListObj;
    int result;
    Tcl_HashEntry *entry;
    
    /*
     * Adjust the range to fit within the existing elements of the
     * listbox, and make sure there's something to delete.
     */

    if (first < 0) {
	first = 0;
    }
    if (last >= listPtr->nElements) {
	last = listPtr->nElements-1;
    }
    count = last + 1 - first;
    if (count <= 0) {
	return TCL_OK;
    }

    /*
     * Foreach deleted index we must:
     * a) remove selection information
     * b) clean up attributes
     */
    for (i = first; i <= last; i++) {
	/* Remove selection information */
	entry = Tcl_FindHashEntry(listPtr->selection, (char *)i);
	if (entry != NULL) {
	    listPtr->numSelected--;
	    Tcl_DeleteHashEntry(entry);
	}

	entry = Tcl_FindHashEntry(listPtr->itemAttrTable, (char *)i);
	if (entry != NULL) {
	    ckfree((char *)Tcl_GetHashValue(entry));
	    Tcl_DeleteHashEntry(entry);
	}
    }

    /* Adjust selection and attribute info for indices after lastIndex */
    RpMigrateHashEntries(listPtr->selection, last+1,
	    listPtr->nElements-1, count*-1);
    RpMigrateHashEntries(listPtr->itemAttrTable, last+1,
	    listPtr->nElements-1, count*-1);

    /* Delete the requested elements */
    if (Tcl_IsShared(listPtr->listObj)) {
	newListObj = Tcl_DuplicateObj(listPtr->listObj);
    } else {
	newListObj = listPtr->listObj;
    }
    result = Tcl_ListObjReplace(listPtr->interp,
	    newListObj, first, count, 0, NULL);
    if (result != TCL_OK) {
	return result;
    }

    /*
     * Replace the current object and set attached listvar, if any.
     * This may error if listvar points to a var in a deleted namespace, but
     * we ignore those errors.  If the namespace is recreated, it will
     * auto-sync with the current value. [Bug 1424513]
     */

    Tcl_IncrRefCount(newListObj);
    Tcl_DecrRefCount(listPtr->listObj);
    listPtr->listObj = newListObj;

    /* Get the new list length */
    Tcl_ListObjLength(listPtr->interp, listPtr->listObj, &listPtr->nElements);

    /*
     * Update the selection and viewing information to reflect the change
     * in the element numbering, and redisplay to slide information up over
     * the elements that were deleted.
     */

    if (first <= listPtr->selectAnchor) {
	listPtr->selectAnchor -= count;
	if (listPtr->selectAnchor < first) {
	    listPtr->selectAnchor = first;
	}
    }
    if (first <= listPtr->topIndex) {
	listPtr->topIndex -= count;
	if (listPtr->topIndex < first) {
	    listPtr->topIndex = first;
	}
    }
    if (listPtr->orient == ORIENT_VERTICAL) {
        pageLines = Tk_Height(listPtr->tkwin)/listPtr->lineHeight;
        if (listPtr->topIndex > (listPtr->nElements - pageLines)) {
	    listPtr->topIndex = listPtr->nElements - pageLines;
	    if (listPtr->topIndex < 0) {
	        listPtr->topIndex = 0;
	    }
        }
    }
    if (listPtr->active > last) {
	listPtr->active -= count;
    } else if (listPtr->active >= first) {
	listPtr->active = first;
	if ((listPtr->active >= listPtr->nElements) &&
		(listPtr->nElements > 0)) {
	    listPtr->active = listPtr->nElements-1;
	}
    }
    listPtr->flags |= UPDATE_V_SCROLLBAR|UPDATE_H_SCROLLBAR;
    RpListboxComputeGeometry(listPtr, 0, 1, 0);
    RpEventuallyRedrawRange(listPtr, first, listPtr->nElements-1);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * RpListboxEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on listboxes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
RpListboxEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    Listbox *listPtr = (Listbox *) clientData;
    int height;
    
    if (eventPtr->type == Expose) {
	RpEventuallyRedrawRange(listPtr,
		RpNearestListboxElement(listPtr,
                    eventPtr->xexpose.x, eventPtr->xexpose.y),
		RpNearestListboxElement(listPtr,
                    eventPtr->xexpose.x + eventPtr->xexpose.width,
                    eventPtr->xexpose.y + eventPtr->xexpose.height));
    } else if (eventPtr->type == DestroyNotify) {
	if (!(listPtr->flags & LISTBOX_DELETED)) {
	    listPtr->flags |= LISTBOX_DELETED;
	    Tcl_DeleteCommandFromToken(listPtr->interp, listPtr->widgetCmd);
	    if (listPtr->setGrid) {
		Tk_UnsetGrid(listPtr->tkwin);
	    }
	    if (listPtr->flags & REDRAW_PENDING) {
		Tcl_CancelIdleCall(RpDisplayListbox, clientData);
	    }
	    Tcl_EventuallyFree(clientData, RpDestroyListbox);
	}
    } else if (eventPtr->type == ConfigureNotify) {
        if (listPtr->orient == ORIENT_HORIZONTAL) {
            /* size can change layout in horizontal mode */
            RpListboxComputeGeometry(listPtr, 0, 1, 0);
        } else {
            height = listPtr->nElements*listPtr->lineHeight
                       - Tk_Height(listPtr->tkwin) + 2*listPtr->inset;
            if (height < 0) {
                height = 0;
            }
            if (listPtr->yOffset > height) {
                listPtr->yOffset = height;
            }
        }
	listPtr->flags |= UPDATE_V_SCROLLBAR|UPDATE_H_SCROLLBAR;
	RpChangeListboxView(listPtr, listPtr->topIndex);
	RpChangeListboxXOffset(listPtr, listPtr->xOffset);

	/*
	 * Redraw the whole listbox.  It's hard to tell what needs
	 * to be redrawn (e.g. if the listbox has shrunk then we
	 * may only need to redraw the borders), so just redraw
	 * everything for safety.
	 */

	RpEventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    listPtr->flags |= GOT_FOCUS;
	    RpEventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    listPtr->flags &= ~GOT_FOCUS;
	    RpEventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
RpListboxCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    Listbox *listPtr = (Listbox *) clientData;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (!(listPtr->flags & LISTBOX_DELETED)) {
	Tk_DestroyWindow(listPtr->tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * RpGetListboxIndex --
 *
 *	Parse an index into a listbox and return either its value
 *	or an error.
 *
 * Results:
 *	A standard Tcl result.  If all went well, then *indexPtr is
 *	filled in with the index (into listPtr) corresponding to
 *	string.  Otherwise an error message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
RpGetListboxIndex(interp, listPtr, indexObj, endIsSize, indexPtr)
    Tcl_Interp *interp;		/* For error messages. */
    Listbox *listPtr;		/* Listbox for which the index is being
				 * specified. */
    Tcl_Obj *indexObj;		/* Specifies an element in the listbox. */
    int endIsSize;		/* If 1, "end" refers to the number of
				 * entries in the listbox.  If 0, "end"
				 * refers to 1 less than the number of
				 * entries. */
    int *indexPtr;		/* Where to store converted index. */
{
    int result;
    int index;
    char *stringRep;
    
    /* First see if the index is one of the named indices */
    result = Tcl_GetIndexFromObj(NULL, indexObj, indexNames, "", 0, &index);
    if (result == TCL_OK) {
	switch (index) {
	    case INDEX_ACTIVE: {
		/* "active" index */
		*indexPtr = listPtr->active;
		break;
	    }

	    case INDEX_ANCHOR: {
		/* "anchor" index */
		*indexPtr = listPtr->selectAnchor;
		break;
	    }

	    case INDEX_END: {
		/* "end" index */
		if (endIsSize) {
		    *indexPtr = listPtr->nElements;
		} else {
		    *indexPtr = listPtr->nElements - 1;
		}
		break;
	    }
	}
	return TCL_OK;
    }

    /* The index didn't match any of the named indices; maybe it's an @x,y */
    stringRep = Tcl_GetString(indexObj);
    if (stringRep[0] == '@') {
	/* @x,y index */
	int x, y;
	char *start, *end;
	start = stringRep + 1;
	x = strtol(start, &end, 0);
	if ((start == end) || (*end != ',')) {
	    Tcl_AppendResult(interp, "bad listbox index \"", stringRep,
		    "\": must be active, anchor, end, @x,y, or a number",
		    (char *)NULL);
	    return TCL_ERROR;
	}
	start = end+1;
	y = strtol(start, &end, 0);
	if ((start == end) || (*end != '\0')) {
	    Tcl_AppendResult(interp, "bad listbox index \"", stringRep,
		    "\": must be active, anchor, end, @x,y, or a number",
		    (char *)NULL);
	    return TCL_ERROR;
	}
	*indexPtr = RpNearestListboxElement(listPtr, x, y);
	return TCL_OK;
    }
    
    /* Maybe the index is just an integer */
    if (Tcl_GetIntFromObj(interp, indexObj, indexPtr) == TCL_OK) {
	return TCL_OK;
    }

    /* Everything failed, nothing matched.  Throw up an error message */
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "bad listbox index \"",
	    Tcl_GetString(indexObj), "\": must be active, anchor, ",
	    "end, @x,y, or a number", (char *) NULL);
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * RpGetListboxPos --
 *
 *	Computes the row/col index for the given listbox element.
 *
 * Results:
 *	Returns the row/col values in the pointers provided.
 *
 * Side effects:
 *	Computes geometry/layout if it is stale.
 *
 *--------------------------------------------------------------
 */

static void
RpGetListboxPos(listPtr, index, rowPtr, colPtr)
    Listbox *listPtr;		/* Listbox for which the index is being
				 * specified. */
    int index;			/* Specifies an element in the listbox. */
    int *rowPtr;		/* Returns: row in listbox layout */
    int *colPtr;		/* Returns: col in listbox layout */
{
    if (listPtr->flags & GEOMETRY_IS_STALE) {
        RpListboxComputeGeometry(listPtr, 0, 1, 0);
        listPtr->flags &= ~GEOMETRY_IS_STALE;
    }

    if (listPtr->elemsPerColumn == 0) {
        *rowPtr = *colPtr = 0;
    } else {
        *colPtr = index / listPtr->elemsPerColumn;
        *rowPtr = index % listPtr->elemsPerColumn;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RpChangeListboxView --
 *
 *	Change the view on a listbox widget so that a given element
 *	is displayed at the top/left.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	What's displayed on the screen is changed.  If there is a
 *	scrollbar associated with this widget, then the scrollbar
 *	is instructed to change its display too.
 *
 *----------------------------------------------------------------------
 */

static void
RpChangeListboxView(listPtr, index)
    register Listbox *listPtr;		/* Information about widget. */
    int index;				/* Index of element in listPtr
					 * that should now appear at the
					 * top of the listbox. */
{
    int nrow, ncol, xOffset, yOffset;

    /* keep the view in bounds */
    if (index > listPtr->nElements) {
        index = listPtr->nElements;
    }
    if (index < 0) {
        index = 0;
    }

    RpGetListboxPos(listPtr, index, &nrow, &ncol);

    xOffset = (ncol > 0) ? listPtr->xColumnMax[ncol-1] : 0;

    if (listPtr->orient == ORIENT_VERTICAL) {
        yOffset = nrow*listPtr->lineHeight;
    } else {
        yOffset = 0;
    }

    RpChangeListboxXOffset(listPtr, xOffset);
    RpChangeListboxYOffset(listPtr, yOffset);

    /* topIndex is top of column */
    listPtr->topIndex = ncol*listPtr->elemsPerColumn;
}

/*
 *----------------------------------------------------------------------
 *
 * RpChangeListboxXOffset --
 *
 *	Change the horizontal offset for a listbox.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The listbox may be redrawn to reflect its new horizontal
 *	offset.
 *
 *----------------------------------------------------------------------
 */

static void
RpChangeListboxXOffset(listPtr, offset)
    register Listbox *listPtr;		/* Information about widget. */
    int offset;				/* Desired new "xOffset" for
					 * listbox. */
{
    int windowWidth;
    
    windowWidth = Tk_Width(listPtr->tkwin) - 2*listPtr->inset - 10;
    if (offset > listPtr->maxWidth-windowWidth) {
	offset = listPtr->maxWidth-windowWidth;
    }
    if (offset < 0) {
	offset = 0;
    }
    if (offset != listPtr->xOffset) {
	listPtr->xOffset = offset;
	listPtr->flags |= UPDATE_H_SCROLLBAR;
	RpEventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RpChangeListboxYOffset --
 *
 *	Change the vertical offset for a listbox.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The listbox may be redrawn to reflect its new vertical
 *	offset.
 *
 *----------------------------------------------------------------------
 */

static void
RpChangeListboxYOffset(listPtr, offset)
    register Listbox *listPtr;		/* Information about widget. */
    int offset;				/* Desired new "yOffset" for
					 * listbox. */
{
    int windowHeight;
    
    windowHeight = Tk_Height(listPtr->tkwin) - 2*listPtr->inset;
    if (offset > listPtr->elemsPerColumn*listPtr->lineHeight - windowHeight) {
	offset = listPtr->elemsPerColumn*listPtr->lineHeight - windowHeight;
    }
    if (offset < 0) {
	offset = 0;
    }
    if (offset != listPtr->yOffset) {
	listPtr->yOffset = offset;
	listPtr->flags |= UPDATE_V_SCROLLBAR;
	RpEventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxScanTo --
 *
 *	Given a point (presumably of the curent mouse location)
 *	drag the view in the window to implement the scan operation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The view in the window may change.
 *
 *----------------------------------------------------------------------
 */

static void
RpListboxScanTo(listPtr, x, y)
    register Listbox *listPtr;		/* Information about widget. */
    int x;				/* X-coordinate to use for scan
					 * operation. */
    int y;				/* Y-coordinate to use for scan
					 * operation. */
{
    int ncol, newx, newy, maxx, maxy;

    ncol = listPtr->numColumns;
    maxx = (ncol > 0) ? listPtr->xColumnMax[ncol] : listPtr->xColumnMax[0];
    maxy = listPtr->elemsPerColumn*listPtr->lineHeight;

    /*
     * Compute new top line for screen by amplifying the difference
     * between the current position and the place where the scan
     * started (the "mark" position).  If we run off the top or bottom
     * of the list, then reset the mark point so that the current
     * position continues to correspond to the edge of the window.
     * This means that the picture will start dragging as soon as the
     * mouse reverses direction (without this reset, might have to slide
     * mouse a long ways back before the picture starts moving again).
     */
    newx = listPtr->scanMarkXOffset - (10*(x - listPtr->scanMarkX));
    if (newx > maxx) {
	newx = listPtr->scanMarkXOffset = maxx;
	listPtr->scanMarkX = x;
    } else if (newx < 0) {
	newx = listPtr->scanMarkXOffset = 0;
	listPtr->scanMarkX = x;
    }
    RpChangeListboxXOffset(listPtr, newx);

    newy = listPtr->scanMarkYOffset - (10*(y - listPtr->scanMarkY));
    if (newy > maxy) {
	newy = listPtr->scanMarkYOffset = maxy;
	listPtr->scanMarkY = y;
    } else if (newy < 0) {
	newy = listPtr->scanMarkYOffset = 0;
	listPtr->scanMarkY = y;
    }
    RpChangeListboxYOffset(listPtr, newy);
}

/*
 *----------------------------------------------------------------------
 *
 * RpNearestListboxElement --
 *
 *	Given an (x,y) coordinate inside a listbox, compute the index
 *	of the element under that point or closest to it.
 *
 * Results:
 *	The return value is an index of an element of listPtr.  If
 *	listPtr has no elements, then 0 is always returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
RpNearestListboxElement(listPtr, x, y)
    register Listbox *listPtr;		/* Information about widget. */
    int x;				/* X-coordinate in listPtr's window. */
    int y;				/* Y-coordinate in listPtr's window. */
{
    int index, ncol, x0, x1, colIndex;

    /* need to update geometry/layout to figure out where point sits */
    if (listPtr->flags & GEOMETRY_IS_STALE) {
        RpListboxComputeGeometry(listPtr, 1, 1, 1);
        listPtr->flags &= ~GEOMETRY_IS_STALE;
        listPtr->flags |= UPDATE_H_SCROLLBAR;
    }

    /* if we're scrolled over, adjust the x/y coordinates */
    x += listPtr->xOffset;
    y += listPtr->yOffset;

    /* find the column containing this point */
    if (x > listPtr->maxWidth) {
        ncol = listPtr->numColumns-1;
    } else {
        for (ncol=0; ncol < listPtr->numColumns; ncol++) {
            x0 = (ncol > 0) ? listPtr->xColumnMax[ncol-1]+1 : 0;
            x1 = listPtr->xColumnMax[ncol];

            if (x >= x0 && x <= x1) {
                break;
            }
        }
    }
    colIndex = ncol*listPtr->elemsPerColumn;

    index = (y - listPtr->inset)/listPtr->lineHeight;
    if (index >= listPtr->elemsPerColumn) {
	index = listPtr->elemsPerColumn;
    }
    if (index < 0) {
	index = 0;
    }
    index += colIndex;
    if (index >= listPtr->nElements) {
	index = listPtr->nElements-1;
    }
    return index;
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxSelect --
 *
 *	Select or deselect one or more elements in a listbox..
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	All of the elements in the range between first and last are
 *	marked as either selected or deselected, depending on the
 *	"select" argument.  Any items whose state changes are redisplayed.
 *	The selection is claimed from X when the number of selected
 *	elements changes from zero to non-zero.
 *
 *----------------------------------------------------------------------
 */

static int
RpListboxSelect(listPtr, first, last, select)
    register Listbox *listPtr;		/* Information about widget. */
    int first;				/* Index of first element to
					 * select or deselect. */
    int last;				/* Index of last element to
					 * select or deselect. */
    int select;				/* 1 means select items, 0 means
					 * deselect them. */
{
    int i, firstRedisplay, oldCount;
    Tcl_HashEntry *entry;
    int new;
    
    if (last < first) {
	i = first;
	first = last;
	last = i;
    }
    if ((last < 0) || (first >= listPtr->nElements)) {
	return TCL_OK;
    }
    if (first < 0) {
	first = 0;
    }
    if (last >= listPtr->nElements) {
	last = listPtr->nElements - 1;
    }
    oldCount = listPtr->numSelected;
    firstRedisplay = -1;

    /*
     * For each index in the range, find it in our selection hash table.
     * If it's not there but should be, add it.  If it's there but shouldn't
     * be, remove it.
     */
    for (i = first; i <= last; i++) {
	entry = Tcl_FindHashEntry(listPtr->selection, (char *)i);
	if (entry != NULL) {
	    if (!select) {
		Tcl_DeleteHashEntry(entry);
		listPtr->numSelected--;
		if (firstRedisplay < 0) {
		    firstRedisplay = i;
		}
	    }
	} else {
	    if (select) {
		entry = Tcl_CreateHashEntry(listPtr->selection,
			(char *)i, &new);
		Tcl_SetHashValue(entry, (ClientData) NULL);
		listPtr->numSelected++;
		if (firstRedisplay < 0) {
		    firstRedisplay = i;
		}
	    }
	}
    }

    if (firstRedisplay >= 0) {
	RpEventuallyRedrawRange(listPtr, first, last);
    }
    if ((oldCount == 0) && (listPtr->numSelected > 0)
	    && (listPtr->exportSelection)) {
	Tk_OwnSelection(listPtr->tkwin, XA_PRIMARY, RpListboxLostSelection,
		(ClientData) listPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxFetchSelection --
 *
 *	This procedure is called back by Tk when the selection is
 *	requested by someone.  It returns part or all of the selection
 *	in a buffer provided by the caller.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored
 *	at buffer.  Buffer is filled (or partially filled) with a
 *	NULL-terminated string containing part or all of the selection,
 *	as given by offset and maxBytes.  The selection is returned
 *	as a Tcl list with one list element for each element in the
 *	listbox.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
RpListboxFetchSelection(clientData, offset, buffer, maxBytes)
    ClientData clientData;		/* Information about listbox widget. */
    int offset;				/* Offset within selection of first
					 * byte to be returned. */
    char *buffer;			/* Location in which to place
					 * selection. */
    int maxBytes;			/* Maximum number of bytes to place
					 * at buffer, not including terminating
					 * NULL character. */
{
    register Listbox *listPtr = (Listbox *) clientData;
    Tcl_DString selection;
    int length, count, needNewline;
    Tcl_Obj *curElement;
    char *stringRep;
    int stringLen;
    Tcl_HashEntry *entry;
    int i;
    
    if (!listPtr->exportSelection) {
	return -1;
    }

    /*
     * Use a dynamic string to accumulate the contents of the selection.
     */

    needNewline = 0;
    Tcl_DStringInit(&selection);
    for (i = 0; i < listPtr->nElements; i++) {
	entry = Tcl_FindHashEntry(listPtr->selection, (char *)i);
	if (entry != NULL) {
	    if (needNewline) {
		Tcl_DStringAppend(&selection, "\n", 1);
	    }
	    Tcl_ListObjIndex(listPtr->interp, listPtr->listObj, i,
		    &curElement);
	    stringRep = Tcl_GetStringFromObj(curElement, &stringLen);
	    Tcl_DStringAppend(&selection, stringRep, stringLen);
	    needNewline = 1;
	}
    }

    length = Tcl_DStringLength(&selection);
    if (length == 0) {
	return -1;
    }

    /*
     * Copy the requested portion of the selection to the buffer.
     */

    count = length - offset;
    if (count <= 0) {
	count = 0;
    } else {
	if (count > maxBytes) {
	    count = maxBytes;
	}
	memcpy((VOID *) buffer,
		(VOID *) (Tcl_DStringValue(&selection) + offset),
		(size_t) count);
    }
    buffer[count] = '\0';
    Tcl_DStringFree(&selection);
    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxLostSelection --
 *
 *	This procedure is called back by Tk when the selection is
 *	grabbed away from a listbox widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The existing selection is unhighlighted, and the window is
 *	marked as not containing a selection.
 *
 *----------------------------------------------------------------------
 */

static void
RpListboxLostSelection(clientData)
    ClientData clientData;		/* Information about listbox widget. */
{
    register Listbox *listPtr = (Listbox *) clientData;
    
    if ((listPtr->exportSelection) && (listPtr->nElements > 0)) {
	RpListboxSelect(listPtr, 0, listPtr->nElements-1, 0);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * RpEventuallyRedrawRange --
 *
 *	Ensure that a given range of elements is eventually redrawn on
 *	the display (if those elements in fact appear on the display).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets redisplayed.
 *
 *----------------------------------------------------------------------
 */

static void
RpEventuallyRedrawRange(listPtr, first, last)
    register Listbox *listPtr;		/* Information about widget. */
    int first;				/* Index of first element in list
					 * that needs to be redrawn. */
    int last;				/* Index of last element in list
					 * that needs to be redrawn.  May
					 * be less than first;
					 * these just bracket a range. */
{
    /* We don't have to register a redraw callback if one is already pending,
     * or if the window doesn't exist, or if the window isn't mapped */
    if ((listPtr->flags & REDRAW_PENDING)
	    || (listPtr->flags & LISTBOX_DELETED)
	    || !Tk_IsMapped(listPtr->tkwin)) {
	return;
    }
    listPtr->flags |= REDRAW_PENDING;
    Tcl_DoWhenIdle(RpDisplayListbox, (ClientData) listPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxUpdateVScrollbar --
 *
 *	This procedure is invoked whenever information has changed in
 *	a listbox in a way that would invalidate a vertical scrollbar
 *	display.  If there is an associated scrollbar, then this command
 *	updates it by invoking a Tcl command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl command is invoked, and an additional command may be
 *	invoked to process errors in the command.
 *
 *----------------------------------------------------------------------
 */

static void
RpListboxUpdateVScrollbar(listPtr)
    register Listbox *listPtr;		/* Information about widget. */
{
    char string[TCL_DOUBLE_SPACE * 2];
    double first, last, worldHeight;
    int result;
    Tcl_Interp *interp;
    
    if (listPtr->yScrollCmd == NULL) {
	return;
    }
    if (listPtr->nElements == 0) {
	first = 0.0;
	last = 1.0;
    } else {
        /* need to update geometry/layout to figure out scrollbar bubble */
        if (listPtr->flags & GEOMETRY_IS_STALE) {
            RpListboxComputeGeometry(listPtr, 0, 1, 0);
            listPtr->flags &= ~GEOMETRY_IS_STALE;
        }
        worldHeight = listPtr->elemsPerColumn*listPtr->lineHeight;

	first = listPtr->yOffset/((double)worldHeight);
	last  = (listPtr->yOffset+Tk_Height(listPtr->tkwin))
                  / ((double)worldHeight);
	if (last > 1.0) {
	    last = 1.0;
	}
    }
    sprintf(string, " %g %g", first, last);

    /*
     * We must hold onto the interpreter from the listPtr because the data
     * at listPtr might be freed as a result of the Tcl_VarEval.
     */
    
    interp = listPtr->interp;
    Tcl_Preserve((ClientData) interp);
    result = Tcl_VarEval(interp, listPtr->yScrollCmd, string,
	    (char *) NULL);
    if (result != TCL_OK) {
	Tcl_AddErrorInfo(interp,
		"\n    (vertical scrolling command executed by listbox)");
	Tcl_BackgroundError(interp);
    }
    Tcl_Release((ClientData) interp);
}

/*
 *----------------------------------------------------------------------
 *
 * RpListboxUpdateHScrollbar --
 *
 *	This procedure is invoked whenever information has changed in
 *	a listbox in a way that would invalidate a horizontal scrollbar
 *	display.  If there is an associated horizontal scrollbar, then
 *	this command updates it by invoking a Tcl command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl command is invoked, and an additional command may be
 *	invoked to process errors in the command.
 *
 *----------------------------------------------------------------------
 */

static void
RpListboxUpdateHScrollbar(listPtr)
    register Listbox *listPtr;		/* Information about widget. */
{
    char string[TCL_DOUBLE_SPACE * 2];
    int result, windowWidth;
    double first, last;
    Tcl_Interp *interp;

    if (listPtr->xScrollCmd == NULL) {
	return;
    }
    windowWidth = Tk_Width(listPtr->tkwin) - 2*listPtr->inset;

    if (listPtr->maxWidth < 5) {
	first = 0;
	last = 1.0;
    } else {
	first = listPtr->xOffset/((double)listPtr->maxWidth);
	last = (listPtr->xOffset + windowWidth)
		/((double)listPtr->maxWidth);
	if (last > 1.0) {
	    last = 1.0;
	}
    }
    sprintf(string, " %g %g", first, last);

    /*
     * We must hold onto the interpreter because the data referred to at
     * listPtr might be freed as a result of the call to Tcl_VarEval.
     */
    
    interp = listPtr->interp;
    Tcl_Preserve((ClientData) interp);
    result = Tcl_VarEval(interp, listPtr->xScrollCmd, string,
	    (char *) NULL);
    if (result != TCL_OK) {
	Tcl_AddErrorInfo(interp,
		"\n    (horizontal scrolling command executed by listbox)");
	Tcl_BackgroundError(interp);
    }
    Tcl_Release((ClientData) interp);
}

/*
 *----------------------------------------------------------------------
 *
 * RpMigrateHashEntries --
 *
 *	Given a hash table with entries keyed by a single integer value,
 *	move all entries in a given range by a fixed amount, so that
 *	if in the original table there was an entry with key n and
 *	the offset was i, in the new table that entry would have key n + i.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Rekeys some hash table entries.
 *
 *----------------------------------------------------------------------
 */

static void
RpMigrateHashEntries(table, first, last, offset)
    Tcl_HashTable *table;
    int first;
    int last;
    int offset;
{
    int i, new;
    Tcl_HashEntry *entry;
    ClientData clientData;

    if (offset == 0) {
	return;
    }
    /* It's more efficient to do one if/else and nest the for loops inside,
     * although we could avoid some code duplication if we nested the if/else
     * inside the for loops */
    if (offset > 0) {
	for (i = last; i >= first; i--) {
	    entry = Tcl_FindHashEntry(table, (char *)i);
	    if (entry != NULL) {
		clientData = Tcl_GetHashValue(entry);
		Tcl_DeleteHashEntry(entry);
		entry = Tcl_CreateHashEntry(table, (char *)(i + offset), &new);
		Tcl_SetHashValue(entry, clientData);
	    }
	}
    } else {
	for (i = first; i <= last; i++) {
	    entry = Tcl_FindHashEntry(table, (char *)i);
	    if (entry != NULL) {
		clientData = Tcl_GetHashValue(entry);
		Tcl_DeleteHashEntry(entry);
		entry = Tcl_CreateHashEntry(table, (char *)(i + offset), &new);
		Tcl_SetHashValue(entry, clientData);
	    }
	}
    }
    return;
}
