# -*- mode: tcl; indent-tabs-mode: nil -*-
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
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *MainWin.mode desktop widgetDefault
option add *MainWin.borderWidth 0 widgetDefault
option add *MainWin.relief raised widgetDefault
option add *MainWin.anchor center widgetDefault
option add *MainWin.titleFont \
    -*-helvetica-bold-o-normal-*-14-* widgetDefault

#
# Tk text widget doesn't honor Ctrl-V by default.  Get rid
# of the default binding so that Ctrl-V works for <<Paste>>
# as expected.
#
bind Text <Control-KeyPress-v> {}

#
# Fix the built-in <<Paste>> bindings to work properly even
# for the X11 windowing system.  By default, Tk won't replace
# selected text in X11.  What kind of stupid nonsense is that?
#
bind Entry <<Paste>> {
    catch {
        # always replace existing selection
        catch { %W delete sel.first sel.last }

        %W insert insert [::tk::GetSelection %W CLIPBOARD]
        tk::EntrySeeInsert %W
    }
}
proc ::tk_textPaste w {
    global tcl_platform
    if {![catch {::tk::GetSelection $w CLIPBOARD} sel]} {
        if {[catch {$w cget -autoseparators} oldSeparator]} {
            # in case we're using an older version of Tk
            set oldSeparator 0
        }
        if { $oldSeparator } {
            $w configure -autoseparators 0
            $w edit separator
        }

        # always replace existing selection
        catch { $w delete sel.first sel.last }
        $w insert insert $sel

        if { $oldSeparator } {
            $w edit separator
            $w configure -autoseparators 1
        }
    }
}

# ======================================================================
itcl::class Rappture::MainWin {
    inherit itk::Toplevel

    itk_option define -mode mode Mode ""
    itk_option define -anchor anchor Anchor "center"
    itk_option define -bgscript bgScript BgScript ""

    constructor {args} { # defined below }

    public method draw {option args}
    public method syncCutBuffer {option args}

    protected method _redraw {}

    private variable _contents ""  ;# frame containing app
    private variable _sync         ;# to sync current selection and cut buffer
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
        -command {destroy .}

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

    set _sync(cutbuffer) ""
    set _sync(selection) ""

    #
    # We used to do this to make "copy/paste with desktop" work
    # properly.  Well, it never really worked *properly*, but
    # it was an attempt.  We might as well skip it.  We use
    # the importfile/exportfile stuff now.
    #
    ##global tcl_platform
    ##if {$tcl_platform(platform) == "unix"} {
    ##    # this sync stuff works only for X windows
    ##    blt::cutbuffer set ""
    ##    syncCutBuffer ifneeded
    ##}
}

# ----------------------------------------------------------------------
# USAGE: syncCutBuffer ifneeded
# USAGE: syncCutBuffer transfer <offset> <maxchars>
# USAGE: syncCutBuffer lostselection
#
# Invoked automatically whenever the mouse pointer enters or leaves
# a main window to sync the cut buffer with the primary selection.
# This helps applications work properly with the "Copy/Paste with
# Desktop" option on the VNC applet for the nanoHUB.
#
# The "ifneeded" option syncs the cutbuffer and the primary selection
# if either one has new data.
#
# The "fromselection" option syncs from the primary selection to the
# cut buffer.  If there's a primary selection, it gets copied to the
# cut buffer.
# ----------------------------------------------------------------------
itcl::body Rappture::MainWin::syncCutBuffer {option args} {
    set mainwin $itk_component(hull)
    switch -- $option {
        ifneeded {
            #
            # See if the incoming cut buffer has changed.
            # If so, then sync the new input to the primary selection.
            #
            set s [blt::cutbuffer get]
            if {"" != $s && ![string equal $s $_sync(cutbuffer)]} {
                #
                # Convert any \r's in the cutbuffer to \n's.
                #
                if {[string first "\r" $s] >= 0} {
                    regsub -all "\r\n" $s "\n" s
                    regsub -all "\r" $s "\n" s
                    blt::cutbuffer set $s
                }

                set _sync(cutbuffer) $s

                if {![string equal $s $_sync(selection)]
                      && [selection own -selection PRIMARY] != $mainwin} {
                    set _sync(selection) $s

                    clipboard clear
                    clipboard append -- $s
                    selection handle -selection PRIMARY $mainwin \
                        [itcl::code $this syncCutBuffer transfer]
                    selection own -selection PRIMARY -command \
                        [itcl::code $this syncCutBuffer lostselection] \
                        $mainwin
                }
            }

            #
            # See if the selection has changed.  If so, then sync
            # the new input to the cut buffer, so it's available
            # outside the VNC client.
            #
            if {[catch {selection get -selection PRIMARY} s]} {
                set s ""
            }
            if {"" != $s && ![string equal $s $_sync(selection)]} {
                set _sync(selection) $s
                blt::cutbuffer set $s
            }

            # do this again soon
            after 1000 [itcl::code $this syncCutBuffer ifneeded]
        }
        transfer {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"syncCutBuffer transfer offset max\""
            }
            set offset [lindex $args 0]
            set maxchars [lindex $args 1]
            return [string range $_currseln $offset [expr {$offset+$maxchars-1}]]
        }
        lostselection {
            # nothing to do
        }
        default {
            error "bad option \"$option\": should be ifneeded, transfer, or lostselection"
        }
    }
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

        set bd 0  ;# optional border
        set sw [winfo width $itk_component(area)]
        set sh [winfo height $itk_component(area)]

        set clip 0
        set w [winfo reqwidth $itk_component(app)]
        set h [winfo reqheight $itk_component(app)]
        if {$w > $sw-2*$bd} {
            set $w [expr {$sw-2*$bd}]
            set clip 1
        }

        set anchor $itk_option(-anchor)
        switch -- $itk_option(-anchor) {
            n {
                set x [expr {$sw/2}]
                set y $bd
            }
            s {
                set x [expr {$sw/2}]
                set y [expr {$sh-$bd}]
            }
            center {
                set x [expr {$sw/2}]
                set y [expr {$sh/2}]
            }
            w {
                set x $bd
                set y [expr {$sh/2}]
            }
            e {
                set x [expr {$sw-$bd}]
                set y [expr {$sh/2}]
            }
            nw {
                set x $bd
                set y $bd
            }
            ne {
                set x [expr {$sw-$bd}]
                set y $bd
            }
            sw {
                set x $bd
                set y [expr {$sh-$bd}]
            }
            se {
                set x [expr {$sw-$bd}]
                set y [expr {$sh-$bd}]
            }
            fill {
                set anchor nw
                set x $bd
                set y $bd
                set w [expr {$sw-2*$bd}]
                set h [expr {$sh-2*$bd}]
                set clip 1
            }
        }

        # if the app is too big, use w/h. otherwise, 0,0 for default size
        if {!$clip} {
            set w 0
            set h 0
        }

        $itk_component(area) create window $x $y \
            -anchor $anchor -window $itk_component(app) \
            -width $w -height $h
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
    if {[lsearch {n s e w ne nw se sw center fill} $itk_option(-anchor)] < 0} {
        error "bad anchor \"$itk_option(-anchor)\""
    }
    _redraw
}
