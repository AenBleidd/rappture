/*
 * ----------------------------------------------------------------------
 *  RpCanvPlacard - canvas item with text and background box
 *
 *  This canvas item makes it easy to create a box with text inside.
 *  The box is normally stretched around the text, but can be given
 *  a max size and causing text to be clipped.
 *
 *    .c create placard <x> <y> -anchor <nsew> \
 *        -text <text>         << text to be displayed
 *        -font <name>         << font used for text
 *        -imageleft <image>   << image displayed on the left of text
 *        -imageright <image>  << image displayed on the right of text
 *        -maxwidth <size>     << maximum size
 *        -textcolor <color>   << text color
 *        -background <color>  << fill for rect behind text
 *        -outline <color>     << outline around text
 *        -borderwidth <size>  << for 3D border
 *        -relief <value>      << for 3D border (drawn under -outline)
 *        -tags <taglist>      << usual Tk canvas tags
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <string.h>
#include <math.h>
#include "tk.h"

/* used for text within the placard that needs to be truncated */
#define ellipsis_width 9
#define ellipsis_height 1
static unsigned char ellipsis_bits[] = {
   0x92, 0x00};

/*
 * Record for each placard item:
 */
typedef struct PlacardItem  {
    Tk_Item header;             /* Generic stuff that's the same for all
                                 * types.  MUST BE FIRST IN STRUCTURE. */
    Tcl_Interp *interp;         /* Interp that owns this item */
    Tk_Window tkwin;            /* Window that represents this canvas */
    Tk_Canvas canvas;           /* Canvas that owns this item */
    Pixmap dots;                /* ellipsis used for truncated text */

    /*
     * Fields that are set by widget commands other than "configure":
     */
    double x, y;                /* Positioning point for text */

    /*
     * Configuration settings that are updated by Tk_ConfigureWidget:
     */
    char *text;                 /* text to be displayed */
    Tk_Anchor anchor;           /* Where to anchor text relative to (x,y) */
    XColor *textColor;          /* Color for text */
    XColor *lineColor;          /* Color for outline of rectangle */
    Tk_3DBorder bgBorder;       /* used for drawing background rectangle */
    Tk_Font tkfont;             /* Font for drawing text */
    int width;                  /* Fixed width for placard (0 = not set) */
    int maxWidth;               /* Maximum overall width for placard */
    int borderWidth;            /* supports 3D border (drawn under -outline) */
    int relief;                 /* indicates whether placard as a whole is
                                 * raised, sunken, or flat */
    int padding;                /* Padding around text and images */

    char *imageLeftString;      /* Name of image displayed on the left */
    char *imageRightString;     /* Name of image displayed on the right */
  
    /*
     * Fields whose values are derived from the current values of the
     * configuration settings above:
     */
    int textNumChars;           /* Length of text in characters. */
    int textNumBytes;           /* Length of text in bytes. */
    int textDrawChars;          /* Same as textNumChars or shorter, if
                                 * text is too long for -maxwidth */
    int textWidth;              /* Overall width of text (perhaps truncated)
                                 * in pixels */
    Tk_TextLayout textLayout;   /* Cached text layout information for value */

    Tk_Image imageLeft;         /* icon drawn to left of text */
    int imageLeftW;             /* width of image (if image is not NULL) */
    int imageLeftH;             /* height of image (if image is not NULL) */

    Tk_Image imageRight;        /* icon drawn to right of text */
    int imageRightW;            /* width of image (if image is not NULL) */
    int imageRightH;            /* height of image (if image is not NULL) */

    int leftEdge;               /* Pixel location of the left edge of the
                                 * item. (Where the left border of the
                                 * text layout is drawn.) */
    int rightEdge;              /* Pixel just to right of right edge of
                                 * area of text item. */
    GC gcText;                  /* Graphics context for drawing text. */
    GC gcLineRect;              /* Graphics context for rectangle outline. */
} PlacardItem;

/*
 * Information used for parsing configuration specs:
 */
static Tk_CustomOption tagsOption = {
    (Tk_OptionParseProc *) Tk_CanvasTagsParseProc,
    Tk_CanvasTagsPrintProc, (ClientData) NULL
};

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_ANCHOR, "-anchor", (char*)NULL, (char*)NULL,
        "center", Tk_Offset(PlacardItem, anchor),
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_BORDER, "-background", (char*)NULL, (char*)NULL,
        "", Tk_Offset(PlacardItem, bgBorder), TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-borderwidth", (char*)NULL, (char*)NULL,
        "0", Tk_Offset(PlacardItem, borderWidth), 0},
    {TK_CONFIG_FONT, "-font", (char*)NULL, (char*)NULL,
        "helvetica -12", Tk_Offset(PlacardItem, tkfont), 0},
    {TK_CONFIG_COLOR, "-foreground", (char*)NULL, (char*)NULL,
        "black", Tk_Offset(PlacardItem, textColor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-imageleft", (char*)NULL, (char*)NULL,
        (char*)NULL, Tk_Offset(PlacardItem, imageLeftString),
        TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-imageright", (char*)NULL, (char*)NULL,
        (char*)NULL, Tk_Offset(PlacardItem, imageRightString),
        TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-maxwidth", (char*)NULL, (char*)NULL,
        "0", Tk_Offset(PlacardItem, maxWidth), 0},
    {TK_CONFIG_COLOR, "-outline", (char*)NULL, (char*)NULL,
        "", Tk_Offset(PlacardItem, lineColor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-padding", (char*)NULL, (char*)NULL,
        "4", Tk_Offset(PlacardItem, padding), 0},
    {TK_CONFIG_RELIEF, "-relief", (char*)NULL, (char*)NULL,
        "flat", Tk_Offset(PlacardItem, relief), 0},
    {TK_CONFIG_CUSTOM, "-tags", (char*)NULL, (char*)NULL,
        (char*)NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_STRING, "-text", (char*)NULL, (char*)NULL,
        "", Tk_Offset(PlacardItem, text), 0},
    {TK_CONFIG_PIXELS, "-width", (char*)NULL, (char*)NULL,
        "0", Tk_Offset(PlacardItem, width), 0},
    {TK_CONFIG_END, (char*)NULL, (char*)NULL, (char*)NULL,
        (char*)NULL, 0, 0}
};

/*
 * Prototypes for procedures defined in this file
 */
static void ComputePlacardBbox _ANSI_ARGS_((Tk_Canvas canvas,
    PlacardItem *plPtr));
static int ConfigurePlacard _ANSI_ARGS_((Tcl_Interp *interp,
    Tk_Canvas canvas, Tk_Item *itemPtr, int argc,
    Tcl_Obj *CONST argv[], int flags));
static int CreatePlacard _ANSI_ARGS_((Tcl_Interp *interp,
    Tk_Canvas canvas, struct Tk_Item *itemPtr,
    int argc, Tcl_Obj *CONST argv[]));
static void DeletePlacard _ANSI_ARGS_((Tk_Canvas canvas,
    Tk_Item *itemPtr, Display *display));
static void DisplayCanvPlacard _ANSI_ARGS_((Tk_Canvas canvas,
    Tk_Item *itemPtr, Display *display, Drawable dst,
    int x, int y, int width, int height));
static void ScalePlacard _ANSI_ARGS_((Tk_Canvas canvas,
    Tk_Item *itemPtr, double originX, double originY,
    double scaleX, double scaleY));
static void TranslatePlacard _ANSI_ARGS_((Tk_Canvas canvas,
    Tk_Item *itemPtr, double deltaX, double deltaY));
static int PlacardCoords _ANSI_ARGS_((Tcl_Interp *interp,
    Tk_Canvas canvas, Tk_Item *itemPtr,
    int argc, Tcl_Obj *CONST argv[]));
static int PlacardToArea _ANSI_ARGS_((Tk_Canvas canvas,
    Tk_Item *itemPtr, double *rectPtr));
static double PlacardToPoint _ANSI_ARGS_((Tk_Canvas canvas,
    Tk_Item *itemPtr, double *pointPtr));
static int PlacardToPostscript _ANSI_ARGS_((Tcl_Interp *interp,
    Tk_Canvas canvas, Tk_Item *itemPtr, int prepass));

static void ImageLeftChangedProc _ANSI_ARGS_((ClientData clientData,
    int x, int y, int width, int height, int imgWidth,
    int imgHeight));
static void ImageRightChangedProc _ANSI_ARGS_((ClientData clientData,
    int x, int y, int width, int height, int imgWidth,
    int imgHeight));

/*
 * The structures below defines the canvas item type:
 */
Tk_ItemType rpPlacardType = {
    "placard",                          /* name */
    sizeof(PlacardItem),                /* itemSize */
    CreatePlacard,                      /* createProc */
    configSpecs,                        /* configSpecs */
    ConfigurePlacard,                   /* configureProc */
    PlacardCoords,                      /* coordProc */
    DeletePlacard,                      /* deleteProc */
    DisplayCanvPlacard,                 /* displayProc */
    TK_CONFIG_OBJS,                     /* flags */
    PlacardToPoint,                     /* pointProc */
    PlacardToArea,                      /* areaProc */
    PlacardToPostscript,                /* postscriptProc */
    ScalePlacard,                       /* scaleProc */
    TranslatePlacard,                   /* translateProc */
    (Tk_ItemIndexProc *) NULL,          /* indexProc */
    (Tk_ItemCursorProc *) NULL,         /* icursorProc */
    (Tk_ItemSelectionProc *) NULL,      /* selectionProc */
    (Tk_ItemInsertProc *) NULL,         /* insertProc */
    (Tk_ItemDCharsProc *) NULL,         /* dTextProc */
    (Tk_ItemType *) NULL,               /* nextPtr */
};

/*
 * The record below describes a canvas widget.  It is made available
 * to the item procedures so they can access certain shared fields such
 * as the overall displacement and scale factor for the canvas.
 */

typedef struct TkCanvas {
    Tk_Window tkwin;		/* Window that embodies the canvas.  NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up.*/
    Display *display;		/* Display containing widget;  needed, among
				 * other things, to release resources after
				 * tkwin has already gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with canvas. */
    Tcl_Command widgetCmd;	/* Token for canvas's widget command. */
    Tk_Item *firstItemPtr;	/* First in list of all items in canvas,
				 * or NULL if canvas empty. */
    Tk_Item *lastItemPtr;	/* Last in list of all items in canvas,
				 * or NULL if canvas empty. */

    /*
     * Information used when displaying widget:
     */

    int borderWidth;		/* Width of 3-D border around window. */
    Tk_3DBorder bgBorder;	/* Used for canvas background. */
    int relief;			/* Indicates whether window as a whole is
				 * raised, sunken, or flat. */
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
    GC pixmapGC;		/* Used to copy bits from a pixmap to the
				 * screen and also to clear the pixmap. */
    int width, height;		/* Dimensions to request for canvas window,
				 * specified in pixels. */
    int redrawX1, redrawY1;	/* Upper left corner of area to redraw,
				 * in pixel coordinates.  Border pixels
				 * are included.  Only valid if
				 * REDRAW_PENDING flag is set. */
    int redrawX2, redrawY2;	/* Lower right corner of area to redraw,
				 * in integer canvas coordinates.  Border
				 * pixels will *not* be redrawn. */
    int confine;		/* Non-zero means constrain view to keep
				 * as much of canvas visible as possible. */

    /*
     * Information used to manage the selection and insertion cursor:
     */

    Tk_CanvasTextInfo textInfo; /* Contains lots of fields;  see tk.h for
				 * details.  This structure is shared with
				 * the code that implements individual items. */
    int insertOnTime;		/* Number of milliseconds cursor should spend
				 * in "on" state for each blink. */
    int insertOffTime;		/* Number of milliseconds cursor should spend
				 * in "off" state for each blink. */
    Tcl_TimerToken insertBlinkHandler;
				/* Timer handler used to blink cursor on and
				 * off. */

    /*
     * Transformation applied to canvas as a whole:  to compute screen
     * coordinates (X,Y) from canvas coordinates (x,y), do the following:
     *
     * X = x - xOrigin;
     * Y = y - yOrigin;
     */

    int xOrigin, yOrigin;	/* Canvas coordinates corresponding to
				 * upper-left corner of window, given in
				 * canvas pixel units. */
    int drawableXOrigin, drawableYOrigin;
				/* During redisplay, these fields give the
				 * canvas coordinates corresponding to
				 * the upper-left corner of the drawable
				 * where items are actually being drawn
				 * (typically a pixmap smaller than the
				 * whole window). */

    /*
     * Information used for event bindings associated with items.
     */

    Tk_BindingTable bindingTable;
				/* Table of all bindings currently defined
				 * for this canvas.  NULL means that no
				 * bindings exist, so the table hasn't been
				 * created.  Each "object" used for this
				 * table is either a Tk_Uid for a tag or
				 * the address of an item named by id. */
    Tk_Item *currentItemPtr;	/* The item currently containing the mouse
				 * pointer, or NULL if none. */
    Tk_Item *newCurrentPtr;	/* The item that is about to become the
				 * current one, or NULL.  This field is
				 * used to detect deletions  of the new
				 * current item pointer that occur during
				 * Leave processing of the previous current
				 * item.  */
    double closeEnough;		/* The mouse is assumed to be inside an
				 * item if it is this close to it. */
    XEvent pickEvent;		/* The event upon which the current choice
				 * of currentItem is based.  Must be saved
				 * so that if the currentItem is deleted,
				 * can pick another. */
    int state;			/* Last known modifier state.  Used to
				 * defer picking a new current object
				 * while buttons are down. */

    /*
     * Information used for managing scrollbars:
     */

    char *xScrollCmd;		/* Command prefix for communicating with
				 * horizontal scrollbar.  NULL means no
				 * horizontal scrollbar.  Malloc'ed*/
    char *yScrollCmd;		/* Command prefix for communicating with
				 * vertical scrollbar.  NULL means no
				 * vertical scrollbar.  Malloc'ed*/
    int scrollX1, scrollY1, scrollX2, scrollY2;
				/* These four coordinates define the region
				 * that is the 100% area for scrolling (i.e.
				 * these numbers determine the size and
				 * location of the sliders on scrollbars).
				 * Units are pixels in canvas coords. */
    char *regionString;		/* The option string from which scrollX1
				 * etc. are derived.  Malloc'ed. */
    int xScrollIncrement;	/* If >0, defines a grid for horizontal
				 * scrolling.  This is the size of the "unit",
				 * and the left edge of the screen will always
				 * lie on an even unit boundary. */
    int yScrollIncrement;	/* If >0, defines a grid for horizontal
				 * scrolling.  This is the size of the "unit",
				 * and the left edge of the screen will always
				 * lie on an even unit boundary. */

    /*
     * Information used for scanning:
     */

    int scanX;			/* X-position at which scan started (e.g.
				 * button was pressed here). */
    int scanXOrigin;		/* Value of xOrigin field when scan started. */
    int scanY;			/* Y-position at which scan started (e.g.
				 * button was pressed here). */
    int scanYOrigin;		/* Value of yOrigin field when scan started. */

    /*
     * Information used to speed up searches by remembering the last item
     * created or found with an item id search.
     */

    Tk_Item *hotPtr;		/* Pointer to "hot" item (one that's been
				 * recently used.  NULL means there's no
				 * hot item. */
    Tk_Item *hotPrevPtr;	/* Pointer to predecessor to hotPtr (NULL
				 * means item is first in list).  This is
				 * only a hint and may not really be hotPtr's
				 * predecessor. */

    /*
     * Miscellaneous information:
     */

    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */
    double pixelsPerMM;		/* Scale factor between MM and pixels;
				 * used when converting coordinates. */
    int flags;			/* Various flags;  see below for
				 * definitions. */
    int nextId;			/* Number to use as id for next item
				 * created in widget. */
    Tk_PostscriptInfo psInfo;
				/* Pointer to information used for generating
				 * Postscript for the canvas.  NULL means
				 * no Postscript is currently being
				 * generated. */
    Tcl_HashTable idTable;	/* Table of integer indices. */
    /*
     * Additional information, added by the 'dash'-patch
     */
    VOID *reserved1;
    Tk_State canvas_state;	/* state of canvas */
    VOID *reserved2;
    VOID *reserved3;
    Tk_TSOffset tsoffset;
#ifndef USE_OLD_TAG_SEARCH
    void *bindTagExprs; /* linked list of tag expressions used in bindings */
#endif
} TkCanvas;


/*
 * ------------------------------------------------------------------------
 *  RpCanvPlacard_Init --
 *
 *  Invoked when the Rappture GUI library is being initialized
 *  to install the "placard" item on the Tk canvas widget.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpCanvPlacard_Init(interp)
    Tcl_Interp *interp;         /* interpreter being initialized */
{
    Tk_CreateItemType(&rpPlacardType);

    Tk_DefineBitmap(interp, Tk_GetUid("rp_ellipsis"),
        (char*)ellipsis_bits, ellipsis_width, ellipsis_height);

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  CreatePlacard --
 *
 *    This procedure is invoked to create a new placard item
 *    in a canvas.  A placard is a text item with a box behind it.
 *
 *  Results:
 *    A standard Tcl return name.  If an error occurred in creating
 *    the item then an error message is left in the interp's result.
 *    In this case itemPtr is left uninitialized so it can be safely
 *    freed by the caller.
 *
 *  Side effects:
 *    A new item is created.
 * ------------------------------------------------------------------------
 */
static int
CreatePlacard(interp, canvas, itemPtr, argc, argv)
    Tcl_Interp *interp;           /* interpreter for error reporting */
    Tk_Canvas canvas;             /* canvas to hold new item */
    Tk_Item *itemPtr;             /* record to hold new item; header has been
                                   * initialized by caller */
    int argc;                     /* number of arguments in argv */
    Tcl_Obj *CONST argv[];        /* arguments describing item */
{
    PlacardItem *plPtr = (PlacardItem *) itemPtr;
    int i;

    if (argc == 1) {
        i = 1;
    } else {
        char *arg = Tcl_GetStringFromObj(argv[1], NULL);
        if ((argc > 1) && (arg[0] == '-')
                && (arg[1] >= 'a') && (arg[1] <= 'z')) {
            i = 1;
        } else {
            i = 2;
        }
    }

    if (argc < i) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            Tk_PathName(Tk_CanvasTkwin(canvas)), " create ",
            itemPtr->typePtr->name, " x y ?options?\"", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * Carry out initialization that is needed in order to clean up after
     * errors during the the remainder of this procedure.
     */
    plPtr->interp           = interp;
    plPtr->tkwin            = Tk_CanvasTkwin(canvas);
    plPtr->canvas           = canvas;
    plPtr->dots             = Tk_GetBitmap(interp, plPtr->tkwin,
                                  Tk_GetUid("rp_ellipsis"));
    plPtr->text             = NULL;
    plPtr->anchor           = TK_ANCHOR_CENTER;
    plPtr->textColor        = NULL;
    plPtr->bgBorder         = NULL;
    plPtr->lineColor        = NULL;
    plPtr->tkfont           = NULL;
    plPtr->width            = 0;
    plPtr->maxWidth         = 0;
    plPtr->borderWidth      = 0;
    plPtr->relief           = TK_RELIEF_FLAT;
    plPtr->padding          = 0;
    plPtr->imageLeftString  = NULL;
    plPtr->imageRightString = NULL;
    plPtr->imageLeft        = NULL;
    plPtr->imageRight       = NULL;
    plPtr->textNumChars     = 0;
    plPtr->textNumBytes     = 0;
    plPtr->textDrawChars    = 0;
    plPtr->textWidth        = 0;
    plPtr->textLayout       = NULL;
    plPtr->leftEdge         = 0;
    plPtr->rightEdge        = 0;
    plPtr->gcText           = None;
    plPtr->gcLineRect       = None;

    /*
     * Process the arguments to fill in the item record.
     */
    if ((PlacardCoords(interp, canvas, itemPtr, i, argv) != TCL_OK)) {
        goto error;
    }
    if (ConfigurePlacard(interp, canvas, itemPtr,
            argc-i, argv+i, 0) == TCL_OK) {

        return TCL_OK;
    }

error:
    DeletePlacard(canvas, itemPtr, Tk_Display(plPtr->tkwin));
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  PlacardCoords --
 *
 *    This procedure is invoked to process the "coords" widget command on
 *    placard items.  See the user documentation for details on what
 *    it does.
 *
 *  Results:
 *    Returns TCL_OK or TCL_ERROR, and sets the interp's result.
 *
 *  Side effects:
 *    The coordinates for the given item may be changed.
 * ------------------------------------------------------------------------
 */
static int
PlacardCoords(interp, canvas, itemPtr, argc, argv)
    Tcl_Interp *interp;     /* Used for error reporting. */
    Tk_Canvas canvas;       /* Canvas containing item. */
    Tk_Item *itemPtr;       /* Item whose coordinates are to be read or
                             * modified. */
    int argc;               /* Number of coordinates supplied in argv. */
    Tcl_Obj *CONST argv[];  /* Array of coordinates: x1, y1, x2, y2, ... */
{
    PlacardItem *plPtr = (PlacardItem *) itemPtr;
    char buf[64 + TCL_INTEGER_SPACE];

    if (argc == 0) {
        Tcl_Obj *obj = Tcl_NewObj();
        Tcl_Obj *subobj = Tcl_NewDoubleObj(plPtr->x);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        subobj = Tcl_NewDoubleObj(plPtr->y);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        Tcl_SetObjResult(interp, obj);
    }
    else if (argc < 3) {
        if (argc == 1) {
            if (Tcl_ListObjGetElements(interp, argv[0], &argc,
                    (Tcl_Obj ***) &argv) != TCL_OK) {
                return TCL_ERROR;
            } else if (argc != 2) {

                sprintf(buf, "wrong # coordinates: expected 2, got %d", argc);
                Tcl_SetResult(interp, buf, TCL_VOLATILE);
                return TCL_ERROR;
            }
        }
        if ((Tk_CanvasGetCoordFromObj(interp, canvas, argv[0],
                   &plPtr->x) != TCL_OK)
                || (Tk_CanvasGetCoordFromObj(interp, canvas, argv[1],
                          &plPtr->y) != TCL_OK)) {
            return TCL_ERROR;
        }
        ComputePlacardBbox(canvas, plPtr);
    } else {
        sprintf(buf, "wrong # coordinates: expected 0 or 2, got %d", argc);
        Tcl_SetResult(interp, buf, TCL_VOLATILE);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ConfigurePlacard --
 *
 *    This procedure is invoked to configure various aspects of a
 *    placard item, such as its color and font.
 *
 *  Results:
 *    A standard Tcl result code.  If an error occurs, then an error
 *    message is left in the interp's result.
 *
 *  Side effects:
 *    Configuration information, such as colors and GCs, may be set
 *    for itemPtr.
 * ------------------------------------------------------------------------
 */
static int
ConfigurePlacard(interp, canvas, itemPtr, argc, argv, flags)
    Tcl_Interp *interp;           /* Interpreter for error reporting. */
    Tk_Canvas canvas;             /* Canvas containing itemPtr. */
    Tk_Item *itemPtr;             /* Rectangle item to reconfigure. */
    int argc;                     /* Number of elements in argv.  */
    Tcl_Obj *CONST argv[];        /* Args describing things to configure. */
    int flags;                    /* Flags to pass to Tk_ConfigureWidget. */
{
    PlacardItem *plPtr = (PlacardItem *) itemPtr;
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    Tk_Window tkwin;
    XColor *color;
    Tk_Image image;

    tkwin = Tk_CanvasTkwin(canvas);
    if (Tk_ConfigureWidget(interp, tkwin, configSpecs,
            argc, (CONST char **) argv,
            (char*)plPtr, flags|TK_CONFIG_OBJS) != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * A few of the options require additional processing, such as
     * graphics contexts.
     */
    color = plPtr->textColor;
    newGC = None;
    if (plPtr->tkfont != NULL) {
        gcValues.font = Tk_FontId(plPtr->tkfont);
        mask = GCFont;
        if (color != NULL) {
            gcValues.foreground = color->pixel;
            mask |= GCForeground;
            newGC = Tk_GetGC(tkwin, mask, &gcValues);
        }
    }
    if (plPtr->gcText != None) {
        Tk_FreeGC(Tk_Display(tkwin), plPtr->gcText);
    }
    plPtr->gcText = newGC;


    newGC = None;
    if (plPtr->lineColor != NULL) {
        gcValues.foreground = plPtr->lineColor->pixel;
        gcValues.line_width = 1;
        mask = GCForeground | GCLineWidth;
        newGC = Tk_GetGC(tkwin, mask, &gcValues);
    }
    if (plPtr->gcLineRect != None) {
        Tk_FreeGC(Tk_Display(tkwin), plPtr->gcLineRect);
    }
    plPtr->gcLineRect = newGC;


    if (plPtr->imageLeftString != NULL) {
        image = Tk_GetImage(interp, tkwin, plPtr->imageLeftString,
                ImageLeftChangedProc, (ClientData)plPtr);
        if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (plPtr->imageLeft != NULL) {
        Tk_FreeImage(plPtr->imageLeft);
    }
    plPtr->imageLeft = image;

    if (image != NULL) {
        Tk_SizeOfImage(image,&plPtr->imageLeftW, &plPtr->imageLeftH);
    }


    if (plPtr->imageRightString != NULL) {
        image = Tk_GetImage(interp, tkwin, plPtr->imageRightString,
                ImageRightChangedProc, (ClientData)plPtr);
        if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (plPtr->imageRight != NULL) {
        Tk_FreeImage(plPtr->imageRight);
    }
    plPtr->imageRight = image;

    if (image != NULL) {
        Tk_SizeOfImage(image,&plPtr->imageRightW, &plPtr->imageRightH);
    }


    if (plPtr->text != NULL) {
        plPtr->textNumBytes = strlen(plPtr->text);
        plPtr->textNumChars = Tcl_NumUtfChars(plPtr->text, plPtr->textNumBytes);
    };

    ComputePlacardBbox(canvas, plPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  DeletePlacard --
 *
 *    This procedure is called to clean up the data structure
 *    associated with a placard item.
 *
 *  Results:
 *    None.
 *
 *  Side effects:
 *    Resources associated with itemPtr are released.
 * ------------------------------------------------------------------------
 */
static void
DeletePlacard(canvas, itemPtr, display)
    Tk_Canvas canvas;          /* Info about overall canvas widget. */
    Tk_Item *itemPtr;          /* Item that is being deleted. */
    Display *display;          /* Display containing window for canvas. */
{
    PlacardItem *plPtr = (PlacardItem *) itemPtr;

    if (plPtr->textColor != NULL) {
        Tk_FreeColor(plPtr->textColor);
    }
    if (plPtr->lineColor != NULL) {
        Tk_FreeColor(plPtr->lineColor);
    }
    if (plPtr->text != NULL) {
        ckfree(plPtr->text);
    }


    if (plPtr->imageLeftString != NULL) {
        ckfree(plPtr->imageLeftString);
    }
    if (plPtr->imageLeft != NULL ) {
        Tk_FreeImage(plPtr->imageLeft);
    }
    if (plPtr->imageRightString != NULL) {
        ckfree(plPtr->imageRightString);
    }
    if (plPtr->imageRight != NULL ) {
        Tk_FreeImage(plPtr->imageRight);
    }

    Tk_FreeTextLayout(plPtr->textLayout);

    Tk_FreeFont(plPtr->tkfont);

    if (plPtr->gcText != None) {
        Tk_FreeGC(display, plPtr->gcText);
    }
    if (plPtr->gcLineRect != None) {
        Tk_FreeGC(display, plPtr->gcLineRect);
    }
}

/*
 * ------------------------------------------------------------------------
 *  ComputePlacardBbox --
 *
 *    This procedure is invoked to compute the bounding box of
 *    all the pixels that may be drawn as part of a text item.
 *    In addition, it recomputes all of the geometry information
 *    used to display a text item or check for mouse hits.
 *
 *  Results:
 *    None.
 *
 *  Side effects:
 *    The fields x1, y1, x2, and y2 are updated in the header
 *    for itemPtr, and the linePtr structure is regenerated
 *    for itemPtr.
 * ------------------------------------------------------------------------
 */
static void
ComputePlacardBbox(canvas, plPtr)
    Tk_Canvas canvas;           /* Canvas that contains item. */
    PlacardItem *plPtr;       /* Item whose bbox is to be recomputed. */
{
    int leftX, topY, width, height, maxwidth, ellw, ellh;

    /*
     * Get the layout for the placard text.
     */
    plPtr->textDrawChars = plPtr->textNumChars;

    Tk_FreeTextLayout(plPtr->textLayout);
    plPtr->textLayout = Tk_ComputeTextLayout(plPtr->tkfont,
            plPtr->text, plPtr->textNumChars, -1,
            TK_JUSTIFY_LEFT, 0, &width, &height);

    plPtr->textWidth = width;

    if (plPtr->textColor == NULL) {
        width = height = 0;
    } else {
        width += 2*plPtr->padding;
        height += 2*plPtr->padding;
    }

    /*
     * If the current value has an image, add on its width.
     */
    if (plPtr->imageLeft) {
        width += plPtr->imageLeftW;
        if (plPtr->textWidth > 0) {
            width += plPtr->padding;
        }
        if (plPtr->imageLeftH > height) {
            height = plPtr->imageLeftH;
        }
    }
    if (plPtr->imageRight) {
        width += plPtr->imageRightW;
        if (plPtr->textWidth > 0) {
            width += plPtr->padding;
        }
        if (plPtr->imageRightH > height) {
            height = plPtr->imageRightH;
        }
    }

    /*
     * Add on extra size for the 3D border (if there is one).
     */
    if (plPtr->borderWidth > 0) {
        width += 2*plPtr->borderWidth;
        height += 2*plPtr->borderWidth;
    }

    /*
     * If the overall width exceeds the -maxwidth, then we must
     * truncate the text to make it fit.
     */
    maxwidth = 0;
    if (plPtr->maxWidth > 0 && plPtr->width > 0) {
        maxwidth = (plPtr->width < plPtr->maxWidth) ? plPtr->width : plPtr->maxWidth;
    } else if (plPtr->maxWidth > 0) {
        maxwidth = plPtr->maxWidth;
    } else if (plPtr->width > 0) {
        maxwidth = plPtr->width;
    }

    if (maxwidth > 0 && width > maxwidth) {
        Tk_SizeOfBitmap(Tk_Display(plPtr->tkwin), plPtr->dots, &ellw, &ellh);
        width = maxwidth - 2*plPtr->padding - 2*plPtr->borderWidth - ellw;
        if (plPtr->imageLeft) {
            width -= plPtr->imageLeftW + plPtr->padding;
        }
        if (plPtr->imageRight) {
            width -= plPtr->imageRightW + plPtr->padding;
        }
        if (width < 0) { width = 0; }

        plPtr->textDrawChars = Tk_MeasureChars(plPtr->tkfont,
            plPtr->text, plPtr->textNumChars, width, /*flags*/ 0,
            &plPtr->textWidth);

        /* recompute layout for truncated text */
        Tk_FreeTextLayout(plPtr->textLayout);
        plPtr->textLayout = Tk_ComputeTextLayout(plPtr->tkfont,
                plPtr->text, plPtr->textDrawChars, -1,
                TK_JUSTIFY_LEFT, 0, NULL, NULL);

        width = maxwidth;  /* fit everything in this width */
    }

    /*
     * If there's a fixed -width, then use that instead.
     */
    if (plPtr->width > 0) {
        width = plPtr->width;
    }

    /*
     * Use overall geometry information to compute the top-left corner
     * of the bounding box for the text item.
     */
    leftX = (int) (plPtr->x + 0.5);
    topY = (int) (plPtr->y + 0.5);
    switch (plPtr->anchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_N:
        case TK_ANCHOR_NE:
            break;

        case TK_ANCHOR_W:
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_E:
            topY -= height / 2;
            break;

        case TK_ANCHOR_SW:
        case TK_ANCHOR_S:
        case TK_ANCHOR_SE:
            topY -= height;
            break;
    }
    switch (plPtr->anchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_W:
        case TK_ANCHOR_SW:
            break;

        case TK_ANCHOR_N:
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_S:
            leftX -= width / 2;
            break;

        case TK_ANCHOR_NE:
        case TK_ANCHOR_E:
        case TK_ANCHOR_SE:
            leftX -= width;
            break;
    }

    plPtr->leftEdge  = leftX;
    plPtr->rightEdge = leftX + width;

    /*
     * Last of all, update the bounding box for the item.
     */
    plPtr->header.x1 = leftX;
    plPtr->header.y1 = topY;
    plPtr->header.x2 = leftX + width;
    plPtr->header.y2 = topY + height;
}

/*
 * ------------------------------------------------------------------------
 *  DisplayCanvPlacard --
 *
 *    This procedure is invoked to draw a text item in a given drawable.
 *
 *  Results:
 *    None.
 *
 *  Side effects:
 *    ItemPtr is drawn in drawable using the transformation information
 *    in canvas.
 * ------------------------------------------------------------------------
 */
static void
DisplayCanvPlacard(canvas, itemPtr, display, drawable, x, y, width, height)
    Tk_Canvas canvas;           /* Canvas that contains item. */
    Tk_Item *itemPtr;           /* Item to be displayed. */
    Display *display;           /* Display on which to draw item. */
    Drawable drawable;          /* Pixmap or window in which to draw item. */
    int x, y, width, height;    /* Describes region of canvas that must be
                                 * redisplayed (not used). */
{
    PlacardItem *plPtr = (PlacardItem *) itemPtr;
    int imagew, delta, ellx, elly, ellw, ellh;
    short drawableX, drawableY;
    Tk_FontMetrics fm;

    if (plPtr->gcText == None) {
        return;
    }

    /*
     * If there's a background rectangle, draw it first.
     */
    Tk_CanvasDrawableCoords(canvas, (double)plPtr->leftEdge,
            (double)plPtr->header.y1, &drawableX, &drawableY);

    if (plPtr->bgBorder) {
        Tk_Fill3DRectangle(plPtr->tkwin, drawable, plPtr->bgBorder,
            (int)drawableX, (int)drawableY,
            (int)(plPtr->rightEdge - plPtr->leftEdge),
            (int)(plPtr->header.y2 - plPtr->header.y1),
            plPtr->borderWidth, plPtr->relief);
    }

    if (plPtr->gcLineRect) {
        XDrawRectangle(display, drawable, plPtr->gcLineRect,
            (int)drawableX, (int)drawableY,
            (unsigned int)(plPtr->rightEdge - plPtr->leftEdge - 1),
            (unsigned int)(plPtr->header.y2 - plPtr->header.y1 - 1));
    }

    /*
     * If there's a right-hand image, draw it first.
     */
    imagew = 0;
    if (plPtr->imageRight) {
        Tk_CanvasDrawableCoords(canvas, (double)plPtr->rightEdge,
                (double)plPtr->header.y1, &drawableX, &drawableY);

        delta = (plPtr->header.y2 - plPtr->header.y1)/2 - plPtr->imageRightH/2;
        drawableX -= plPtr->imageRightW + plPtr->padding + plPtr->borderWidth;
        Tk_RedrawImage(plPtr->imageRight, 0, 0,
                       plPtr->imageRightW, plPtr->imageRightH,
                       drawable,
                       (int)drawableX, (int)(drawableY+delta));
        imagew += plPtr->imageRightW;
    }

    /*
     * If there's a left-hand image, draw it next.
     */
    Tk_CanvasDrawableCoords(canvas, (double)plPtr->leftEdge,
            (double)plPtr->header.y1, &drawableX, &drawableY);

    drawableX += plPtr->padding + plPtr->borderWidth;
    drawableY += plPtr->padding + plPtr->borderWidth;

    if (plPtr->imageLeft) {
        delta = (plPtr->header.y2 - plPtr->header.y1)/2
                       - plPtr->imageLeftH/2 - plPtr->padding
                       - plPtr->borderWidth;

        Tk_RedrawImage(plPtr->imageLeft, 0, 0,
                       plPtr->imageLeftW, plPtr->imageLeftH,
                       drawable,
                       (int)drawableX, (int)(drawableY+delta));
        drawableX += plPtr->imageLeftW + plPtr->padding;
        imagew += plPtr->imageLeftW;
    }

    /*
     * Draw the text.
     */
    Tk_DrawTextLayout(display, drawable, plPtr->gcText, plPtr->textLayout,
            drawableX, drawableY, 0, -1);

    if (plPtr->textDrawChars < plPtr->textNumChars
          && plPtr->rightEdge - plPtr->leftEdge - imagew > ellw) {
        Tk_SizeOfBitmap(Tk_Display(plPtr->tkwin), plPtr->dots, &ellw, &ellh);
        Tk_GetFontMetrics(plPtr->tkfont, &fm);
        ellx = drawableX+plPtr->textWidth;
        elly = drawableY+fm.ascent-1;

        XSetClipMask(display, plPtr->gcText, plPtr->dots);
        XSetClipOrigin(display, plPtr->gcText, ellx, elly);

        XCopyPlane(display, plPtr->dots, drawable, plPtr->gcText, 0, 0,
            (unsigned int)ellw, (unsigned int)ellh, ellx, elly, 1);

        XSetClipOrigin(display, plPtr->gcText, 0, 0);
        XSetClipMask(display, plPtr->gcText, None);
    }
}

/*
 * ------------------------------------------------------------------------
 *  PlacardToPoint --
 *
 *    Computes the distance from a given point to a given placard
 *    item, in canvas units.
 *
 *  Results:
 *    The return value is 0 if the point whose x and y coordinates
 *    are pointPtr[0] and pointPtr[1] is inside the text item.  If
 *    the point isn't inside the text item then the return value
 *    is the distance from the point to the text item.
 *
 *  Side effects:
 *    None.
 * ------------------------------------------------------------------------
 */
static double
PlacardToPoint(canvas, itemPtr, pointPtr)
    Tk_Canvas canvas;                /* Canvas containing itemPtr. */
    Tk_Item *itemPtr;                /* Item to check against point. */
    double *pointPtr;                /* Pointer to x and y coordinates. */
{
    PlacardItem *plPtr = (PlacardItem *) itemPtr;
    double value, x0, y0, x1, y1, xDiff, yDiff;

    if (plPtr->bgBorder || plPtr->lineColor) {
        /* have a rectangle -- compare directly against that */
        x0 = plPtr->leftEdge;
        y0 = plPtr->header.y1;
        x1 = plPtr->rightEdge;
        y1 = plPtr->header.y2;
    }
    else if (plPtr->imageLeft || plPtr->imageRight) {
        /* have images -- compare against a rectangle around image/text */
        x0 = plPtr->leftEdge + plPtr->padding + plPtr->borderWidth;
        y0 = plPtr->header.y1 + plPtr->padding + plPtr->borderWidth;
        x1 = plPtr->rightEdge - plPtr->padding - plPtr->borderWidth;
        y1 = plPtr->header.y2 - plPtr->padding - plPtr->borderWidth;
    }
    else {
        value = (double) Tk_DistanceToTextLayout(plPtr->textLayout,
                    (int) pointPtr[0] - plPtr->leftEdge,
                    (int) pointPtr[1] - plPtr->header.y1);

        if ((plPtr->textColor == NULL) || (plPtr->text == NULL)
              || (*plPtr->text == 0)) {
            value = 1.0e36;
        }
        return value;
    }

    if (pointPtr[0] < x0) {
        xDiff = x0 - pointPtr[0];
    } else if (pointPtr[0] >= x1)  {
        xDiff = pointPtr[0] + 1 - x1;
    } else {
        xDiff = 0;
    }

    if (pointPtr[1] < y0) {
        yDiff = y0 - pointPtr[1];
    } else if (pointPtr[1] >= y1)  {
        yDiff = pointPtr[1] + 1 - y1;
    } else {
        yDiff = 0;
    }

    return hypot(xDiff, yDiff);
}

/*
 * ------------------------------------------------------------------------
 *  PlacardToArea --
 *
 *    This procedure is called to determine whether an item lies
 *    entirely inside, entirely outside, or overlapping a given
 *    rectangle.
 *
 *  Results:
 *    -1 is returned if the item is entirely outside the area given
 *    by rectPtr, 0 if it overlaps, and 1 if it is entirely inside
 *    the given area.
 *
 *  Side effects:
 *    None.
 * ------------------------------------------------------------------------
 */
static int
PlacardToArea(canvas, itemPtr, rectPtr)
    Tk_Canvas canvas;       /* Canvas containing itemPtr. */
    Tk_Item *itemPtr;       /* Item to check against rectangle. */
    double *rectPtr;        /* Pointer to array of four coordinates
                             * (x1, y1, x2, y2) describing rectangular
                             * area.  */
{
    PlacardItem *plPtr = (PlacardItem *) itemPtr;
    int x0, y0, x1, y1;

    if (plPtr->bgBorder || plPtr->lineColor) {
        /* have a rectangle -- compare directly against that */
        x0 = plPtr->leftEdge;
        y0 = plPtr->header.y1;
        x1 = plPtr->rightEdge;
        y1 = plPtr->header.y2;
    }
    else if (plPtr->imageLeft || plPtr->imageRight) {
        /* have images -- compare against a rectangle around image/text */
        x0 = plPtr->leftEdge + plPtr->padding + plPtr->borderWidth;
        y0 = plPtr->header.y1 + plPtr->padding + plPtr->borderWidth;
        x1 = plPtr->rightEdge - plPtr->padding - plPtr->borderWidth;
        y1 = plPtr->header.y2 - plPtr->padding - plPtr->borderWidth;
    }
    else {
        /* have only text -- compare against text layout */
        return Tk_IntersectTextLayout(plPtr->textLayout,
            (int) (rectPtr[0] + 0.5) - (plPtr->leftEdge + plPtr->padding + plPtr->borderWidth),
            (int) (rectPtr[1] + 0.5) - (plPtr->header.y1 + plPtr->padding + plPtr->borderWidth),
            (int) (rectPtr[2] - rectPtr[0] + 0.5),
            (int) (rectPtr[3] - rectPtr[1] + 0.5));
    }

    /* compare against rectangle... */
    if (rectPtr[0] > x1 || rectPtr[1] > y1
          || rectPtr[2] < x0 || rectPtr[3] < y0) {
        return -1;  /* completely outside */
    }
    if (rectPtr[0] >= x0 && rectPtr[1] >= y0
          && rectPtr[2] <= x1 && rectPtr[3] <= y1) {
        return 1;  /* completely inside */
    }
    return 0;  /* must be overlapping */
}

/*
 * ------------------------------------------------------------------------
 *  ScalePlacard --
 *
 *    This procedure is invoked to rescale a text item.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Scales the position of the text, but not the size of the font
 *    for the text.
 * ------------------------------------------------------------------------
 */
        /* ARGSUSED */
static void
ScalePlacard(canvas, itemPtr, originX, originY, scaleX, scaleY)
    Tk_Canvas canvas;             /* Canvas containing rectangle. */
    Tk_Item *itemPtr;             /* Rectangle to be scaled. */
    double originX, originY;      /* Origin about which to scale rect. */
    double scaleX;                /* Amount to scale in X direction. */
    double scaleY;                /* Amount to scale in Y direction. */
{
    PlacardItem *plPtr = (PlacardItem *) itemPtr;

    plPtr->x = originX + scaleX*(plPtr->x - originX);
    plPtr->y = originY + scaleY*(plPtr->y - originY);
    ComputePlacardBbox(canvas, plPtr);
    return;
}

/*
 * ------------------------------------------------------------------------
 *  TranslatePlacard --
 *
 *    This procedure is called to move a text item by a given amount.
 *
 *  Results:
 *    None.
 *
 *  Side effects:
 *    The position of the text item is offset by (xDelta, yDelta),
 *    and the bounding box is updated in the generic part of the
 *    item structure.
 * ------------------------------------------------------------------------
 */
static void
TranslatePlacard(canvas, itemPtr, deltaX, deltaY)
    Tk_Canvas canvas;             /* Canvas containing item. */
    Tk_Item *itemPtr;             /* Item that is being moved. */
    double deltaX, deltaY;        /* Amount by which item is to be moved. */
{
    PlacardItem *plPtr = (PlacardItem *) itemPtr;

    plPtr->x += deltaX;
    plPtr->y += deltaY;
    ComputePlacardBbox(canvas, plPtr);
}

/*
 * ------------------------------------------------------------------------
 *  PlacardToPostscript --
 *
 *    This procedure is called to generate Postscript for placard
 *    items.
 *
 *  Results:
 *    The return value is a standard Tcl result.  If an error
 *    occurs in generating Postscript then an error message is
 *    left in the interp's result, replacing whatever used
 *    to be there.  If no error occurs, then Postscript for the
 *    item is appended to the result.
 *
 *  Side effects:
 *    None.
 * ------------------------------------------------------------------------
 */
static int
PlacardToPostscript(interp, canvas, itemPtr, prepass)
    Tcl_Interp *interp;         /* Leave Postscript or error message here. */
    Tk_Canvas canvas;           /* Information about overall canvas. */
    Tk_Item *itemPtr;           /* Item for which Postscript is wanted. */
    int prepass;                /* 1 means this is a prepass to collect
                                 * font information; 0 means final Postscript
                                 * is being created. */
{
    PlacardItem *plPtr = (PlacardItem *) itemPtr;
    Tk_Window canvasWin = Tk_CanvasTkwin(canvas);

    int x, y;
    double xpos;
    Tk_FontMetrics fm;
    char *justify;
    char buffer[500];
    int delta;

    if (plPtr->textColor == NULL || plPtr->text == NULL
          || *plPtr->text == 0) {
        return TCL_OK;
    }

    if (Tk_CanvasPsFont(interp, canvas, plPtr->tkfont) != TCL_OK) {
        return TCL_ERROR;
    }


    if (!prepass && 
	Tk_CanvasPsColor(interp, canvas, plPtr->textColor) != TCL_OK) {
        return TCL_ERROR;
    }
    xpos = plPtr->x;

    /*
     * Check for an image to print.
     */

    if (plPtr->imageLeft) {
      if(!prepass) {
	delta = (plPtr->header.y2 - plPtr->header.y1)/2 - plPtr->imageLeftH/2;
	sprintf(buffer, "%.15g %.15g", xpos,
		Tk_CanvasPsY(canvas, plPtr->y+delta+2));
	Tcl_AppendResult(interp, buffer, " translate\n", (char *) NULL);
      }

      Tk_PostscriptImage(plPtr->imageLeft, interp, canvasWin,
			 ((TkCanvas*)canvas)->psInfo, 0, 0,
                         plPtr->imageLeftW, plPtr->imageLeftH, prepass);

      if ( !prepass ) {
	/*
	 * This PS code is needed in order for the label to display
	 * correctly
	 */
	
	Tcl_AppendResult(interp,"grestore\ngsave\n",(char *)NULL);
      }

      xpos += plPtr->imageLeftW + plPtr->padding;
    }

    
    if (prepass != 0 ) {
      return TCL_OK;
    }

    /*
     * Before drawing the text, reset the font and color information.
     * Otherwise the text won't appear.
     */

    if ( plPtr->imageLeft ){
      if (Tk_CanvasPsFont(interp, canvas, plPtr->tkfont) != TCL_OK) {
        return TCL_ERROR;
      }
      
      if (Tk_CanvasPsColor(interp, canvas, plPtr->textColor) != TCL_OK) {
        return TCL_ERROR;
      }
    }


    /*
     * Draw the text for the placard.
     */
    sprintf(buffer, "%.15g %.15g [\n", xpos,
        Tk_CanvasPsY(canvas, plPtr->y));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    Tk_TextLayoutToPostscript(interp, plPtr->textLayout);

    x = 0;  y = 0;  justify = NULL;
    switch (plPtr->anchor) {
        case TK_ANCHOR_NW:        x = 0; y = 0;        break;
        case TK_ANCHOR_N:         x = 1; y = 0;        break;
        case TK_ANCHOR_NE:        x = 2; y = 0;        break;
        case TK_ANCHOR_E:         x = 2; y = 1;        break;
        case TK_ANCHOR_SE:        x = 2; y = 2;        break;
        case TK_ANCHOR_S:         x = 1; y = 2;        break;
        case TK_ANCHOR_SW:        x = 0; y = 2;        break;
        case TK_ANCHOR_W:         x = 0; y = 1;        break;
        case TK_ANCHOR_CENTER:    x = 1; y = 1;        break;
    }
    justify = "0"; /* TK_JUSTIFY_LEFT */

    Tk_GetFontMetrics(plPtr->tkfont, &fm);
    sprintf(buffer, "] %d %g %g %s %s DrawText\n",
            fm.linespace, x / -2.0, y / 2.0, justify,
            "false" /* stipple */);
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * ImageLeftChangedProc --
 *
 *      This procedure is invoked by the image code whenever the manager
 *      for an image does something that affects the image's size or
 *      how it is displayed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the canvas to get redisplayed.
 *
 * ----------------------------------------------------------------------
 */
static void
ImageLeftChangedProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;              /* Pointer to canvas item for image. */
    int x, y;                           /* Upper left pixel (within image)
                                         * that must be redisplayed. */
    int width, height;                  /* Dimensions of area to redisplay
                                         * (may be <= 0). */
    int imgWidth, imgHeight;            /* New dimensions of image. */
{      
    PlacardItem *plPtr = (PlacardItem *) clientData;
    
    plPtr->imageLeftW = imgWidth;
    plPtr->imageLeftH = imgHeight;
    ComputePlacardBbox(plPtr->canvas, plPtr);
    return;
}

/*
 * ----------------------------------------------------------------------
 *
 * ImageRightChangedProc --
 *
 *      This procedure is invoked by the image code whenever the manager
 *      for an image does something that affects the image's size or
 *      how it is displayed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the canvas to get redisplayed.
 *
 * ----------------------------------------------------------------------
 */
static void
ImageRightChangedProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;              /* Pointer to canvas item for image. */
    int x, y;                           /* Upper left pixel (within image)
                                         * that must be redisplayed. */
    int width, height;                  /* Dimensions of area to redisplay
                                         * (may be <= 0). */
    int imgWidth, imgHeight;            /* New dimensions of image. */
{      
    PlacardItem *plPtr = (PlacardItem *) clientData;
    
    plPtr->imageRightW = imgWidth;
    plPtr->imageRightH = imgHeight;
    ComputePlacardBbox(plPtr->canvas, plPtr);
    return;
}
