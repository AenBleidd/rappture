# ----------------------------------------------------------------------
#  COMPONENT: objpath - show object path and copy/rename
#
#  This widget is used within the header above the object options
#  panel.  It shows the object name, but includes a button for things
#  like "rename" to change the object ID, and "copy" to copy the
#  path and paste into another widget.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *ObjPath.selectBackground cyan widgetDefault
option add *ObjPath.pathFont {helvetica -12} widgetDefault

itcl::class Rappture::ObjPath {
    inherit itk::Widget

    itk_option define -label label Label ""
    itk_option define -pathtext pathText PathText ""
    itk_option define -selectbackground selectBackground Foreground ""
    itk_option define -renamecommand renameCommand RenameCommand ""

    constructor {args} { # defined below }

    protected method _redraw {args}
    protected method _resize {args}
    protected method _hilite {op}
    protected method _copy {}
    protected method _edit {option args}

    private variable _dispatcher ""  ;# dispatcher for !events
}

itk::usual ObjPath {
    keep -cursor -font -foreground -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ObjPath::constructor {args} {
    # create a dispatcher for events
    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw [itcl::code $this _redraw]

    itk_component add size {
        frame $itk_interior.size -width 20
    }
    pack propagate $itk_component(size) no
    pack $itk_component(size) -side left -expand yes -fill both

    itk_component add path {
        text $itk_component(size).path -width 1 -height 1 \
            -borderwidth 0 -highlightthickness 0
    } {
        usual
        rename -font -pathfont pathFont Font
        ignore -highlightthickness -highlightbackground -highlightcolor
    }
    pack $itk_component(path) -expand yes -fill both

    # bind to <Expose> instead of <Configure> so we get events even after
    # we've reached the maximum width size, so we can scale down smaller
    bind $itk_component(path) <Expose> [itcl::code $this _resize]
    bindtags $itk_component(path) [list $itk_component(path) [winfo toplevel $itk_component(path)] all]

    # binding so when you click on the path, it becomes highlighted
    $itk_component(path) tag bind path <ButtonPress> [itcl::code $this _hilite toggle]

    # add the button for Rename/Copy
    itk_component add button {
        button $itk_interior.btn -width 6 -text "Rename" \
            -command [itcl::code $this _edit start]
    } {
        usual
        ignore -font -borderwidth -relief
    }
    pack $itk_component(button) -side left

    # create a pop-up editor to handle "rename" operations
    itk_component add editor {
        Rappture::Editor $itk_interior.editor \
            -activatecommand [itcl::code $this _edit activate] \
            -validatecommand [itcl::code $this _edit validate] \
            -applycommand [itcl::code $this _edit apply]
    }

    bind $itk_component(editor) <Unmap> [itcl::code $this _edit revert]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: _redraw ?<eventArgs>...?
#
# Used internally to redraw all items on the path name display.  This
# gets invoked after the widget is packed to wrap the path name
# properly whenever the width changes.
# ----------------------------------------------------------------------
itcl::body Rappture::ObjPath::_redraw {args} {
    set fn $itk_option(-pathfont)
    $itk_component(path) delete 1.0 end

    set label $itk_option(-label)
    if {"" != $label && ![string match *: $label]} {
        append label ":"
    }
    $itk_component(path) insert end $label
    $itk_component(path) insert end "  "
    $itk_component(path) insert end $itk_option(-pathtext) path

    _resize
}

# ----------------------------------------------------------------------
# USAGE: _resize ?<eventArgs>...?
#
# Used internally to update the height of the path part of this widget
# whenever it changes size.  If the path gets squeezed, it wraps onto
# multiple lines.  This routine figures out how many lines to display.
# ----------------------------------------------------------------------
itcl::body Rappture::ObjPath::_resize {args} {
    set fn $itk_option(-pathfont)

    # figure out how much width is available for the path
    set whull [winfo width $itk_component(hull)]
    set wbtn [winfo width $itk_component(button)]
    set wpath [expr {$whull-$wbtn-10}]

    # if the path were on 1 line, how much width would we need?
    # if it fits on 1 line, then pack it differently
    set str [$itk_component(path) get 1.0 end]
    set w [font measure $fn $str]
    if {$w < $wpath} {
        $itk_component(size) configure -width [expr {$w+4}]
        pack $itk_component(size) -expand no -fill x
    } else {
        $itk_component(size) configure -width 1
        pack $itk_component(size) -expand yes -fill both
    }

    # figure out how many lines we need for the height
    set lspace [font metrics $fn -linespace]
    set bbox [$itk_component(path) bbox end-1char]
    if {"" != $bbox} {
        # Find the position of the last char and figure out how tall to be.
        foreach {x0 y0 w h} $bbox break
        set ht [expr {round(ceil(($y0+$h-2)/double($lspace)))}]
        $itk_component(path) configure -height $ht
    } else {
        # Dang! bbox won't work because the last char is off screen.
        # Figure out the char in the bottom-right corner and then
        # guess how many lines are off screen below that.
        set w [expr {[winfo width $itk_component(path)]-4}]
        set h [expr {[winfo height $itk_component(path)]-4}]
        set index [$itk_component(path) index @$w,$h]
        set cnum [lindex [split $index .] 1]
        set end [lindex [split [$itk_component(path) index end-1char] .] 1]
        set frac [expr {1 + double($end-$cnum)/$end}]

        set ht [expr {$frac*[winfo height $itk_component(path)]}]
        set ht [expr {round(ceil($ht/$lspace))}]
        $itk_component(path) configure -height $ht
    }
    $itk_component(size) configure -height [expr {$ht*$lspace+4}]
}

# ----------------------------------------------------------------------
# USAGE: _hilite set|clear
#
# Invoked whenever you click on the path name to highlight the path
# and change to "copy" mode for the path.
# ----------------------------------------------------------------------
itcl::body Rappture::ObjPath::_hilite {op} {
    switch -- $op {
        set {
            $itk_component(path) tag configure path \
                -background $itk_option(-selectbackground) \
                -borderwidth 1 -relief raised
            $itk_component(button) configure -text "Copy" \
                -command [itcl::code $this _copy]
        }
        clear {
            $itk_component(path) tag configure path \
                -background "" -borderwidth 0
            $itk_component(button) configure -text "Rename" \
                -command [itcl::code $this _edit start]
        }
        toggle {
            if {"" != [$itk_component(path) tag cget path -background]} {
                _hilite clear
            } else {
                _hilite set
            }
        }
        default {
            error "bad option \"$op\": should be set, clear, toggle"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _copy
#
# Handles the operation of the "Copy" button.  Copies the current
# path text to the clipboard so it can be pasted into other widgets.
# ----------------------------------------------------------------------
itcl::body Rappture::ObjPath::_copy {} {
    clipboard clear
    clipboard append -type STRING -- $itk_option(-pathtext)
    _hilite clear
}

# ----------------------------------------------------------------------
# USAGE: _edit activate
# USAGE: _edit validate <value>
# USAGE: _edit apply <value>
# USAGE: _edit revert
#
# Used internally to handle the pop-up editor that edits part of
# the title string for a node.  The "start" operation is called when
# the user clicks on the "rename" button.  This brings up an editor
# for the id anme, allowing the user to change the name.  The
# "activate" operation is called by the editor to figure out where
# it should pop up.  The "validate" operation is called to check the
# new value and make sure that it is okay.  The "apply" operation
# applies the new name to the node.  The "revert" operation puts
# the original value back into the display when the user presses Esc.
# ----------------------------------------------------------------------
itcl::body Rappture::ObjPath::_edit {option args} {
    switch -- $option {
        start {
            if {[regexp -indices {[a-zA-Z]+\([a-zA-Z0-9_]+\)$} $itk_option(-pathtext) match]} {
                foreach {s0 s1} $match break
                $itk_component(path) delete 1.0 end
                $itk_component(path) insert 1.0 [string range $itk_option(-pathtext) $s0 end]
                $itk_component(editor) activate
            }
        }
        activate {
            if {[regexp -indices {([a-zA-Z]+)\(([a-zA-Z0-9_]+)\)$} $itk_option(-pathtext) match type id]} {
                foreach {t0 t1} $type break
                foreach {s0 s1} $id break
                set str [string range $itk_option(-pathtext) $s0 $s1]

                set s0 [expr {$s0-$t0}]  ;# set relative to start of tail string
                set s1 [expr {$s1-$t0}]

                # get bbox for first and last char in "(id)" part
                foreach {x0 y0 w h} [$itk_component(path) bbox 1.$s0] break
                foreach {x1 y0 w h} [$itk_component(path) bbox 1.$s1] break
                set x1 [expr {$x1+$w}]
                set y1 [expr {$y0+$h}]

                # compute overall x0,y0 and width/height for this area
                set w [expr {$x1-$x0}]
                set h [expr {$y1-$y0}]
                set x0 [expr {[winfo rootx $itk_component(path)]+$x0}]
                set y0 [expr {[winfo rooty $itk_component(path)]+$y0}]

                return [list text $str x $x0 y [expr {$y0-2}] w $w h $h]
            }
        }
        validate {
            set val [lindex $args 0]
            if {![regexp {^[a-zA-Z0-9_]+$} $val]} {
                bell
                return 0
            }
            return 1
        }
        apply {
            set val [lindex $args 0]
            if {[string length $itk_option(-renamecommand)] > 0} {
                uplevel #0 $itk_option(-renamecommand) [list $val]
            }
        }
        revert {
            _redraw
        }
        default {
            error "bad option \"$option\": should be start, activate, validate, apply, revert"
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -label, -pathtext
# ----------------------------------------------------------------------
itcl::configbody Rappture::ObjPath::label {
    $_dispatcher event -idle !redraw
}
itcl::configbody Rappture::ObjPath::pathtext {
    $_dispatcher event -idle !redraw
}
