
/*
 * ----------------------------------------------------------------------
 *  RpCanvHotspot - canvas item with text and background box
 *
 *  This canvas item makes it easy to create a box with text inside.
 *  The box is normally stretched around the text, but can be given
 *  a max size and causing text to be clipped.
 *
 *    .c create hotspot <x> <y> -anchor <nsew> \
 *        -text <text>         << text to be displayed
 *        -font <name>         << font used for text
 *        -image <image>       << image displayed on the right of text
 *        -maxwidth <size>     << maximum size
 *        -textcolor <color>   << text color
 *        -substtextcolor <color>   << substituted text color
 *        -background <color>  << fill for rect behind text
 *        -outline <color>     << outline around text
 *        -borderwidth <size>  << for 3D border
 *        -relief <value>      << for 3D border (drawn under -outline)
 *        -tags <taglist>      << usual Tk canvas tags
 *	  -interp interp	Interpreter containing substitution variables.
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

/* used for text within the hotspot that needs to be truncated */
#define ellipsis_width 9
#define ellipsis_height 1
static unsigned char ellipsis_bits[] = {
   0x92, 0x00};

typedef struct _HotspotSegment HotspotSegment;

/*
 * HotspotSegment 
 *
 */
struct _HotspotSegment {
    HotspotSegment *nextPtr;
    int type;				/* Image, normal text, or substituted
					 * text. */
    short int x, y;			/* Location of image or text on
					 * canvas. */
    int length;				/* # of bytes in below string. */
    int lineNum;			/* Line where segment if located. */
    const char *subst;			/* Substitututed text. */
    const char text[1];
};

/*
 * Record for each hotspot item:
 */
typedef struct HotspotItem  {
    Tk_Item header;			/* Generic stuff that's the same for
					 * all types.  MUST BE FIRST IN
					 * STRUCTURE. */
    Tcl_Interp *interp;			/* Interp that owns this item */
    Tk_Window tkwin;			/* Window that represents this
					 * canvas */
    Tk_Canvas canvas;			/* Canvas that owns this item */
    Pixmap dots;			/* ellipsis used for truncated text */

    /*
     * Fields that are set by widget commands other than "configure":
     */
    double x, y;			/* Positioning point for item. This in
					 * conjunction with -anchor determine
					 * where the item is drawn on the
					 * canvas. */

    /*
     * Configuration settings that are updated by Tk_ConfigureWidget:
     */
    const char *text;			/* Text to be displayed */
    Tk_Anchor anchor;			/* Where to anchor text relative to
					 * (x,y) */
    XColor *textColor;			/* Color for text */
    XColor *lineColor;			/* Color for outline of rectangle */
    Tk_3DBorder bgBorder;		/* used for drawing background
					 * rectangle */
    Tk_Font tkfont;			/* Font for drawing text */
    int width;				/* Fixed width for hotspot (0 = not
					 * set) */
    int maxWidth;			/* Maximum overall width for
					 * hotspot */
    int borderWidth;			/* supports 3D border (drawn under
					 * -outline) */
    int relief;				/* Indicates whether hotspot as a
					 * whole is raised, sunken, or flat */
    int padding;			/* Padding around text and images */

    char *imageLeftString;		/* Name of image displayed on the
					 * left */
    char *imageRightString;		/* Name of image displayed on the
					 * right */
  
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
} HotspotItem;

/*
 * Information used for parsing configuration specs:
 */
static Tk_CustomOption tagsOption = {
    (Tk_OptionParseProc *) Tk_CanvasTagsParseProc,
    Tk_CanvasTagsPrintProc, (ClientData) NULL
};

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_ANCHOR, "-anchor", (char*)NULL, (char*)NULL,
        "center", Tk_Offset(HotspotItem, anchor),
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_BORDER, "-background", (char*)NULL, (char*)NULL,
        "", Tk_Offset(HotspotItem, bgBorder), TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-borderwidth", (char*)NULL, (char*)NULL,
        "0", Tk_Offset(HotspotItem, borderWidth), 0},
    {TK_CONFIG_FONT, "-font", (char*)NULL, (char*)NULL,
        "helvetica -12", Tk_Offset(HotspotItem, tkfont), 0},
    {TK_CONFIG_COLOR, "-foreground", (char*)NULL, (char*)NULL,
        "black", Tk_Offset(HotspotItem, textColor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-imageleft", (char*)NULL, (char*)NULL,
        (char*)NULL, Tk_Offset(HotspotItem, imageLeftString),
        TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-imageright", (char*)NULL, (char*)NULL,
        (char*)NULL, Tk_Offset(HotspotItem, imageRightString),
        TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-maxwidth", (char*)NULL, (char*)NULL,
        "0", Tk_Offset(HotspotItem, maxWidth), 0},
    {TK_CONFIG_COLOR, "-outline", (char*)NULL, (char*)NULL,
        "", Tk_Offset(HotspotItem, lineColor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-padding", (char*)NULL, (char*)NULL,
        "4", Tk_Offset(HotspotItem, padding), 0},
    {TK_CONFIG_RELIEF, "-relief", (char*)NULL, (char*)NULL,
        "flat", Tk_Offset(HotspotItem, relief), 0},
    {TK_CONFIG_CUSTOM, "-tags", (char*)NULL, (char*)NULL,
        (char*)NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_STRING, "-text", (char*)NULL, (char*)NULL,
        "", Tk_Offset(HotspotItem, text), 0},
    {TK_CONFIG_PIXELS, "-width", (char*)NULL, (char*)NULL,
        "0", Tk_Offset(HotspotItem, width), 0},
    {TK_CONFIG_END, (char*)NULL, (char*)NULL, (char*)NULL,
        (char*)NULL, 0, 0}
};

/*
 * Prototypes for procedures defined in this file
 */
static Tk_ItemCofigureProc ConfigureProc;
static Tk_ItemCreateProc CreateProc;
static Tk_ItemDeleteProc DeleteProc;
static Tk_ItemDisplayProc DisplayProc;
static Tk_ItemScaleProc ScaleProc;
static Tk_ItemTranslateProc TranslateProc;
static Tk_ItemCoordsProc CoordsProc;
static Tk_ItemAreaProc AreaProc;
static Tk_ItemPointProc PointProc;
static Tk_ItemPostscriptproc PostscriptProc;

static Tk_ImageChangedProc ImageChangedProc;

/*
 * The structures below defines the canvas item type:
 */
Tk_ItemType rpHotspotType = {
    "hotspot",                          /* name */
    sizeof(HotspotItem),                /* itemSize */
    CreateProc,
    configSpecs,                        /* configSpecs */
    ConfigureProc,
    CoordsProc,			
    DeleteProc,				
    DisplayProc,			
    TK_CONFIG_OBJS,                     /* flags */
    PointProc,				
    AreaProc,				
    PostscriptProc,			
    ScaleProc,				
    TranslateProc,			
    NULL,				/* indexProc */
    NULL,				/* icursorProc */
    NULL,				/* selectionProc */
    NULL,				/* insertProc */
    NULL,				/* dTextProc */
    NULL,				/* nextPtr */
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
 *  RpCanvHotspot_Init --
 *
 *  Invoked when the Rappture GUI library is being initialized
 *  to install the "hotspot" item on the Tk canvas widget.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpCanvHotspot_Init(interp)
    Tcl_Interp *interp;         /* interpreter being initialized */
{
    Tk_CreateItemType(&rpHotspotType);

    Tk_DefineBitmap(interp, Tk_GetUid("rp_ellipsis"),
        (char*)ellipsis_bits, ellipsis_width, ellipsis_height);

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 *  CreateProc --
 *
 *    This procedure is invoked to create a new hotspot item
 *    in a canvas.  A hotspot is a text item with a box behind it.
 *
 *  Results:
 *    A standard Tcl return name.  If an error occurred in creating
 *    the item then an error message is left in the interp's result.
 *    In this case itemPtr is left uninitialized so it can be safely
 *    freed by the caller.
 *
 *  Side effects:
 *    A new item is created.
 *
 * ------------------------------------------------------------------------
 */
static int
CreateProc(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *canvItemPtr,
	   int objc, Tcl_Obj *const *objv)
{
    HotspotItem *itemPtr = (HotspotItem *)canvItemPtr;
    int i;
    double x, y;

    tkwin = Tk_CanvasTkwin(canvas);
    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
	    Tk_PathName(tkwin), " create ", itemPtr->typePtr->name,
	    " x y ?options?\"", (char *)NULL);
	return TCL_ERROR;
    }

    /*
     * Carry out initialization that is needed in order to clean up after
     * errors during the the remainder of this procedure.
     */
    itemPtr->interp = interp;
    itemPtr->display = Tk_Display(tkwin);
    itemPtr->tkwin = Tk_CanvasTkwin(canvas);
    itemPtr->canvas = canvas;
    itemPtr->anchor = TK_ANCHOR_CENTER;
    itemPtr->textColor = NULL;
    itemPtr->border = NULL;
    itemPtr->outlineColor = NULL;
    itemPtr->textFont = NULL;
    itemPtr->substFont = NULL;
    itemPtr->width = 0;
    itemPtr->maxWidth = 0;
    itemPtr->borderWidth = 0;
    itemPtr->relief = TK_RELIEF_FLAT;
    itemPtr->padding = 0;
    itemPtr->imageLeftString  = NULL;
    itemPtr->imageRightString = NULL;
    itemPtr->imageLeft        = NULL;
    itemPtr->imageRight       = NULL;
    itemPtr->textNumChars     = 0;
    itemPtr->textNumBytes     = 0;
    itemPtr->textDrawChars    = 0;
    itemPtr->textWidth        = 0;
    itemPtr->textLayout       = NULL;
    itemPtr->leftEdge         = 0;
    itemPtr->rightEdge        = 0;
    itemPtr->textGC = None;
    itemPtr->substGC = None;
    itemPtr->outlineGC = None;

    /* Process the arguments to fill in the item record. */
    if ((Tk_CanvasGetCoord(interp, canvas, objv[0], &x) != TCL_OK) ||
	(Tk_CanvasGetCoord(interp, canvas, objv[1], &y) != TCL_OK)) {
	DeleteProc(canvas, itemPtr, itemPtr->display);
	return TCL_ERROR;
    }
    itemPtr->x = x;
    itemPtr->y = y;
    ComputeBbox(canvas, itemPtr);
    if (ConfigureProc(interp, canvas, canvItemPtr, objc - 2, objv + 2, 0) 
	!= TCL_OK) {
	DeleteProc(canvas, itemPtr, itemPtr->display);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *
 *  CoordsProc --
 *
 *    This procedure is invoked to process the "coords" widget command on
 *    hotspot items.  See the user documentation for details on what
 *    it does.
 *
 *  Results:
 *    Returns TCL_OK or TCL_ERROR, and sets the interp's result.
 *
 *  Side effects:
 *    The coordinates for the given item may be changed.
 *
 * ------------------------------------------------------------------------
 */
static int
CoordsProc(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *canvItemPtr, 
	   int objc, Tcl_Obj *const *objv)
{
    HotspotItem *itemPtr = (HotspotItem *) canvItemPtr;
    double x, y;

    if (objc == 0) {
        Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, obj, Tcl_NewDoubleObj(itemPtr->x));
        Tcl_ListObjAppendElement(interp, obj, Tcl_NewDoubleObj(itemPtr->y));
        Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (objc == 1) {
	int elc;
	Tcl_Obj **elv;

	if (Tcl_ListObjGetElements(interp, objv[0], &elc, &elv) != TCL_OK) {
	    return TCL_ERROR;
        }
        if ((Tk_CanvasGetCoordFromObj(interp, canvas, elv[0], &x) != TCL_OK) || 
	    (Tk_CanvasGetCoordFromObj(interp, canvas, elv[1], &y) != TCL_OK)) {
            return TCL_ERROR;
        }
    } else (objc == 3) {
        if ((Tk_CanvasGetCoordFromObj(interp, canvas, objv[0], &x) != TCL_OK) ||
	    (Tk_CanvasGetCoordFromObj(interp, canvas, objv[1], &y) != TCL_OK)) {
            return TCL_ERROR;
        }
    } else {
	char buf[64 + TCL_INTEGER_SPACE];

        sprintf(buf, "wrong # coordinates: expected 0 or 2, got %d", objc);
        Tcl_AppendResult(interp, buf, (char *)NULL);
        return TCL_ERROR;
    }
    itemPtr->x = x;
    itemPtr->y = y;
    ComputeBbox(canvas, itemPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ConfigureProc --
 *
 *    This procedure is invoked to configure various aspects of a
 *    hotspot item, such as its color and font.
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
ConfigureProc(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *canvItemPtr,
	      int objc, Tcl_Obj *CONST *objv, int flags)
{
    HotspotItem *itemPtr = (HotspotItem *) canvItemPtr;
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    Tk_Window tkwin;
    XColor *color;
    Tk_Image image;

    tkwin = Tk_CanvasTkwin(canvas);
    if (Tk_ConfigureWidget(interp, tkwin, configSpecs,
            objc, (CONST char **) objv,
            (char*)itemPtr, flags|TK_CONFIG_OBJS) != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * A few of the options require additional processing, such as
     * graphics contexts.
     */
    color = itemPtr->textColor;
    newGC = None;
    if (itemPtr->tkfont != NULL) {
        gcValues.font = Tk_FontId(itemPtr->tkfont);
        mask = GCFont;
        if (color != NULL) {
            gcValues.foreground = color->pixel;
            mask |= GCForeground;
            newGC = Tk_GetGC(tkwin, mask, &gcValues);
        }
    }
    if (itemPtr->gcText != None) {
        Tk_FreeGC(Tk_Display(tkwin), itemPtr->gcText);
    }
    itemPtr->gcText = newGC;


    newGC = None;
    if (itemPtr->lineColor != NULL) {
        gcValues.foreground = itemPtr->lineColor->pixel;
        gcValues.line_width = 1;
        mask = GCForeground | GCLineWidth;
        newGC = Tk_GetGC(tkwin, mask, &gcValues);
    }
    if (itemPtr->gcLineRect != None) {
        Tk_FreeGC(Tk_Display(tkwin), itemPtr->gcLineRect);
    }
    itemPtr->gcLineRect = newGC;


    if (itemPtr->imageLeftString != NULL) {
        image = Tk_GetImage(interp, tkwin, itemPtr->imageLeftString,
                ImageChangedProc, (ClientData)itemPtr);
        if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (itemPtr->imageLeft != NULL) {
        Tk_FreeImage(itemPtr->imageLeft);
    }
    itemPtr->imageLeft = image;

    if (image != NULL) {
        Tk_SizeOfImage(image,&itemPtr->imageLeftW, &itemPtr->imageLeftH);
    }


    if (itemPtr->imageRightString != NULL) {
        image = Tk_GetImage(interp, tkwin, itemPtr->imageRightString,
                ImageChangedProc, (ClientData)itemPtr);
        if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (itemPtr->imageRight != NULL) {
        Tk_FreeImage(itemPtr->imageRight);
    }
    itemPtr->imageRight = image;

    if (image != NULL) {
        Tk_SizeOfImage(image,&itemPtr->imageRightW, &itemPtr->imageRightH);
    }


    if (itemPtr->text != NULL) {
        itemPtr->textNumBytes = strlen(itemPtr->text);
        itemPtr->textNumChars = Tcl_NumUtfChars(itemPtr->text, itemPtr->textNumBytes);
    };

    ComputeHotspotBbox(canvas, itemPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  DeleteHotspot --
 *
 *    This procedure is called to clean up the data structure
 *    associated with a hotspot item.
 *
 *  Results:
 *    None.
 *
 *  Side effects:
 *    Resources associated with itemPtr are released.
 * ------------------------------------------------------------------------
 */
static void
DeleteProc(Tk_Canvas canvas, Tk_Item *canvItemPtr, Display *display)
{
    HotspotItem *itemPtr = (HotspotItem *)canvItemPtr;

    if (itemPtr->textColor != NULL) {
        Tk_FreeColor(itemPtr->textColor);
    }
    if (itemPtr->lineColor != NULL) {
        Tk_FreeColor(itemPtr->lineColor);
    }
    if (itemPtr->text != NULL) {
        ckfree(itemPtr->text);
    }


    if (itemPtr->imageLeftString != NULL) {
        ckfree(itemPtr->imageLeftString);
    }
    if (itemPtr->imageLeft != NULL ) {
        Tk_FreeImage(itemPtr->imageLeft);
    }
    if (itemPtr->imageRightString != NULL) {
        ckfree(itemPtr->imageRightString);
    }
    if (itemPtr->imageRight != NULL ) {
        Tk_FreeImage(itemPtr->imageRight);
    }

    Tk_FreeTextLayout(itemPtr->textLayout);

    Tk_FreeFont(itemPtr->tkfont);

    if (itemPtr->gcText != None) {
        Tk_FreeGC(display, itemPtr->gcText);
    }
    if (itemPtr->gcLineRect != None) {
        Tk_FreeGC(display, itemPtr->gcLineRect);
    }
}

/*
 * ------------------------------------------------------------------------
 *  ComputeBbox --
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
ComputeBbox(Tk_Canvas canvas, HotspotItem *itemPtr)
{
    int leftX, topY, width, height, maxwidth, ellw, ellh;

    /*
     * Get the layout for the hotspot text.
     */
    itemPtr->textDrawChars = itemPtr->textNumChars;

    Tk_FreeTextLayout(itemPtr->textLayout);
    itemPtr->textLayout = Tk_ComputeTextLayout(itemPtr->tkfont,
            itemPtr->text, itemPtr->textNumChars, -1,
            TK_JUSTIFY_LEFT, 0, &width, &height);

    itemPtr->textWidth = width;

    if (itemPtr->textColor == NULL) {
        width = height = 0;
    } else {
        width += 2*itemPtr->padding;
        height += 2*itemPtr->padding;
    }

    /*
     * If the current value has an image, add on its width.
     */
    if (itemPtr->imageLeft) {
        width += itemPtr->imageLeftW;
        if (itemPtr->textWidth > 0) {
            width += itemPtr->padding;
        }
        if (itemPtr->imageLeftH > height) {
            height = itemPtr->imageLeftH;
        }
    }
    if (itemPtr->imageRight) {
        width += itemPtr->imageRightW;
        if (itemPtr->textWidth > 0) {
            width += itemPtr->padding;
        }
        if (itemPtr->imageRightH > height) {
            height = itemPtr->imageRightH;
        }
    }

    /*
     * Add on extra size for the 3D border (if there is one).
     */
    if (itemPtr->borderWidth > 0) {
        width += 2*itemPtr->borderWidth;
        height += 2*itemPtr->borderWidth;
    }

    /*
     * If the overall width exceeds the -maxwidth, then we must
     * truncate the text to make it fit.
     */
    maxwidth = 0;
    if (itemPtr->maxWidth > 0 && itemPtr->width > 0) {
        maxwidth = (itemPtr->width < itemPtr->maxWidth) ? itemPtr->width : itemPtr->maxWidth;
    } else if (itemPtr->maxWidth > 0) {
        maxwidth = itemPtr->maxWidth;
    } else if (itemPtr->width > 0) {
        maxwidth = itemPtr->width;
    }

    if (maxwidth > 0 && width > maxwidth) {
        Tk_SizeOfBitmap(Tk_Display(itemPtr->tkwin), itemPtr->dots, &ellw, &ellh);
        width = maxwidth - 2*itemPtr->padding - 2*itemPtr->borderWidth - ellw;
        if (itemPtr->imageLeft) {
            width -= itemPtr->imageLeftW + itemPtr->padding;
        }
        if (itemPtr->imageRight) {
            width -= itemPtr->imageRightW + itemPtr->padding;
        }
        if (width < 0) { width = 0; }

        itemPtr->textDrawChars = Tk_MeasureChars(itemPtr->tkfont,
            itemPtr->text, itemPtr->textNumChars, width, /*flags*/ 0,
            &itemPtr->textWidth);

        /* recompute layout for truncated text */
        Tk_FreeTextLayout(itemPtr->textLayout);
        itemPtr->textLayout = Tk_ComputeTextLayout(itemPtr->tkfont,
                itemPtr->text, itemPtr->textDrawChars, -1,
                TK_JUSTIFY_LEFT, 0, NULL, NULL);

        width = maxwidth;  /* fit everything in this width */
    }

    /*
     * If there's a fixed -width, then use that instead.
     */
    if (itemPtr->width > 0) {
        width = itemPtr->width;
    }

    /*
     * Use overall geometry information to compute the top-left corner
     * of the bounding box for the text item.
     */
    leftX = (int) (itemPtr->x + 0.5);
    topY = (int) (itemPtr->y + 0.5);
    switch (itemPtr->anchor) {
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
    switch (itemPtr->anchor) {
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

    itemPtr->leftEdge  = leftX;
    itemPtr->rightEdge = leftX + width;

    /*
     * Last of all, update the bounding box for the item.
     */
    itemPtr->header.x1 = leftX;
    itemPtr->header.y1 = topY;
    itemPtr->header.x2 = leftX + width;
    itemPtr->header.y2 = topY + height;
}

/*
 * ------------------------------------------------------------------------
 *
 *  DisplayProc --
 *
 *    This procedure is invoked to draw a text item in a given drawable.
 *
 *  Results:
 *    None.
 *
 *  Side effects:
 *    ItemPtr is drawn in drawable using the transformation information
 *    in canvas.
 *
 * ------------------------------------------------------------------------
 */
static void
DisplayProc(Tk_Canvas canvas, Tk_Item *canvItemPtr, Display *display,
	    Drawable drawable, int x, int y, int width, int height)
{
    HotspotItem *itemPtr = (HotspotItem *) canvItemPtr;
    int imagew, delta, ellx, elly, ellw, ellh;
    short drawableX, drawableY;
    Tk_FontMetrics fm;

    if (itemPtr->gcText == None) {
        return;
    }
    Tk_CanvasDrawableCoords(canvas, (double)itemPtr->leftEdge,
            (double)itemPtr->header.y1, &canvasX, &canvasY);

    /*
     * If there's a background rectangle, draw it first.
     */
    if (itemPtr->border != NULL) {
        Tk_Fill3DRectangle(itemPtr->tkwin, drawable, itemPtr->border,
            (int)canvasX, (int)canvasY,
            (int)(itemPtr->rightEdge - itemPtr->leftEdge),
            (int)(itemPtr->header.y2 - itemPtr->header.y1),
            itemPtr->borderWidth, itemPtr->relief);
    }

    if (itemPtr->outline != NULL) {
        XDrawRectangle(display, drawable, itemPtr->outlineGC,
            (int)canvasX, (int)canvasY,
            (unsigned int)(itemPtr->rightEdge - itemPtr->leftEdge - 1),
            (unsigned int)(itemPtr->header.y2 - itemPtr->header.y1 - 1));
    }

    /*
     * If there's a right-hand image, draw it first.
     */
    imagew = 0;
    if (itemPtr->imageRight) {
        Tk_CanvasDrawableCoords(canvas, (double)itemPtr->rightEdge,
                (double)itemPtr->header.y1, &drawableX, &drawableY);

        delta = (itemPtr->header.y2 - itemPtr->header.y1)/2 - itemPtr->imageRightH/2;
        drawableX -= itemPtr->imageRightW + itemPtr->padding + itemPtr->borderWidth;
        Tk_RedrawImage(itemPtr->imageRight, 0, 0,
                       itemPtr->imageRightW, itemPtr->imageRightH,
                       drawable,
                       (int)drawableX, (int)(drawableY+delta));
        imagew += itemPtr->imageRightW;
    }

    /*
     * If there's a left-hand image, draw it next.
     */
    Tk_CanvasDrawableCoords(canvas, (double)itemPtr->leftEdge,
            (double)itemPtr->header.y1, &drawableX, &drawableY);

    drawableX += itemPtr->padding + itemPtr->borderWidth;
    drawableY += itemPtr->padding + itemPtr->borderWidth;

    if (itemPtr->imageLeft) {
        delta = (itemPtr->header.y2 - itemPtr->header.y1)/2
                       - itemPtr->imageLeftH/2 - itemPtr->padding
                       - itemPtr->borderWidth;

        Tk_RedrawImage(itemPtr->imageLeft, 0, 0,
                       itemPtr->imageLeftW, itemPtr->imageLeftH,
                       drawable,
                       (int)drawableX, (int)(drawableY+delta));
        drawableX += itemPtr->imageLeftW + itemPtr->padding;
        imagew += itemPtr->imageLeftW;
    }

    /*
     * Draw the text.
     */
    Tk_DrawTextLayout(display, drawable, itemPtr->gcText, itemPtr->textLayout,
            drawableX, drawableY, 0, -1);

    if (itemPtr->textDrawChars < itemPtr->textNumChars
          && itemPtr->rightEdge - itemPtr->leftEdge - imagew > ellw) {
        Tk_SizeOfBitmap(Tk_Display(itemPtr->tkwin), itemPtr->dots, &ellw, &ellh);
        Tk_GetFontMetrics(itemPtr->tkfont, &fm);
        ellx = drawableX+itemPtr->textWidth;
        elly = drawableY+fm.ascent-1;

        XSetClipMask(display, itemPtr->gcText, itemPtr->dots);
        XSetClipOrigin(display, itemPtr->gcText, ellx, elly);

        XCopyPlane(display, itemPtr->dots, drawable, itemPtr->gcText, 0, 0,
            (unsigned int)ellw, (unsigned int)ellh, ellx, elly, 1);

        XSetClipOrigin(display, itemPtr->gcText, 0, 0);
        XSetClipMask(display, itemPtr->gcText, None);
    }
}

/*
 * ------------------------------------------------------------------------
 *
 *  PointProc --
 *
 *    Computes the distance from a given point to a given hotspot
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
 *
 * ------------------------------------------------------------------------
 */
static double
PointProc(Tk_Canvas canvas, Tk_Item *canvItemPtr, double *pointPtr)
{
    HotspotItem *itemPtr = (HotspotItem *)canvItemPtr;
    double value, x0, y0, x1, y1, xDiff, yDiff;

    if (itemPtr->bgBorder || itemPtr->lineColor) {
        /* have a rectangle -- compare directly against that */
        x0 = itemPtr->leftEdge;
        y0 = itemPtr->header.y1;
        x1 = itemPtr->rightEdge;
        y1 = itemPtr->header.y2;
    }
    else if (itemPtr->imageLeft || itemPtr->imageRight) {
        /* have images -- compare against a rectangle around image/text */
        x0 = itemPtr->leftEdge + itemPtr->padding + itemPtr->borderWidth;
        y0 = itemPtr->header.y1 + itemPtr->padding + itemPtr->borderWidth;
        x1 = itemPtr->rightEdge - itemPtr->padding - itemPtr->borderWidth;
        y1 = itemPtr->header.y2 - itemPtr->padding - itemPtr->borderWidth;
    }
    else {
        value = (double) Tk_DistanceToTextLayout(itemPtr->textLayout,
                    (int) pointPtr[0] - itemPtr->leftEdge,
                    (int) pointPtr[1] - itemPtr->header.y1);

        if ((itemPtr->textColor == NULL) || (itemPtr->text == NULL)
              || (*itemPtr->text == 0)) {
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
 *
 *  AreaProc --
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
 *
 * ------------------------------------------------------------------------
 */
static int
AreaProc(Tk_Canvas canvas, Tk_Item *canvItemPtr, double *rectPtr)
{
    HotspotItem *itemPtr = (HotspotItem *)canvItemPtr;
    int x0, y0, x1, y1;

    if (itemPtr->bgBorder || itemPtr->lineColor) {
        /* have a rectangle -- compare directly against that */
        x0 = itemPtr->leftEdge;
        y0 = itemPtr->header.y1;
        x1 = itemPtr->rightEdge;
        y1 = itemPtr->header.y2;
    }
    else if (itemPtr->imageLeft || itemPtr->imageRight) {
        /* have images -- compare against a rectangle around image/text */
        x0 = itemPtr->leftEdge + itemPtr->padding + itemPtr->borderWidth;
        y0 = itemPtr->header.y1 + itemPtr->padding + itemPtr->borderWidth;
        x1 = itemPtr->rightEdge - itemPtr->padding - itemPtr->borderWidth;
        y1 = itemPtr->header.y2 - itemPtr->padding - itemPtr->borderWidth;
    }
    else {
        /* have only text -- compare against text layout */
        return Tk_IntersectTextLayout(itemPtr->textLayout,
            (int) (rectPtr[0] + 0.5) - (itemPtr->leftEdge + itemPtr->padding + itemPtr->borderWidth),
            (int) (rectPtr[1] + 0.5) - (itemPtr->header.y1 + itemPtr->padding + itemPtr->borderWidth),
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
 *
 *  ScaleProc --
 *
 *    This procedure is invoked to rescale a text item.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Scales the position of the text, but not the size of the font
 *    for the text.
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ScaleProc(Tk_Canvas canvas, Tk_Item *canvItemPtr, double x, double y, 
	  double scaleX, double scaleY)
{
    HotspotItem *itemPtr = (HotspotItem *)canvItemPtr;

    itemPtr->x = x + scaleX * (itemPtr->x - x);
    itemPtr->y = y + scaleY * (itemPtr->y - y);
    ComputeBbox(canvas, itemPtr);
    return;
}

/*
 * ------------------------------------------------------------------------
 *  TranslateProc --
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
TranslateProc(Tk_Canvas canvas, Tk_Item *canvItemPtr, double dx,  double dy)
{
    HotspotItem *itemPtr = (HotspotItem *)canvItemPtr;

    itemPtr->x += dx;
    itemPtr->y += dy;
    ComputeHotspotBbox(canvas, itemPtr);
}

/*
 * ------------------------------------------------------------------------
 *
 *  PostscriptProc --
 *
 *    This procedure is called to generate Postscript for hotspot items.
 *
 *  Results:
 *    The return value is a standard Tcl result.  If an error occurs in
 *    generating Postscript then an error message is left in the interp's
 *    result, replacing whatever used to be there.  If no error occurs, then
 *    Postscript for the item is appended to the result.
 *
 *  Side effects:
 *    None.
 * ------------------------------------------------------------------------
 */
static int
PostscriptProc(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *canvItemPtr, 
	       int prepass)
{
    HotspotItem *itemPtr = (HotspotItem *)canvItemPtr;
    Tk_Window canvasWin = Tk_CanvasTkwin(canvas);

    int x, y;
    double xpos;
    Tk_FontMetrics fm;
    char *justify;
    char buffer[500];
    int delta;

    if (itemPtr->textColor == NULL || itemPtr->text == NULL
          || *itemPtr->text == 0) {
        return TCL_OK;
    }

    if (Tk_CanvasPsFont(interp, canvas, itemPtr->tkfont) != TCL_OK) {
        return TCL_ERROR;
    }


    if (!prepass && 
	Tk_CanvasPsColor(interp, canvas, itemPtr->textColor) != TCL_OK) {
        return TCL_ERROR;
    }
    xpos = itemPtr->x;

    /*
     * Check for an image to print.
     */

    if (itemPtr->imageLeft) {
      if(!prepass) {
	delta = (itemPtr->header.y2 - itemPtr->header.y1)/2 - itemPtr->imageLeftH/2;
	sprintf(buffer, "%.15g %.15g", xpos,
		Tk_CanvasPsY(canvas, itemPtr->y+delta+2));
	Tcl_AppendResult(interp, buffer, " translate\n", (char *) NULL);
      }

      Tk_PostscriptImage(itemPtr->imageLeft, interp, canvasWin,
			 ((TkCanvas*)canvas)->psInfo, 0, 0,
                         itemPtr->imageLeftW, itemPtr->imageLeftH, prepass);

      if ( !prepass ) {
	/*
	 * This PS code is needed in order for the label to display
	 * correctly
	 */
	
	Tcl_AppendResult(interp,"grestore\ngsave\n",(char *)NULL);
      }

      xpos += itemPtr->imageLeftW + itemPtr->padding;
    }

    
    if (prepass != 0 ) {
      return TCL_OK;
    }

    /*
     * Before drawing the text, reset the font and color information.
     * Otherwise the text won't appear.
     */

    if ( itemPtr->imageLeft ){
      if (Tk_CanvasPsFont(interp, canvas, itemPtr->tkfont) != TCL_OK) {
        return TCL_ERROR;
      }
      
      if (Tk_CanvasPsColor(interp, canvas, itemPtr->textColor) != TCL_OK) {
        return TCL_ERROR;
      }
    }


    /*
     * Draw the text for the hotspot.
     */
    sprintf(buffer, "%.15g %.15g [\n", xpos,
        Tk_CanvasPsY(canvas, itemPtr->y));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    Tk_TextLayoutToPostscript(interp, itemPtr->textLayout);

    x = 0;  y = 0;  justify = NULL;
    switch (itemPtr->anchor) {
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

    Tk_GetFontMetrics(itemPtr->tkfont, &fm);
    sprintf(buffer, "] %d %g %g %s %s DrawText\n",
            fm.linespace, x / -2.0, y / 2.0, justify,
            "false" /* stipple */);
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * ImageChangedProc --
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
ImageChangedProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;              /* Pointer to canvas item for image. */
    int x, y;                           /* Upper left pixel (within image)
                                         * that must be redisplayed. */
    int width, height;                  /* Dimensions of area to redisplay
                                         * (may be <= 0). */
    int imgWidth, imgHeight;            /* New dimensions of image. */
{      
    HotspotItem *itemPtr = (HotspotItem *) clientData;
    
    itemPtr->imageLeftW = imgWidth;
    itemPtr->imageLeftH = imgHeight;
    ComputeBbox(plPtr->canvas, itemPtr);
    return;
}

static int
AddTextSegment(HotspotItem *itemPtr, const char *text, int length)
{
    HotspotSegment *segPtr;

    segPtr = Tcl_Alloc(sizeof(HotspotSegment) + length + 1);
    if (segPtr == NULL) {
	Tcl_Panic("can't allocate text segent for hotspot");
    }
    segPtr->itemPtr = itemPtr;
    segPtr->lineNum = itemPtr->numLines;
    segPtr->type = SEGMENT_TYPE_TEXT;
    memcpy(segPtr->text, text, length);
    segPtr->text[length] = '\0';
    segPtr->length = length;
    segPtr->x = segPtr->y = 0;
    segPtr->nextPtr = NULL;
    if (itemPtr->firstPtr == NULL) {
	itemPtr->firstPtr = segPtr;
    }
    if (itemPtr->lastPtr == NULL) {
	itemPtr->lastPtr = segPtr;
    } else {
	itemPtr->lastPtr->nextPtr = segPtr;
    }
}

static int
AddVariableSegment(HotspotItem *itemPtr, const char *varName, int length)
{
    HotspotSegment *segPtr;

    segPtr = Tcl_Alloc(sizeof(HotspotSegment) + length + 1);
    if (segPtr == NULL) {
	Tcl_Panic("can't allocate variable segent for hotspot");
    }
    segPtr->itemPtr = itemPtr;
    segPtr->lineNum = itemPtr->numLines;
    segPtr->type = SEGMENT_TYPE_VARIABLE;
    memcpy(segPtr->text, varName, length);
    segPtr->text[length] = '\0';
    segPtr->length = length;
    Tcl_TraceVar(itemPtr->substInterp, varName, VAR_FLAGS, ItemVarTraceProc, 
	segPtr);
    segPtr->subst = NULL;
    segPtr->x = segPtr->y = 0;
    segPtr->nextPtr = NULL;
    if (itemPtr->firstPtr == NULL) {
	itemPtr->firstPtr = segPtr;
    }
    if (itemPtr->lastPtr == NULL) {
	itemPtr->lastPtr = segPtr;
    } else {
	itemPtr->lastPtr->nextPtr = segPtr;
    }
}

static int
AddHotspotSegment(HotspotItem *itemPtr)
{
    HotspotSegment *segPtr;

    segPtr = Tcl_Alloc(sizeof(HotspotSegment));
    if (segPtr == NULL) {
	Tcl_Panic("can't allocate hotspot segent for hotspot");
    }
    segPtr->itemPtr = itemPtr;
    segPtr->lineNum = itemPtr->numLines;
    segPtr->type = SEGMENT_TYPE_HOTSPOT;
    segPtr->text[0] = '\0';
    segPtr->length = 0;
    segPtr->x = segPtr->y = 0;
    segPtr->nextPtr = NULL;
    if (itemPtr->firstPtr == NULL) {
	itemPtr->firstPtr = segPtr;
    }
    if (itemPtr->lastPtr == NULL) {
	itemPtr->lastPtr = segPtr;
    } else {
	itemPtr->lastPtr->nextPtr = segPtr;
    }
}

static int
ParseDescription(HotspotItem *itemPtr, const char *description, int length)
{
    const char *p, *pend, *start;

    start = description;
    itemPtr->numLines = 1;
    for (p = description, pend = p + length; p < pend; p++) {
	if ((p[0] == '$') && (p[1] == '{')) {
	    const char *varName;
	    const char q;

	    if (p > start) {
		/* Save the leading text. */
		AddTextSegment(itemPtr, start, p - start);
		start = p;
	    }
	    /* Start of substitution variable. */
	    varName = p + 2;
	    for (q = varName; q < pend; q++) {
		if (*q == '}') {
		    AddVariableSegment(itemPtr, varName, q - varName);
		    AddHotspotSegment(itemPtr);
		    p = q;
		    goto next;
		}
	    }
	    Tcl_AppendResult(interp, "back variable specification in \"",
			     description, "\"", (char *)NULL);
	    return TCL_ERROR;
	} else if (p[0] == '\n') {
	    if (p > start) {
		AddTextSegment(itemPtr, start, p - start);
		start = p + 1;
		itemPtr->numLines++;
	    }
	}
    }
    next:
    }
    if (p > start) {
	/* Save the trailing text. */
	AddTextSegment(itemPtr, start, p - start);
    }
    return TCL_OK;
}

static int
DoSubst(HotspotItem *itemPtr)
{
    HotspotSegment *segptr;

    for (segPtr = itemPtr->firstPtr; segPtr != NULL; segPtr = segPtr->nextPtr) {
	if (segPtr->type == SEGMENT_TYPE_VARIABLE) {
	    const char *value;

	    value = Tcl_GetVar(itemPtr->substInterp, segPtr->text, NULL, 
		TCL_GLOBAL_ONLY);
	    if (value == NULL) {
		value = "???";
	    }
	    if (segPtr->subst != NULL) {
		Tcl_Free(segPtr->subst);
	    }
	    segPtr->subst = Tcl_Alloc(strlen(value) + 1);
	    strcpy(segPtr->subst, value);
	}
    }
}

static int
ComputeItemGeometry(HotspotItem *itemPtr)
{
    int imgWidth, imgHeight;
    int w, h;
    HotspotSegment *segPtr;

    /* Collect the individual segment sizes. */
    Tk_SizeOfImage(itemPtr->image, &imgWidth, &ingHeight);
    imgWidth += 2 * IMAGE_PAD;
    imgHeight += 2 * IMAGE_PAD;
    for (segPtr = itemPtr->firstPtr; segPtr != NULL; segPtr = segPtr->nextPtr) {
	if (segPtr->layout != NULL) {
	    Tk_FreeTextLayout(segPtr->layout);
	}
	switch (segPtr->type) {
	case SEGMENT_TYPE_HOTSPOT:
	    w = imgWidth, h = imgHeight;
	    break;
	case SEGMENT_TYPE_VARIABLE:
	    segPtr->layout = Tk_ComputeTextLayout(segPtr->substFont, 
		segPtr->subst, segPtr->length, wrapLength, justify, flags, 
		&w, &h);
	    break;
	case SEGMENT_TYPE_TEXT:
	    segPtr->layout = Tk_ComputeTextLayout(segPtr->textFont, 
		segPtr->text, segPtr->length, wrapLength, justify, flags, 
		&w, &h);
	    break;
	default:
	    Tcl_Panic("unknown hotspot segment type");
	}
	segPtr->width = w;
	segPtr->height = h;
    }
    /* Compute the relative positions of the segments. */
    x = y = 0;
    maxWidth = maxHeight = 0;
    for (segPtr = itemPtr->firstPtr; segPtr != NULL; segPtr = segPtr->nextPtr) {
	int maxHeight, lineNum;
	HotspotSegment *startPtr;

	maxHeight = 0;
	lineNum = segPtr->lineNum;
	startPtr = segPtr;
	x = 0;
	while ((segPtr->lineNum == lineNum) && (segPtr != NULL)) {
	    if (segPtr->height > segPtr->maxHeight) {
		segPtr->maxHeight = segPtr->height;
	    }
	    segPtr = segPtr->nextPtr;
	    segPtr->x = x;
	    x += segPtr->width;
	}
	if (x > maxWidth) {
	    maxWidth = x;
	}
	segPtr = startPtr;
	while ((segPtr->lineNum == lineNum) && (segPtr != NULL)) {
	    segPtr->y = (maxHeight - segPtr->height) / 2;
	}
	y += maxHeight;
    }
    itemPtr->width = maxWidth;
    itemPtr->height = y;
}
