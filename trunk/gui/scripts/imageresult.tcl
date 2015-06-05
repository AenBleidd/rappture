# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: imageresult - picture image in a ResultSet
#
#  This widget displays an image found in the output of a Rappture
#  tool run.  Use the "add" and "delete" methods to control the images
#  showing in the widget.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT
package require Img

option add *ImageResult.width 3i widgetDefault
option add *ImageResult.height 3i widgetDefault
option add *ImageResult.controlBackground gray widgetDefault
option add *ImageResult.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::ImageResult {
    inherit itk::Widget

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {image {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method parameters {title args} { # do nothing }
    public method download {option args}

    protected method _rebuild {args}
    protected method _top {what}
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
        default 1
        x 0
        y 0
    }

    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    Rappture::Panes $itk_interior.panes \
        -sashwidth 2 -sashrelief solid -sashpadding 1

    pack $itk_interior.panes -expand yes -fill both
    set main [$itk_interior.panes pane 0]
    $itk_interior.panes fraction 0 1

    itk_component add controls {
        frame $main.cntls
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(controls) -side right -fill y

    itk_component add reset {
        button $itk_component(controls).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon reset] \
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
            -bitmap [Rappture::icon zoomin] \
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
            -bitmap [Rappture::icon zoomout] \
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
        label $main.image -image $_image(final) -anchor c
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

    #
    # Add area at the bottom for notes.
    #
    set notes [$itk_interior.panes insert end -fraction 0.15]
    $itk_interior.panes visibility 1 off
    Rappture::Scroller $notes.scr -xscrollmode auto -yscrollmode auto
    pack $notes.scr -expand yes -fill both
    itk_component add notes {
        Rappture::HTMLviewer $notes.scr.html
    }
    $notes.scr contents $notes.scr.html

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
        -description ""
        -param ""
    }
    array set params $settings

    if {$params(-raise)} {
        set _topmost $image
        $_dispatcher event -idle !rebuild
    }

    set pos [lsearch -exact $_dlist $image]
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
# USAGE: download coming
# USAGE: download controls <downloadCommand>
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::download {option args} {
    switch $option {
        coming {
            # nothing to do
        }
        controls {
            # no controls for this download yet
            return ""
        }
        now {
            set top [_top image]
            if {$top == ""} {
                return ""
            }
            # Get the image data (as base64) and decode it back to binary.
            # This is better than writing to temporary files.  When we switch
            # to the BLT picture image it won't be necessary to decode the
            # image data.
            set bytes [$top data -format "jpeg -quality 100"]
            set bytes [Rappture::encoding::decode -as b64 $bytes]
            return [list .jpg $bytes]
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
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
        if {![_zoom rescale]} {
            return
        }
    }
    if {$_scale(current) == "?" || $_scale(default)} {
        set _scale(current) $_scale(max)
        set _scale(x) 0.5
        set _scale(y) 0.5
    }

    set w [winfo width $itk_component(image)]
    set h [winfo height $itk_component(image)]
    set bg [$itk_component(image) cget -background]

    set imh [_top image]
    if {$imh != ""} {
        set iw [image width $imh]
        set ih [image height $imh]
        set wz [expr {round($w*$_scale(current))}]
        set hz [expr {round($h*$_scale(current))}]

        if {$wz < $iw || $hz < $ih} {
            #
            # Scale the image up by creating a "zoom" image which
            # is smaller than the current image.  Sample a small
            # part of the original image by copying into the "zoom"
            # image, then scale that part up to the full "view" area.
            #
            if {$wz > $iw} {
                set wz $iw
            }
            if {$hz > $ih} {
                set hz $ih
            }

            set sx [expr {round($_scale(x)*$_max(w)-0.5*$wz)}]
            if {$sx+$wz > $iw} {
                set sx [expr {$iw-$wz}]
            }
            if {$sx < 0} {
                set sx 0
            }

            set sy [expr {round($_scale(y)*$_max(h)-0.5*$hz)}]
            if {$sy+$hz > $ih} {
                set sy [expr {$ih-$hz}]
            }
            if {$sy < 0} {
                set sy 0
            }

            if {$wz > 1 && $hz > 1} {
                $_image(zoom) configure -width $wz -height $hz
                set wf [expr {round(double($wz)/$_scale(current))}]
                set hf [expr {round(double($hz)/$_scale(current))}]
                $_image(final) configure -width $wf -height $hf
                $_image(zoom) copy $imh -from $sx $sy
                blt::winop resample $_image(zoom) $_image(final) sinc
            }
        } else {
            #
            # Scale the image down by creating a "zoom" image which
            # is smaller than the current image.  Resize the original
            # image to the smaller size, then copy into the current
            # view.
            #
            set wz [expr {round(double($iw)/$_scale(current))}]
            set hz [expr {round(double($ih)/$_scale(current))}]
            if {$wz > 1 && $hz > 1} {
                $_image(zoom) configure -width $wz -height $hz
                $_image(zoom) put $bg -to 0 0 $wz $hz
                blt::winop resample $imh $_image(zoom) sinc

                $_image(final) configure -width $wz -height $hz
                $_image(final) copy $_image(zoom) -from 0 0
            }
        }
    }

    set note [_top note]
    if {[string length $note] > 0} {
        if {[regexp {^html://} $note]} {
            set note [string range $note 7 end]
        } else {
            regexp {&} $note {\007} note
            regexp {<} $note {\&lt;} note
            regexp {>} $note {\&gt;} note
            regexp {\007} $note {\&amp;} note
            regexp "\n\n" $note {<br/>} note
            set note "<html><body>$note</body></html>"
        }
        set notes [$itk_interior.panes pane 1]
        $itk_component(notes) load $note -in [file join [_top tooldir] docs]
        $itk_interior.panes visibility 1 on
    } else {
        $itk_interior.panes visibility 1 off
    }
}

# ----------------------------------------------------------------------
# USAGE: _top image|note|tooldir
#
# Used internally to get the topmost image currently being displayed.
# ----------------------------------------------------------------------
itcl::body Rappture::ImageResult::_top {option} {
    set top $_topmost
    if {"" == $top} {
        set top [lindex $_dlist 0]
    }
    if {"" != $top} {
        switch -- $option {
            image   { return [$top tkimage] }
            note    { return [$top hints note] }
            tooldir { return [$top hints tooldir] }
            default { error "bad option \"$option\": should be image, note, tooldir" }
        }
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
                if {$w == 1 && $h == 1} {
                    return 0
                }

                if {$w < $h} {
                    if {$_max(w)/double($_max(h)) > $w/double($h)} {
                        set _scale(max) [expr {$_max(w)/double($w)}]
                    } else {
                        set _scale(max) [expr {$_max(h)/double($h)}]
                    }
                } else {
                    if {$_max(w)/double($_max(h)) < $w/double($h)} {
                        set _scale(max) [expr {$_max(h)/double($h)}]
                    } else {
                        set _scale(max) [expr {$_max(w)/double($w)}]
                    }
                }
            }
            return 1
        }
        reset {
            set _scale(current) $_scale(max)
            set _scale(default) 1
            set _scale(x) 0.5
            set _scale(y) 0.5
            Rappture::Logger::log image zoom -reset
        }
        in {
            set _scale(current) [expr {$_scale(current)*0.8}]
            set _scale(default) 0
            Rappture::Logger::log image zoom -in
        }
        out {
            set w [winfo width $itk_component(image)]
            set h [winfo height $itk_component(image)]
            if {$_max(w)/$_scale(current) > $w
                  || $_max(h)/$_scale(current) > $h} {
                # must be room left to zoom -- zoom out, but not beyond max
                set _scale(current) [expr {$_scale(current)*1.25}]
                if {$_scale(current) > $_scale(max)} {
                    set _scale(current) $_scale(max)
                }
            } else {
                # no room left to zoom -- zoom out max
                set _scale(current) $_scale(max)
            }

            # fix the center of view, in case it is now out of bounds
            if {$_scale(current) > 1.0} {
                set _scale(x) 0.5
                set _scale(y) 0.5
            }

            # keep this zoom setting now that we've zoomed out
            set _scale(default) 0

            Rappture::Logger::log image zoom -out
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
                    set x [expr {$_scale(x0)-($x-$_scale(xclick))/double($wx)}]
                    set y [expr {$_scale(y0)-($y-$_scale(yclick))/double($hy)}]
                    if {$x*$_max(w) < 0.5*$w*$_scale(current)} {
                        set x [expr {0.5*$w*$_scale(current)/$_max(w)}]
                    }
                    if {$x*$_max(w) > $_max(w) - 0.5*$w*$_scale(current)} {
                        set x [expr {1 - 0.5*$w*$_scale(current)/$_max(w)}]
                    }
                    if {$y*$_max(h) < 0.5*$h*$_scale(current)} {
                        set y [expr {0.5*$h*$_scale(current)/$_max(h)}]
                    }
                    if {$y*$_max(h) > $_max(h) - 0.5*$h*$_scale(current)} {
                        set y [expr {1 - 0.5*$h*$_scale(current)/$_max(h)}]
                    }
                    set _scale(x) $x
                    set _scale(y) $y
                } else {
                    set _scale(x) 0.5
                    set _scale(y) 0.5
                }
                $_dispatcher event -idle !rebuild
            }
        }
        release {
            eval _move drag $args
            $itk_component(image) configure -cursor ""
            catch {unset _scale(xclick)}
            catch {unset _scale(yclick)}

            Rappture::Logger::log image moveto $_scale(x),$_scale(y)
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
}
