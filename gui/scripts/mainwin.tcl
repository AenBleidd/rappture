# ----------------------------------------------------------------------
#  COMPONENT: mainwin - main application window for Rappture
#
#  This widget acts as the main window for a Rappture application.
#  It can be configured to run in two modes:  1) normal desktop
#  application, and 2) web-based application.  In web-based mode,
#  the application window runs inside a VNC window, and it takes
#  the full screen and blends in with the web page.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

option add *MainWin.mode desktop widgetDefault
option add *MainWin.borderWidth 0 widgetDefault
option add *MainWin.relief raised widgetDefault
option add *MainWin.anchor center widgetDefault
option add *MainWin.titleFont \
    -*-helvetica-bold-o-normal-*-*-140-* widgetDefault

itcl::class Rappture::MainWin {
    inherit itk::Toplevel

    itk_option define -mode mode Mode ""
    itk_option define -anchor anchor Anchor "center"
    itk_option define -bgscript bgScript BgScript ""

    constructor {args} { # defined below }

    public method draw {option args}

    protected method _redraw {}

    private variable _contents ""  ;# frame containing app
    private variable _bgscript ""  ;# script of background drawing cmds
    private variable _bgparser ""  ;# parser for bgscript
}
                                                                                
itk::usual MainWin {
    keep -background -cursor foreground -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MainWin::constructor {args} {
    itk_component add area {
        canvas $itk_interior.area
    } {
        usual
        rename -background -bgcolor bgColor Background
    }
    pack $itk_component(area) -expand yes -fill both
    bind $itk_component(area) <Configure> [itcl::code $this _redraw]

    itk_component add app {
        frame $itk_component(area).app
    } {
        usual
        keep -borderwidth -relief
    }
    bind $itk_component(app) <Configure> "
        after cancel [itcl::code $this _redraw]
        after idle [itcl::code $this _redraw]
    "

    itk_component add menu {
        menu $itk_interior.menu
    }
    itk_component add filemenu {
        menu $itk_component(menu).file
    }
    $itk_component(menu) add cascade -label "File" -underline 0 \
        -menu $itk_component(filemenu)
    $itk_component(filemenu) add command -label "Exit" -underline 1 \
        -command exit

    #
    # Create a parser for the -bgscript option that can
    # execute drawing commands on the canvas.  This allows
    # us to draw a background that blends in with web pages.
    #
    set _bgparser [interp create -safe]
    $_bgparser alias rectangle [itcl::code $this draw rectangle]
    $_bgparser alias oval [itcl::code $this draw oval]
    $_bgparser alias line [itcl::code $this draw line]
    $_bgparser alias polygon [itcl::code $this draw polygon]
    $_bgparser alias text [itcl::code $this draw text]
    $_bgparser alias image [itcl::code $this draw image]

    eval itk_initialize $args

    bind RapptureMainWin <Destroy> { exit }
    set btags [bindtags $itk_component(hull)]
    bindtags $itk_component(hull) [lappend btags RapptureMainWin]
}

# ----------------------------------------------------------------------
# USAGE: draw <option> ?<arg> <arg>...?
#
# Used by the -bgscript to draw items in the background area behind
# the app when "-mode web" is active.  This allows an application
# to create a background that blends seamlessly with the underlying
# web page.
# ----------------------------------------------------------------------
itcl::body Rappture::MainWin::draw {option args} {
    set w $itk_component(hull)
    regsub -all {<w>} $args [winfo screenwidth $w] args
    regsub -all {<h>} $args [winfo screenheight $w] args
    eval $itk_component(area) create $option $args
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to redraw the widget whenever it changes size.
# This matters only when "-mode web" is active, when the background
# area is actually visible.
# ----------------------------------------------------------------------
itcl::body Rappture::MainWin::_redraw {} {
    $itk_component(area) delete all
    if {$itk_option(-mode) == "web"} {
        if {[catch {$_bgparser eval $itk_option(-bgscript)} result]} {
            bgerror "$result\n    (while redrawing application background)"
        }

        set sw [winfo width $itk_component(area)]
        set sh [winfo height $itk_component(area)]

        set clip 0
        set w [winfo reqwidth $itk_component(app)]
        set h [winfo reqheight $itk_component(app)]
        if {$w > $sw} {
            set $w $sw
            set clip 1
        }

        set titleh 0
        set fnt [option get $itk_component(hull) titleFont Font]
        if {$itk_option(-title) != "" && $fnt != ""} {
            set titleh [expr {[font metrics $fnt -linespace]+2}]
        }
        if {$h+$titleh > $sh} {
            set $h [expr {$sh-$titleh}]
            set clip 1
        }

        switch -- $itk_option(-anchor) {
            n {
                set x [expr {$sw/2}]
                set y $titleh
                set tx [expr {$x-$w/2}]
                set ty $titleh
            }
            s {
                set x [expr {$sw/2}]
                set y $sh
                set tx [expr {$x-$w/2}]
                set ty [expr {$sh-$h}]
            }
            center {
                set x [expr {$sw/2}]
                set y [expr {$sh/2}]
                set tx [expr {$x-$w/2}]
                set ty [expr {$y-$h/2}]
            }
            w {
                set x 0
                set y [expr {$sh/2}]
                set tx 0
                set ty [expr {$y-$h/2}]
            }
            e {
                set x $sw
                set y [expr {$sh/2}]
                set tx [expr {$sw-$w}]
                set ty [expr {$y-$h/2}]
            }
            nw {
                set x 0
                set y $titleh
                set tx 0
                set ty $titleh
            }
            ne {
                set x $sw
                set y $titleh
                set tx [expr {$sw-$w}]
                set ty $titleh
            }
            sw {
                set x 0
                set y $sh
                set tx 0
                set ty [expr {$sh-$h}]
            }
            se {
                set x $sw
                set y $sh
                set tx [expr {$sw-$w}]
                set ty [expr {$sh-$h}]
            }
        }

        # if the app is too big, use w/h. otherwise, 0,0 for default size
        if {!$clip} {
            set w 0
            set h 0
        }

        $itk_component(area) create window $x $y \
            -anchor $itk_option(-anchor) -window $itk_component(app) \
            -width $w -height $h

        if {$itk_option(-title) != "" && $fnt != ""} {
            $itk_component(area) create text $tx [expr {$ty-2}] \
                -anchor sw -text $itk_option(-title) -font $fnt
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -mode
# ----------------------------------------------------------------------
itcl::configbody Rappture::MainWin::mode {
    switch -- $itk_option(-mode) {
        desktop {
            component hull configure -menu $itk_component(hull).menu
            pack $itk_component(app) -expand yes -fill both
            wm geometry $itk_component(hull) ""
        }
        web {
            component hull configure -menu ""
            pack forget $itk_component(app)
            set wx [winfo screenwidth $itk_component(hull)]
            set wy [winfo screenheight $itk_component(hull)]
            wm geometry $itk_component(hull) ${wx}x${wy}+0+0
            _redraw
        }
        default {
            error "bad value \"$itk_option(-mode)\": should be desktop or web"
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -bgscript
# ----------------------------------------------------------------------
itcl::configbody Rappture::MainWin::bgscript {
    _redraw
}

# ----------------------------------------------------------------------
# OPTION: -anchor
# ----------------------------------------------------------------------
itcl::configbody Rappture::MainWin::anchor {
    if {[lsearch {n s e w ne nw se sw center} $itk_option(-anchor)] < 0} {
        error "bad anchor \"$itk_option(-anchor)\""
    }
    _redraw
}
