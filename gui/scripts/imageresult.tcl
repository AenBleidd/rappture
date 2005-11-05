# ----------------------------------------------------------------------
#  COMPONENT: imageresult - picture image in a ResultSet
#
#  This widget displays an image found in the output of a Rappture
#  tool run.  Use the "add" and "delete" methods to control the images
#  showing in the widget.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *ImageResult.width 3i widgetDefault
option add *ImageResult.height 3i widgetDefault
option add *ImageResult.controlBackground gray widgetDefault
option add *ImageResult.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

blt::bitmap define ImageResult-reset {
#define reset_width 12
#define reset_height 12
static unsigned char reset_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02,
   0x04, 0x02, 0x04, 0x02, 0x04, 0x02, 0xfc, 0x03, 0x00, 0x00, 0x00, 0x00};
}

blt::bitmap define ImageResult-zoomin {
#define zoomin_width 12
#define zoomin_height 12
static unsigned char zoomin_bits[] = {
   0x7c, 0x00, 0x82, 0x00, 0x11, 0x01, 0x11, 0x01, 0x7d, 0x01, 0x11, 0x01,
   0x11, 0x01, 0x82, 0x03, 0xfc, 0x07, 0x80, 0x0f, 0x00, 0x0f, 0x00, 0x06};
}

blt::bitmap define ImageResult-zoomout {
#define zoomout_width 12
#define zoomout_height 12
static unsigned char zoomout_bits[] = {
   0x7c, 0x00, 0x82, 0x00, 0x01, 0x01, 0x01, 0x01, 0x7d, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x82, 0x03, 0xfc, 0x07, 0x80, 0x0f, 0x00, 0x0f, 0x00, 0x06};
}


itcl::class Rappture::ImageResult {
    inherit itk::Widget

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {image {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method download {}

    protected method _rebuild {args}
    protected method _topimage {}
    protected method _zoom {option args}
    protected method _move {option args}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _dlist ""      ;# list of data objects
    private variable _topmost ""    ;# topmost image in _dlist
    private variable _max           ;# max size of all images
    private variable _scale         ;# info related to zoom
    private variable _image         ;# image buffers used for scaling
}
                                                                                
itk::usual ImageResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild [itcl::code $this _rebuild]

    array set _scale {
        max 1.0
        current 1.0
        x 0
        y 0
    }

    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add controls {
        frame $itk_interior.cntls
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(controls) -side right -fill y

    itk_component add reset {
        button $itk_component(controls).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap ImageResult-reset \
            -command [itcl::code $this _zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(controls).zin \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap ImageResult-zoomin \
            -command [itcl::code $this _zoom in]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomin) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
        button $itk_component(controls).zout \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap ImageResult-zoomout \
            -command [itcl::code $this _zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomout) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"


    set _image(zoom) [image create photo]
    set _image(final) [image create photo]

    itk_component add image {
        label $itk_interior.image -image $_image(final)
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(image) -expand yes -fill both

    #
    # Add bindings for resize/move
    #
    bind $itk_component(image) <Configure> \
        [list $_dispatcher event -idle !rebuild resize 1]

    bind $itk_component(image) <ButtonPress-1> \
        [itcl::code $this _move click %x %y]
    bind $itk_component(image) <B1-Motion> \
        [itcl::code $this _move drag %x %y]
    bind $itk_component(image) <ButtonRelease-1> \
        [itcl::code $this _move release %x %y]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::destructor {} {
    foreach name [array names _image] {
        image delete $_image($name)
    }
}

# ----------------------------------------------------------------------
# USAGE: add <image> ?<settings>?
#
# Clients use this to add an image to the plot.  The optional <settings>
# are used to configure the image.  Allowed settings are -color,
# -brightness, -width, -linestyle and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::add {image {settings ""}} {
    array set params {
        -color auto
        -brightness 0
        -width 1
        -raise 0
        -linestyle solid
    }
    foreach {opt val} $settings {
        if {![info exists params($opt)]} {
            error "bad setting \"$opt\": should be [join [lsort [array names params]] {, }]"
        }
        set params($opt) $val
    }

    if {$params(-raise)} {
        set _topmost $image
        $_dispatcher event -idle !rebuild
    }

    set pos [lsearch -exact $image $_dlist]
    if {$pos < 0} {
        lappend _dlist $image
        $_dispatcher event -idle !rebuild
    }
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of images being displayed, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::get {} {
    # put the dataobj list in order according to -raise options
    set dlist $_dlist

    set i [lsearch $_dlist $_topmost]
    if {$i >= 0} {
        set dlist [lreplace $dlist $i $i]
        set dlist [linsert $dlist 0 $_topmost]
    }
    return $dlist
}

# ----------------------------------------------------------------------
# USAGE: delete ?<image1> <image2> ...?
#
# Clients use this to delete an image from the plot.  If no images
# are specified, then all images are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified curves
    set changed 0
    foreach image $args {
        set pos [lsearch -exact $_dlist $image]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            set changed 1

            if {$image == $_topmost} {
                set _topmost ""
            }
        }
    }

    # if anything changed, then rebuild the plot
    if {$changed} {
        $_dispatcher event -idle !rebuild
    }
}

# ----------------------------------------------------------------------
# USAGE: scale ?<image1> <image2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <image> objects.  This
# accounts for all images--even those not showing on the screen.
# Because of this, the limits are appropriate for all images as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::scale {args} {
    set _max(w) 0
    set _max(h) 0
    foreach image $args {
        set imh [$image tkimage]

        set w [image width $imh]
        if {$w > $_max(w)} { set _max(w) $w }

        set h [image height $imh]
        if {$h > $_max(h)} { set _max(h) $h }
    }

    # scale is unknown for now... scale later at next _rebuild
    set _scale(max) "?"
    set _scale(current) "?"

    $_dispatcher event -idle !rebuild
}

# ----------------------------------------------------------------------
# USAGE: download
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::download {} {
    set top [_topimage]
    if {$top == ""} {
        return ""
    }
    return [list jpg [image data $top -format jpg]]
}

# ----------------------------------------------------------------------
# USAGE: _rebuild ?<eventData>...?
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::_rebuild {args} {
    array set event $args
    if {[info exists event(resize)] && $event(resize)} {
        # window changed size -- recompute max scale below
        set _scale(max) "?"
    }

    if {$_scale(max) == "?"} {
        _zoom rescale
    }
    if {$_scale(current) == "?"} {
        _zoom reset
    }

    set w [winfo width $itk_component(image)]
    set h [winfo height $itk_component(image)]
    $_image(final) configure -width $w -height $h
    set bg [$itk_component(image) cget -background]
    $_image(final) put $bg -to 0 0 $w $h

    set imh [_topimage]
    if {$imh != ""} {
        if {$_scale(current) <= 1.0} {
            set wz [expr {round($_scale(current)*$w)}]
            set hz [expr {round($_scale(current)*$h)}]
            if {$wz > 1 && $hz > 1} {
                $_image(zoom) configure -width $wz -height $hz
                $_image(zoom) put $bg -to 0 0 $wz $hz
                set sx [expr {round($_scale(x)*$_scale(current))}]
                set sy [expr {round($_scale(y)*$_scale(current))}]
                $_image(zoom) copy $imh -from $sx $sy
                blt::winop resample $_image(zoom) $_image(final) sinc
            }
        } else {
            set iw [image width $imh]
            set ih [image height $imh]
            set wz [expr {round(double($iw)/$_scale(current))}]
            set hz [expr {round(double($ih)/$_scale(current))}]
            if {$wz > 1 && $hz > 1} {
                $_image(zoom) configure -width $wz -height $hz
                $_image(zoom) put $bg -to 0 0 $wz $hz
                blt::winop resample $imh $_image(zoom) sinc
                $_image(final) copy $_image(zoom) -from $_scale(x) $_scale(y)
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _topimage
#
# Used internally to get the topmost image currently being displayed.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::_topimage {} {
    set top $_topmost
    if {"" == $top} {
        set top [lindex $_dlist 0]
    }
    if {"" != $top} {
        return [$top tkimage]
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: _zoom reset
# USAGE: _zoom in
# USAGE: _zoom out
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::_zoom {option args} {
    switch -- $option {
        rescale {
            # empty list? then reset w/h max size
            if {[llength $_dlist] == 0} {
                set _max(w) 0
                set _max(h) 0
                set _scale(max) 1.0
            } else {
                set w [winfo width $itk_component(image)]
                set h [winfo height $itk_component(image)]

                set wfac [expr {$_max(w)/double($w)}]
                set hfac [expr {$_max(h)/double($h)}]
                set _scale(max) [expr {($wfac > $hfac) ? $wfac : $hfac}]
            }
        }
        reset {
            set _scale(current) $_scale(max)
            set _scale(x) 0
            set _scale(y) 0
        }
        in {
            set _scale(current) [expr {$_scale(current)*0.5}]
        }
        out {
            set w [winfo width $itk_component(image)]
            set h [winfo height $itk_component(image)]
            if {$_max(w)/$_scale(current) > $w
                  || $_max(h)/$_scale(current) > $h} {
                # must be room left to zoom -- zoom out, but not beyond max
                set _scale(current) [expr {$_scale(current)*2.0}]
                if {$_scale(current) < $_scale(max)} {
                    set _scale(current) $_scale(max)
                }
            } else {
                # no room left to zoom -- zoom out max
                if {$_scale(max) < 1} {
                    set _scale(current) 1
                } else {
                    set _scale(current) $_scale(max)
                }
            }
        }
    }
    $_dispatcher event -idle !rebuild
}

# ----------------------------------------------------------------------
# USAGE: _move click <x> <y>
# USAGE: _move drag <x> <y>
# USAGE: _move release <x> <y>
#
# Called automatically when the user clicks and drags on the image
# to pan the view.  Adjusts the (x,y) offset for the scaling info
# and redraws the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::_move {option args} {
    switch -- $option {
        click {
            foreach {x y} $args break
            $itk_component(image) configure -cursor fleur
            set _scale(x0) $_scale(x)
            set _scale(y0) $_scale(y)
            set _scale(xclick) $x
            set _scale(yclick) $y
        }
        drag {
            foreach {x y} $args break
            if {[info exists _scale(xclick)] && [info exists _scale(yclick)]} {
                set w [winfo width $itk_component(image)]
                set h [winfo height $itk_component(image)]
                set wx [expr {round($_max(w)/$_scale(current))}]
                set hy [expr {round($_max(h)/$_scale(current))}]
                if {$wx > $w || $hy > $h} {
                    set x [expr {$_scale(x0)-$x+$_scale(xclick)}]
                    if {$x > $wx-$w} {set x [expr {$wx-$w}]}
                    if {$x < 0} {set x 0}

                    set y [expr {$_scale(y0)-$y+$_scale(yclick)}]
                    if {$y > $hy-$h} {set y [expr {$hy-$h}]}
                    if {$y < 0} {set y 0}

                    set _scale(x) $x
                    set _scale(y) $y
                } else {
                    set _scale(x) 0
                    set _scale(y) 0
                }
                $_dispatcher event -idle !rebuild
            }
        }
        release {
            eval _move drag $args
            $itk_component(image) configure -cursor ""
            catch {unset _scale(xclick)}
            catch {unset _scale(yclick)}
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
}
