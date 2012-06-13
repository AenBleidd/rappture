
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

#define SEGMENT_TYPE_TEXT	0
#define SEGMENT_TYPE_VARIABLE	1
#define SEGMENT_TYPE_HOTSPOT	2
#define VAR_FLAGS (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS)

#define REDRAW_PENDING		(1<<0)
#define LAYOUT_PENDING		(1<<1)
#define SUBST_PENDING		(1<<2)

#undef ROUND
#define ROUND(x) 	((int)(((double)(x)) + (((x)<0.0) ? -0.5 : 0.5)))

#define IMAGE_PAD	3

typedef struct _ItemSegment ItemSegment;
typedef struct _HotspotItem HotspotItem;

/*
 * ItemSegment 
 *
 */
struct _ItemSegment {
    HotspotItem *itemPtr;
    ItemSegment *nextPtr;
    int type;				/* Image, normal text, or substituted
					 * text. */
    short int x, y;			/* Location of segment on canvas. */
    short int width, height;		/* Dimension of segment. */
    Tk_TextLayout layout;		/* Layout of normal or substituted 
					 * text */
    int length;				/* # of bytes in below string. */
    int lineNum;			/* Line where segment if located. */
    char *subst;			/* Substitututed text. */
    char text[1];
};

/*
 * Record for each hotspot item:
 */
struct _HotspotItem  {
    Tk_Item base;			/* Generic stuff that's the same for
					 * all types.  MUST BE FIRST IN
					 * STRUCTURE. */
    unsigned int flags;
    Tcl_Interp *interp;			/* Interp that owns this item */
    Display *display;
    Tk_Window tkwin;			/* Window that represents this
					 * canvas */
    Tk_Canvas canvas;			/* Canvas that owns this item */

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
    Tcl_Interp *substInterp;		/* If non-NULL, interpreter containing
					 * substituted variables.   */
    const char *text;			/* Text to be displayed */
    Tk_Anchor anchor;			/* Where to anchor text relative to
					 * (x,y) */
    XColor *textColor;			/* Color for normal text */
    XColor *substColor;			/* Color for substitututed text */
    XColor *outlineColor;		/* Color for outline of rectangle */
    Tk_3DBorder border;			/* If non-NULL, background color of
					 * rectangle. */
    int borderWidth;			/* supports 3D border (drawn under
					 * -outline) */
    int relief;				/* Indicates whether hotspot as a
					 * whole is raised, sunken, or flat */
    Tk_Font font;			/* Font for drawing normal text. */
    Tk_Font substFont;			/* Font for drawing substituted text. */

    const char *imageName;		/* Name of normal hotspot image. */
    const char *activeImageName;	/* Name of active hotspot image. */
  
    Tk_Image image;			/* The normal icon for a hotspot. */
    Tk_Image activeImage;		/* If non-NULL, active icon. */

    /*
     * Fields whose values are derived from the current values of the
     * configuration settings above:
     */
    GC normalGC;			/* Graphics context for drawing
					 * text. */
    GC substGC;				/* GC for substituted text. */
    GC outlineGC;			/* GC for outline? */
    ItemSegment *firstPtr, *lastPtr;	/* List of hotspot segments. */
    int width, height;			/* Dimension of bounding box. */
    ItemSegment *activePtr;		/* Active segment. */
    short int maxImageWidth, maxImageHeight;
    int numLines;
};

/*
 * Information used for parsing configuration specs:
 */
static Tk_CustomOption tagsOption = {
    (Tk_OptionParseProc *) Tk_CanvasTagsParseProc,
    Tk_CanvasTagsPrintProc, (ClientData) NULL
};

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_STRING, "-activeimage", "activeImage", (char*)NULL, (char*)NULL, 
	Tk_Offset(HotspotItem, activeImageName), TK_CONFIG_NULL_OK},
    {TK_CONFIG_ANCHOR, "-anchor", "anchor", (char*)NULL,
        "center", Tk_Offset(HotspotItem, anchor),
        TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_BORDER, "-background", (char *)NULL, (char*)NULL,
        "", Tk_Offset(HotspotItem, border), TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", (char*)NULL,
        "0", Tk_Offset(HotspotItem, borderWidth), 0},
    {TK_CONFIG_BORDER, "-fill", (char *)NULL, (char*)NULL,
        "", Tk_Offset(HotspotItem, border), TK_CONFIG_NULL_OK},
    {TK_CONFIG_FONT, "-font", "font", (char*)NULL,
        "helvetica -12", Tk_Offset(HotspotItem, font), 0},
    {TK_CONFIG_COLOR, "-foreground", "foreground", (char*)NULL,
        "black", Tk_Offset(HotspotItem, textColor), 0},
    {TK_CONFIG_FONT, "-substitutionfont", (char*)NULL, (char*)NULL,
        "courier -12", Tk_Offset(HotspotItem, substFont), 0},
    {TK_CONFIG_COLOR, "-substitutionforeground", (char*)NULL, (char*)NULL,
        "blue", Tk_Offset(HotspotItem, substColor), 0},
    {TK_CONFIG_STRING, "-image", (char*)NULL, (char*)NULL, (char*)NULL, 
	Tk_Offset(HotspotItem, imageName), 0},
    {TK_CONFIG_COLOR, "-outline", (char*)NULL, (char*)NULL,
        "", Tk_Offset(HotspotItem, outlineColor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_RELIEF, "-relief", (char*)NULL, (char*)NULL,
        "flat", Tk_Offset(HotspotItem, relief), 0},
    {TK_CONFIG_CUSTOM, "-tags", (char*)NULL, (char*)NULL,
        (char*)NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_STRING, "-text", (char*)NULL, (char*)NULL,
        "", Tk_Offset(HotspotItem, text), 0},
    {TK_CONFIG_END, (char*)NULL, (char*)NULL, (char*)NULL,
        (char*)NULL, 0, 0}
};

/*
 * Prototypes for procedures defined in this file
 */
static Tk_ItemConfigureProc ConfigureProc;
static Tk_ItemCreateProc CreateProc;
static Tk_ItemDeleteProc DeleteProc;
static Tk_ItemDisplayProc DisplayProc;
static Tk_ItemScaleProc ScaleProc;
static Tk_ItemTranslateProc TranslateProc;
static Tk_ItemCoordProc CoordsProc;
static Tk_ItemAreaProc AreaProc;
static Tk_ItemPointProc PointProc;
static Tk_ItemPostscriptProc PostscriptProc;

static Tk_ImageChangedProc ImageChangedProc;

/*
 * The structures below defines the canvas item type:
 */
static Tk_ItemType hotspotType = {
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
    Tk_3DBorder border;		/* Used for canvas background. */
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

/* Forward declarations. */
static void ComputeBbox(HotspotItem *itemPtr);


static void
EventuallyRedraw(HotspotItem *itemPtr)
{
    if ((itemPtr->flags & REDRAW_PENDING) == 0) {
	itemPtr->flags |= REDRAW_PENDING;
	Tk_CanvasEventuallyRedraw(itemPtr->canvas, itemPtr->base.x1, 
		itemPtr->base.y1, itemPtr->base.x2, itemPtr->base.y2);
    }
}

static int
GetCoordFromObj(Tcl_Interp *interp, HotspotItem *itemPtr, Tcl_Obj *objPtr, 
		double *valuePtr)
{
    const char *string;

    string = Tcl_GetString(objPtr);
    return Tk_CanvasGetCoord(interp, itemPtr->canvas, string, valuePtr);
}



/*
 *---------------------------------------------------------------------------
 * 
 * TraceVarProc --
 *
 *	This procedure is invoked when someone changes the state variable
 *	associated with a radiobutton or checkbutton entry.  The entry's
 *	selected state is set to match the value of the variable.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The combobox entry may become selected or deselected.
 *
 *---------------------------------------------------------------------------
 */
static char *
TraceVarProc(
    ClientData clientData,		/* Information about the item. */
    Tcl_Interp *interp,			/* Interpreter containing variable. */
    const char *name1,			/* First part of variable's name. */
    const char *name2,			/* Second part of variable's name. */
    int flags)				/* Describes what just happened. */
{
    HotspotItem *itemPtr = clientData;

    if (flags & TCL_INTERP_DESTROYED) {
    	return NULL;			/* Interpreter is going away. */

    }
    /*
     * If the variable is being unset, then re-establish the trace.
     */
    if (flags & TCL_TRACE_UNSETS) {
	if (flags & TCL_TRACE_DESTROYED) {
	    Tcl_TraceVar(interp, name1, VAR_FLAGS, TraceVarProc, itemPtr);
	}
    }
    itemPtr->flags |= SUBST_PENDING;
    EventuallyRedraw(itemPtr);
    return NULL;			/* Done. */
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
    
    itemPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(itemPtr);
    return;
}

static void
AddTextSegment(HotspotItem *itemPtr, const char *text, int length)
{
    ItemSegment *segPtr;

    segPtr = (ItemSegment *)Tcl_Alloc(sizeof(ItemSegment) + length + 1);
    memset(segPtr, 0, sizeof(ItemSegment) + length + 1);
    segPtr->itemPtr = itemPtr;
    segPtr->lineNum = itemPtr->numLines;
    segPtr->type = SEGMENT_TYPE_TEXT;
    strncpy(segPtr->text, text, length);
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
	itemPtr->lastPtr = segPtr;
    }
}

static void
AddVariableSegment(HotspotItem *itemPtr, const char *varName, int length)
{
    ItemSegment *segPtr;
    Tcl_Interp *interp;

    segPtr = (ItemSegment *)Tcl_Alloc(sizeof(ItemSegment) + length + 1);
    memset(segPtr, 0, sizeof(ItemSegment) + length + 1);
    segPtr->itemPtr = itemPtr;
    segPtr->lineNum = itemPtr->numLines;
    segPtr->type = SEGMENT_TYPE_VARIABLE;
    memcpy(segPtr->text, varName, length);
    segPtr->text[length] = '\0';
    segPtr->length = length;
    interp = itemPtr->substInterp;
    if (interp == NULL) {
	interp = itemPtr->interp;
    }
    Tcl_TraceVar(interp, varName, VAR_FLAGS, TraceVarProc, itemPtr);
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
	itemPtr->lastPtr = segPtr;
    }
}

static void
AddHotspotSegment(HotspotItem *itemPtr)
{
    ItemSegment *segPtr;

    segPtr = (ItemSegment *)Tcl_Alloc(sizeof(ItemSegment));
    memset(segPtr, 0, sizeof(ItemSegment));
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
	itemPtr->lastPtr = segPtr;
    }
}

static void
DestroySegment(ItemSegment *segPtr)
{
    HotspotItem *itemPtr;

    itemPtr = segPtr->itemPtr;
    if (segPtr->layout != NULL) {
	Tk_FreeTextLayout(segPtr->layout);
    }
    if (segPtr->type == SEGMENT_TYPE_VARIABLE) {
	Tcl_UntraceVar(itemPtr->interp, segPtr->text, VAR_FLAGS, TraceVarProc, 
		itemPtr);
    }
    if (segPtr->subst != NULL) {
	Tcl_Free((char *)segPtr->subst);
    }
    Tcl_Free((char *)segPtr);
}

static void
DestroySegments(HotspotItem *itemPtr)
{
    ItemSegment *segPtr, *nextPtr;

    for (segPtr = itemPtr->firstPtr; segPtr != NULL; segPtr = nextPtr) {
	nextPtr = segPtr->nextPtr;
	DestroySegment(segPtr);
    }
    itemPtr->firstPtr = itemPtr->lastPtr = NULL;
}

static void
DoSubst(HotspotItem *itemPtr)
{
    ItemSegment *segPtr;
    Tcl_Interp *interp;

    itemPtr->flags &= ~SUBST_PENDING;
    interp = itemPtr->substInterp;
    if (interp == NULL) {
	interp = itemPtr->interp;
    }
    for (segPtr = itemPtr->firstPtr; segPtr != NULL; segPtr = segPtr->nextPtr) {
	const char *value;

	if (segPtr->type != SEGMENT_TYPE_VARIABLE) {
	    continue;
	}
	value = Tcl_GetVar(interp, segPtr->text, TCL_GLOBAL_ONLY);
	if (value == NULL) {
	    value = "???";
	}
	if (segPtr->subst != NULL) {
	    Tcl_Free((char *)segPtr->subst);
	}
	segPtr->subst = Tcl_Alloc(strlen(value) + 1);
	strcpy(segPtr->subst, value);
    }
    itemPtr->flags |= LAYOUT_PENDING;
}

static void
ComputeGeometry(HotspotItem *itemPtr)
{
    int x, y, w, h;
    ItemSegment *segPtr;
    int imgWidth, imgHeight;
    int maxWidth, maxHeight;
    Tk_Font tkFont;
    int justify;
    int wrapLength;
    int flags;

    justify = TK_JUSTIFY_LEFT;
    wrapLength = 0;
    flags = 0;
    /* Collect the individual segment sizes. */
    imgWidth = itemPtr->maxImageWidth + 2 * IMAGE_PAD;
    imgHeight = itemPtr->maxImageHeight + 2 * IMAGE_PAD;
    for (segPtr = itemPtr->firstPtr; segPtr != NULL; segPtr = segPtr->nextPtr) {
	if (segPtr->layout != NULL) {
	    Tk_FreeTextLayout(segPtr->layout);
	}
	switch (segPtr->type) {
	case SEGMENT_TYPE_HOTSPOT:
	    w = imgWidth, h = imgHeight;
	    break;
	case SEGMENT_TYPE_VARIABLE:
	    tkFont = (itemPtr->substFont != NULL) ? 
		itemPtr->substFont : itemPtr->font;
	    segPtr->layout = Tk_ComputeTextLayout(tkFont, segPtr->subst, 
		-1, wrapLength, justify, flags, &w, &h);
	    break;
	case SEGMENT_TYPE_TEXT:
	    segPtr->layout = Tk_ComputeTextLayout(itemPtr->font, 
		segPtr->text, -1, wrapLength, justify, flags, 
		&w, &h);
	    break;
	default:
	    Tcl_Panic("unknown hotspot segment type");
	}
	segPtr->width = w;
	segPtr->height = h;
#ifdef notdef
	fprintf(stderr, "ComputeGeometry: item=(%s) segment=%x (%s) type=%d w=%d h=%d lineNum=%d nextPtr=%x\n",
		itemPtr->text, segPtr, segPtr->text, segPtr->type, segPtr->width, 
		segPtr->height, segPtr->lineNum, segPtr->nextPtr);
#endif
    }
    /* Compute the relative positions of the segments. */
    x = y = 0;
    maxWidth = maxHeight = 0;
    for (segPtr = itemPtr->firstPtr; segPtr != NULL; /*empty*/) {
	int maxHeight, lineNum;
	ItemSegment *startPtr;

	maxHeight = 0;
	lineNum = segPtr->lineNum;
	startPtr = segPtr;
	x = 0;
	while ((segPtr != NULL) && (segPtr->lineNum == lineNum)) {
	    if (segPtr->height > maxHeight) {
		maxHeight = segPtr->height;
	    }
	    segPtr->x = x;
	    x += segPtr->width;
	    segPtr = segPtr->nextPtr;
	}
	if (x > maxWidth) {
	    maxWidth = x;
	}
	segPtr = startPtr;
	while ((segPtr != NULL) && (segPtr->lineNum == lineNum)) {
	    segPtr->y = y + (maxHeight - segPtr->height) / 2;
	    segPtr = segPtr->nextPtr;
	}
	y += maxHeight;
    }
    itemPtr->width = maxWidth;
    itemPtr->height = y;
}

static int
ParseDescription(Tcl_Interp *interp, HotspotItem *itemPtr, 
		const char *description)
{
    const char *p, *start;

    if (itemPtr->firstPtr != NULL) {
	DestroySegments(itemPtr);
    }
    start = description;
    itemPtr->numLines = 1;
    for (p = description; *p != '\0'; p++) {
	if ((p[0] == '$') && (p[1] == '{')) {
	    const char *varName;
	    const char *q;

	    if (p > start) {
		/* Save the leading text. */
		AddTextSegment(itemPtr, start, p - start);
		start = p;
	    }
	    /* Start of substitution variable. */
	    varName = p + 2;
	    for (q = varName; q != '\0'; q++) {
		if (*q == '}') {
		    AddVariableSegment(itemPtr, varName, q - varName);
		    AddHotspotSegment(itemPtr);
		    p = q;
		    start = p + 1;
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
    next:
	;
    }
    if (p > start) {
	/* Save the trailing text. */
	AddTextSegment(itemPtr, start, p - start);
    }
    itemPtr->flags |= SUBST_PENDING;
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
    double x, y;
    Tk_Window tkwin;

    tkwin = Tk_CanvasTkwin(canvas);
    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
	    Tk_PathName(tkwin), " create ", canvItemPtr->typePtr->name,
	    " x y ?options?\"", (char *)NULL);
	return TCL_ERROR;
    }

    /*
     * Carry out initialization that is needed in order to clean up after
     * errors during the the remainder of this procedure.
     */
    itemPtr->activeImage = NULL;
    itemPtr->activeImageName = NULL;
    itemPtr->activePtr = NULL;
    itemPtr->anchor = TK_ANCHOR_CENTER;
    itemPtr->border = NULL;
    itemPtr->borderWidth = 0;
    itemPtr->canvas = canvas;
    itemPtr->display = Tk_Display(tkwin);
    itemPtr->firstPtr = NULL;
    itemPtr->font = NULL;
    itemPtr->height = 0;
    itemPtr->image = NULL;
    itemPtr->imageName  = NULL;
    itemPtr->interp = interp;
    itemPtr->lastPtr = NULL;
    itemPtr->maxImageWidth = itemPtr->maxImageHeight = 0;
    itemPtr->normalGC = None;
    itemPtr->numLines = 0;
    itemPtr->outlineColor = NULL;
    itemPtr->outlineGC = None;
    itemPtr->relief = TK_RELIEF_FLAT;
    itemPtr->substColor = NULL;
    itemPtr->substFont = NULL;
    itemPtr->substGC = None;
    itemPtr->substInterp = interp;
    itemPtr->text = NULL;
    itemPtr->textColor = NULL;
    itemPtr->tkwin = tkwin;
    itemPtr->width = 0;
    itemPtr->x = itemPtr->y = 0.0;

    /* Process the arguments to fill in the item record. */
    if ((GetCoordFromObj(interp, itemPtr, objv[0], &x) != TCL_OK) ||
	(GetCoordFromObj(interp, itemPtr, objv[1], &y) != TCL_OK)) {
	DeleteProc(canvas, canvItemPtr, itemPtr->display);
	return TCL_ERROR;
    }
    itemPtr->x = x;
    itemPtr->y = y;
    ComputeBbox(itemPtr);
    if (ConfigureProc(interp, canvas, canvItemPtr, objc - 2, objv + 2, 0) 
	!= TCL_OK) {
	DeleteProc(canvas, canvItemPtr, itemPtr->display);
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
        Tcl_ListObjAppendElement(interp, listObjPtr,
		Tcl_NewDoubleObj(itemPtr->x));
        Tcl_ListObjAppendElement(interp, listObjPtr,
		Tcl_NewDoubleObj(itemPtr->y));
        Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (objc == 1) {
	int elc;
	Tcl_Obj **elv;

	if (Tcl_ListObjGetElements(interp, objv[0], &elc, &elv) != TCL_OK) {
	    return TCL_ERROR;
        }
        if ((GetCoordFromObj(interp, itemPtr, elv[0], &x) != TCL_OK) || 
	    (GetCoordFromObj(interp, itemPtr, elv[1], &y) != TCL_OK)) {
            return TCL_ERROR;
        }
    } else if (objc == 3) {
        if ((GetCoordFromObj(interp, itemPtr, objv[0], &x) != TCL_OK) ||
	    (GetCoordFromObj(interp, itemPtr, objv[1], &y) != TCL_OK)) {
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
    ComputeBbox(itemPtr);
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
    HotspotItem *itemPtr = (HotspotItem *)canvItemPtr;
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    Tk_Window tkwin;
    Tk_Image tkImage;

    tkwin = Tk_CanvasTkwin(canvas);
    if (Tk_ConfigureWidget(interp, tkwin, configSpecs, objc, 
	(CONST char **)objv, (char*)itemPtr, flags|TK_CONFIG_OBJS) != TCL_OK) {
        return TCL_ERROR;
    }

    /* Normal text GC. */
    mask = GCFont | GCForeground;
    gcValues.font = Tk_FontId(itemPtr->font);
    gcValues.foreground = itemPtr->textColor->pixel;
    newGC = Tk_GetGC(tkwin, mask, &gcValues);
    if (itemPtr->normalGC != None) {
        Tk_FreeGC(Tk_Display(tkwin), itemPtr->normalGC);
    }
    itemPtr->normalGC = newGC;

    /* Outline GC */
    if (itemPtr->outlineColor != NULL) {
	mask = GCLineWidth | GCForeground;
	gcValues.foreground = itemPtr->outlineColor->pixel;
	gcValues.line_width = 1;
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
	if (itemPtr->outlineGC != None) {
	    Tk_FreeGC(Tk_Display(tkwin), itemPtr->outlineGC);
	}
	itemPtr->outlineGC = newGC;
    }

    /* Substituted text GC. */
    mask = GCFont | GCForeground;
    if (itemPtr->substFont != NULL) {
	gcValues.font = Tk_FontId(itemPtr->substFont);
    } else {
	gcValues.font = Tk_FontId(itemPtr->font);
    }
    if (itemPtr->substColor != NULL) {
	gcValues.foreground = itemPtr->substColor->pixel;
    } else {
	gcValues.foreground = itemPtr->textColor->pixel;
    }
    newGC = Tk_GetGC(tkwin, mask, &gcValues);
    if (itemPtr->substGC != None) {
        Tk_FreeGC(Tk_Display(tkwin), itemPtr->substGC);
    }
    itemPtr->substGC = newGC;

    itemPtr->maxImageWidth = itemPtr->maxImageHeight = 0;
    /* Normal hotspot image. */
    if (itemPtr->imageName != NULL) {
	int iw, ih;

        tkImage = Tk_GetImage(interp, tkwin, itemPtr->imageName,
                ImageChangedProc, (ClientData)itemPtr);
        if (tkImage == NULL) {
            return TCL_ERROR;
        }
	Tk_SizeOfImage(tkImage, &iw, &ih);
	if (iw > itemPtr->maxImageWidth) {
	    itemPtr->maxImageWidth = iw;
	}
	if (ih > itemPtr->maxImageHeight) {
	    itemPtr->maxImageHeight = ih;
	}
    } else {
	tkImage = NULL;
    }
	
    if (itemPtr->image != NULL) {
        Tk_FreeImage(itemPtr->image);
    }
    itemPtr->image = tkImage;

    /* Active hotspot image. */
    if (itemPtr->activeImageName != NULL) {
	int iw, ih;
	
        tkImage = Tk_GetImage(interp, tkwin, itemPtr->activeImageName,
                ImageChangedProc, (ClientData)itemPtr);
        if (tkImage == NULL) {
            return TCL_ERROR;
        }
	Tk_SizeOfImage(tkImage, &iw, &ih);
	if (iw > itemPtr->maxImageWidth) {
	    itemPtr->maxImageWidth = iw;
	}
	if (ih > itemPtr->maxImageHeight) {
	    itemPtr->maxImageHeight = ih;
	}
    } else {
	tkImage = NULL;
    } 
    if (itemPtr->activeImage != NULL) {
        Tk_FreeImage(itemPtr->activeImage);
    }
    itemPtr->activeImage = tkImage;

    if (itemPtr->text != NULL) {
	if (ParseDescription(interp, itemPtr, itemPtr->text) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    ComputeBbox(itemPtr);
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
    if (itemPtr->substColor != NULL) {
        Tk_FreeColor(itemPtr->substColor);
    }
    if (itemPtr->outlineColor != NULL) {
        Tk_FreeColor(itemPtr->outlineColor);
    }
    if (itemPtr->text != NULL) {
        Tcl_Free((char *)itemPtr->text);
    }
    if (itemPtr->imageName != NULL) {
        Tcl_Free((char *)itemPtr->imageName);
    }
    if (itemPtr->activeImageName != NULL) {
        Tcl_Free((char *)itemPtr->activeImageName);
    }
    if (itemPtr->image != NULL ) {
        Tk_FreeImage(itemPtr->image);
    }
    if (itemPtr->activeImage != NULL ) {
        Tk_FreeImage(itemPtr->activeImage);
    }
    if (itemPtr->font != NULL) {
	Tk_FreeFont(itemPtr->font);
    }
    if (itemPtr->substFont != NULL) {
	Tk_FreeFont(itemPtr->substFont);
    }
    if (itemPtr->normalGC != None) {
        Tk_FreeGC(display, itemPtr->normalGC);
    }
    if (itemPtr->outlineGC != None) {
        Tk_FreeGC(display, itemPtr->outlineGC);
    }
    if (itemPtr->substGC != None) {
        Tk_FreeGC(display, itemPtr->substGC);
    }
    DestroySegments(itemPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * TranslateAnchor --
 *
 * 	Translate the coordinates of a given bounding box based upon the
 * 	anchor specified.  The anchor indicates where the given xy position
 * 	is in relation to the bounding box.
 *
 *  		nw --- n --- ne
 *  		|            |     x,y ---+
 *  		w   center   e      |     |
 *  		|            |      +-----+
 *  		sw --- s --- se
 *
 * Results:
 *	The translated coordinates of the bounding box are returned.
 *
 *---------------------------------------------------------------------------
 */
static void
TranslateAnchor(HotspotItem *itemPtr, int *xPtr, int *yPtr)
{
    int x, y;

    x = *xPtr, y = *yPtr;
    switch (itemPtr->anchor) {
    case TK_ANCHOR_NW:		/* Upper left corner */
	break;
    case TK_ANCHOR_W:		/* Left center */
	y -= (itemPtr->height / 2);
	break;
    case TK_ANCHOR_SW:		/* Lower left corner */
	y -= itemPtr->height;
	break;
    case TK_ANCHOR_N:		/* Top center */
	x -= (itemPtr->width / 2);
	break;
    case TK_ANCHOR_CENTER:	/* Centered */
	x -= (itemPtr->width / 2);
	y -= (itemPtr->height / 2);
	break;
    case TK_ANCHOR_S:		/* Bottom center */
	x -= (itemPtr->width / 2);
	y -= itemPtr->height;
	break;
    case TK_ANCHOR_NE:		/* Upper right corner */
	x -= itemPtr->width;
	break;
    case TK_ANCHOR_E:		/* Right center */
	x -= itemPtr->width;
	y -= (itemPtr->height / 2);
	break;
    case TK_ANCHOR_SE:		/* Lower right corner */
	x -= itemPtr->width;
	y -= itemPtr->height;
	break;
    }
    *xPtr = x;
    *yPtr = y;
}

/*
 * ------------------------------------------------------------------------
 *
 *  ComputeBbox --
 *
 *    This procedure is invoked to compute the bounding box of all the pixels
 *    that may be drawn as part of a text item.  In addition, it recomputes
 *    all of the geometry information used to display a text item or check for
 *    mouse hits.
 *
 *  Results:
 *    None.
 *
 *  Side effects:
 *    The fields x1, y1, x2, and y2 are updated in the base structure
 *    for itemPtr, and the linePtr structure is regenerated
 *    for itemPtr.
 *
 * ------------------------------------------------------------------------
 */
static void
ComputeBbox(HotspotItem *itemPtr)
{
    int x, y;

    x = ROUND(itemPtr->x);
    y = ROUND(itemPtr->y);
    TranslateAnchor(itemPtr,&x, &y);
    itemPtr->base.x1 = x;
    itemPtr->base.y1 = y;
    itemPtr->base.x2 = x + itemPtr->width;
    itemPtr->base.y2 = y + itemPtr->height;
}

static void 
DrawSegment(ItemSegment *segPtr, Drawable drawable, int x, int y)
{
    HotspotItem *itemPtr;

    itemPtr = segPtr->itemPtr;
    if (segPtr->type == SEGMENT_TYPE_HOTSPOT) {
	Tk_Image tkImage;
	int w, h;

	tkImage = NULL;
	if (itemPtr->activePtr == segPtr) {
	    tkImage = itemPtr->activeImage;
	}
	if (tkImage == NULL) {
	    tkImage = itemPtr->image;
	}
	Tk_SizeOfImage(tkImage, &w, &h);
	Tk_RedrawImage(tkImage, 0, 0, w, h, drawable, x + segPtr->x + IMAGE_PAD, 
		y + segPtr->y + IMAGE_PAD);
    } else {
	GC gc;

	gc = (segPtr->type == SEGMENT_TYPE_VARIABLE) ? 
	    itemPtr->substGC : itemPtr->normalGC;
	Tk_DrawTextLayout(Tk_Display(itemPtr->tkwin), drawable, gc,
		segPtr->layout, x + segPtr->x, y + segPtr->y, 0, -1);
    }
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
    ItemSegment *segPtr;
    short int cx, cy;

    itemPtr->flags &= ~REDRAW_PENDING;
    if (itemPtr->flags & SUBST_PENDING) {
	DoSubst(itemPtr);
    }
    if (itemPtr->flags & LAYOUT_PENDING) {
	ComputeGeometry(itemPtr);
    }
    Tk_CanvasDrawableCoords(canvas, itemPtr->x, itemPtr->y, &cx, &cy);
    x = cx, y = cy;
    TranslateAnchor(itemPtr, &x, &y);

    if (itemPtr->border != NULL) {
        Tk_Fill3DRectangle(itemPtr->tkwin, drawable, itemPtr->border, x, y, 
		itemPtr->width, itemPtr->height, itemPtr->borderWidth, 
		itemPtr->relief);
    }
    if (itemPtr->outlineColor != NULL) {
        XDrawRectangle(display, drawable, itemPtr->outlineGC, x, y, 
		itemPtr->width, itemPtr->height);
    }
    for (segPtr = itemPtr->firstPtr; segPtr != NULL; segPtr = segPtr->nextPtr) {
	fprintf(stderr, "Display: segment=(%s) type=%d x=%d y=%d (x=%d y=%d w=%d h=%d) lineNum=%d nextPtr=%x\n",
		segPtr->text, segPtr->type, x, y, segPtr->x, segPtr->y,
		segPtr->width, segPtr->height, segPtr->lineNum, segPtr->nextPtr);
	DrawSegment(segPtr, drawable, x, y);
    }
}

/*
 * ------------------------------------------------------------------------
 *
 *  PointProc --
 *
 *    Computes the distance from a given point to a given hotspot item, in
 *    canvas units.
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
    double x1, y1, x2, y2, dx, dy;

    x1 = itemPtr->x;
    y1 = itemPtr->y;
    x2 = itemPtr->x + itemPtr->width;
    y2 = itemPtr->y + itemPtr->height;
    if (pointPtr[0] < x1) {
        dx = x1 - pointPtr[0];
    } else if (pointPtr[0] >= x2)  {
        dx = pointPtr[0] + 1 - x2;
    } else {
        dx = 0;
    }
    if (pointPtr[1] < y1) {
        dy = y1 - pointPtr[1];
    } else if (pointPtr[1] >= y2)  {
        dy = pointPtr[1] + 1 - y2;
    } else {
        dy = 0;
    }
    return hypot(dx, dy);
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

    /* Compare against bounding box... */
    if ((rectPtr[0] > itemPtr->base.x2) || (rectPtr[1] > itemPtr->base.y2) || 
	(rectPtr[2] < itemPtr->base.x1) || (rectPtr[3] < itemPtr->base.y1)) {
        return -1;			/* Completely outside. */
    }
    if ((rectPtr[0] >= itemPtr->base.x1) && (rectPtr[1] >= itemPtr->base.y1) && 
	(rectPtr[2] <= itemPtr->base.x2) && (rectPtr[3] <= itemPtr->base.y2)) {
        return 1;			/* Completely inside. */
    }
    return 0;				/* Must be overlapping */
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

    itemPtr->x += scaleX * (itemPtr->x - x);
    itemPtr->y += scaleY * (itemPtr->y - y);
    ComputeBbox(itemPtr);
    return;
}

/*
 * ------------------------------------------------------------------------
 *
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
 *
 * ------------------------------------------------------------------------
 */
static void
TranslateProc(Tk_Canvas canvas, Tk_Item *canvItemPtr, double dx,  double dy)
{
    HotspotItem *itemPtr = (HotspotItem *)canvItemPtr;

    itemPtr->x += dx;
    itemPtr->y += dy;
    ComputeBbox(itemPtr);
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
 *
 * ------------------------------------------------------------------------
 */
static int
PostscriptProc(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *canvItemPtr, 
	       int prepass)
{
    HotspotItem *itemPtr = (HotspotItem *)canvItemPtr;
    Tk_Window canvasWin = Tk_CanvasTkwin(canvas);

#ifdef notdef
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

    if (Tk_CanvasPsFont(interp, canvas, itemPtr->font) != TCL_OK) {
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
	delta = (itemPtr->base.y2 - itemPtr->base.y1)/2 - itemPtr->imageLeftH/2;
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
      if (Tk_CanvasPsFont(interp, canvas, itemPtr->font) != TCL_OK) {
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

    Tk_GetFontMetrics(itemPtr->font, &fm);
    sprintf(buffer, "] %d %g %g %s %s DrawText\n",
            fm.linespace, x / -2.0, y / 2.0, justify,
            "false" /* stipple */);
    Tcl_AppendResult(interp, buffer, (char *) NULL);
#endif
    return TCL_OK;
}

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
    Tk_CreateItemType(&hotspotType);
    return TCL_OK;
}

