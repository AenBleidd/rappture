# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: editor - pop-up editor for little bits of text
#
#  This widget acts as a pop-up editor for small text fields.  It
#  pops up on top of any text field, accepts edits, and then attempts
#  to validate and apply changes back to the underlying widget.
#
#  This widget uses a number of callbacks to handle communication
#  with the underlying widget:
#
#  -activatecommand .... Should return a key/value list with the
#                        following elements:
#                          x ...... root x coordinate for editor
#                          y ...... root y coordinate for editor
#                          w ...... width of text being edited
#                          h ...... height of text being edited
#                          text ... initial text for the editor
#
#  -validatecommand .... Invoked with the new value as an argument.
#                        Should return 1 if the value is okay, and
#                        0 otherwise.
#
#  -applycommand ....... Invoked with the new value as an argument.
#                        Should apply the new value to the underlying
#                        widget.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Editor.background white widgetDefault
option add *Editor.outline black widgetDefault
option add *Editor.borderwidth 1 widgetDefault
option add *Editor.relief flat widgetDefault
option add *Editor.selectBorderWidth 0 widgetDefault

itcl::class Rappture::Editor {
    inherit itk::Toplevel

    itk_option define -outline outline Outline ""
    itk_option define -activatecommand activateCommand ActivateCommand ""
    itk_option define -validatecommand validateCommand ValidateCommand ""
    itk_option define -applycommand applyCommand ApplyCommand ""

    constructor {args} { # defined below }

    public method activate {}
    public method deactivate {args}
    public method value {newval}

    protected method _click {x y}
    protected method _resize {}
    protected variable _loc   ;# array of editor location parameters
}

itk::usual Editor {
    keep -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Editor::constructor {args} {
    wm overrideredirect $itk_component(hull) yes
    wm withdraw $itk_component(hull)

    itk_option remove hull.background hull.borderwidth
    component hull configure -borderwidth 1

    itk_component add editor {
        entry $itk_interior.editor -highlightthickness 0
    } {
        usual
        keep -relief
        ignore -highlightthickness
        ignore -highlightcolor
        ignore -highlightbackground
    }
    pack $itk_component(editor) -expand yes -fill both

    bind $itk_component(editor) <KeyPress> \
        [itcl::code $this _resize]
    bind $itk_component(editor) <KeyPress-Return> \
        [itcl::code $this deactivate]
    bind $itk_component(editor) <KeyPress-Escape> \
        [itcl::code $this deactivate -abort]
    bind $itk_component(editor) <ButtonPress> \
        [itcl::code $this _click %X %Y]

    itk_component add emenu {
        menu $itk_component(editor).menu -tearoff 0
    } {
        usual
        ignore -tearoff
        ignore -background -foreground
    }
    $itk_component(emenu) add command -label "Cut" -accelerator "^X" \
        -command [list event generate $itk_component(editor) <<Cut>>]
    $itk_component(emenu) add command -label "Copy" -accelerator "^C" \
        -command [list event generate $itk_component(editor) <<Copy>>]
    $itk_component(emenu) add command -label "Paste" -accelerator "^V" \
        -command [list event generate $itk_component(editor) <<Paste>>]
    bind $itk_component(editor) <<PopupMenu>> {
        tk_popup %W.menu %X %Y
    }

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: activate
#
# Clients use this to start the editing process on the underlying
# widget.  This pops up the editor with the current text from the
# underlying widget and allows the user to edit the text.  The editor
# remains up until it is deactivated.
# ----------------------------------------------------------------------
itcl::body Rappture::Editor::activate {} {
    set e $itk_component(editor)
    if {[winfo ismapped $e]} {
        return  ;# already mapped -- nothing to do
    }

    set info ""
    if {[string length $itk_option(-activatecommand)] > 0} {
        set status [catch {uplevel #0 $itk_option(-activatecommand)} info]
        if {$status != 0} {
            bgerror $info
            return
        }
    }

    #
    # Pull out the location information from the values passed back
    # from the activation command.  We must have at least an x,y
    # coordinate.  If we get width and height too, then use it.
    # If not, figure out the width and height based on the size
    # of the string.
    #
    array set vals $info
    if {![info exists vals(x)] || ![info exists vals(y)]} {
        return
    }
    set _loc(x) $vals(x)
    set _loc(y) $vals(y)
    set _loc(w) [expr {([info exists vals(w)]) ? $vals(w) : 0}]
    set _loc(h) [expr {([info exists vals(h)]) ? $vals(h) : 0}]

    $itk_component(editor) delete 0 end
    if {[info exists vals(text)]} {
        $itk_component(editor) insert end $vals(text)
    }
    $itk_component(editor) select from 0
    $itk_component(editor) select to end

    _resize
    wm deiconify $itk_component(hull)
    raise $itk_component(hull)
    focus -force $itk_component(editor)

    # try to grab the pointer, and keep trying...
    update
    while {[catch {grab set -global $itk_component(editor)}]} {
        after 100
    }
}

# ----------------------------------------------------------------------
# USAGE: deactivate ?-abort?
#
# This is invoked automatically whenever the user presses Enter or
# Escape in the editor.  Clients can also use it explicitly to
# deactivate the editor.
#
# If the -abort flag is specified, then the editor is taken down
# without any validation or application of the result.  Otherwise,
# we validate the contents of the editor and apply the change back
# to the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Editor::deactivate {args} {
    # take down any error cue that might be up
    ::Rappture::Tooltip::cue hide

    if {$args == "-abort"} {
        grab release $itk_component(editor)
        wm withdraw $itk_component(hull)
        return
    }

    set str [$itk_component(editor) get]

    #
    # If there's a -validatecommand option, then invoke the code
    # now to check the new value.
    #
    if {[string length $itk_option(-validatecommand)] > 0} {
        set cmd "uplevel #0 [list $itk_option(-validatecommand) [list $str]]"
        if {[catch $cmd result]} {
            bgerror $result
            set result 1
        }
        if {$result == 0} {
            bell
            $itk_component(editor) select from 0
            $itk_component(editor) select to end
            $itk_component(editor) icursor end
            focus $itk_component(editor)
            return
        }
    }

    grab release $itk_component(editor)
    wm withdraw $itk_component(hull)

    #
    # If there's an -applycommand option, then invoke the code
    # now to apply the new value.
    #
    if {[string length $itk_option(-applycommand)] > 0} {
        set cmd "uplevel #0 [list $itk_option(-applycommand) [list $str]]"
        if {[catch $cmd result]} {
            bgerror $result
            return
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: value <newval>
#
# Clients use this to suggest a new value, particular when they've
# caught an error in the editing process.  For example, if the user's
# value is below the minimum allowed value, a client would call this
# method to suggest the minimum value.
# ----------------------------------------------------------------------
itcl::body Rappture::Editor::value {newval} {
    $itk_component(editor) delete 0 end
    $itk_component(editor) insert end $newval
}

# ----------------------------------------------------------------------
# USAGE: _click <X> <Y>
#
# This is invoked automatically whenever the user clicks somewhere
# inside or outside of the editor.  If the <X>,<Y> coordinate is
# outside the editor, then we assume the user is done and wants to
# take the editor down.  Otherwise, we do nothing, and let the entry
# bindings take over.
# ----------------------------------------------------------------------
itcl::body Rappture::Editor::_click {x y} {
    if {[winfo containing $x $y] != $itk_component(editor)} {
        deactivate
    } else {
        # make sure the editor has keyboard focus!
        # it loses focus sometimes during cut/copy/paste operations
        focus -force $itk_component(editor)
    }
}

# ----------------------------------------------------------------------
# USAGE: _resize
#
# Invoked automatically as each key is pressed in the editor.
# Resizes the editor so that it is just big enough to show all
# of the text within it.
# ----------------------------------------------------------------------
itcl::body Rappture::Editor::_resize {} {
    set e $itk_component(editor)
    set str [$e get]
    set fnt [$e cget -font]

    set w [expr {[font measure $fnt $str]+20}]
    set w [expr {($w < $_loc(w)) ? $_loc(w) : $w}]
    if {$w+$_loc(x) >= [winfo screenwidth $e]} {
        set w [expr {[winfo screenwidth $e]-$_loc(x)}]
    }

    set h [expr {[font metrics $fnt -linespace]+4}]
    set h [expr {($h < $_loc(h)) ? $_loc(h) : $h}]
    if {$h+$_loc(y) >= [winfo screenheight $e]} {
        set h [expr {[winfo screenheight $e]-$_loc(y)}]
    }
    # Temporary fix to prevent Opps. Don't deal with negative dimensions.
    if { $w <= 0 || $h <= 0 } {
        wm geometry $itk_component(hull) "+$_loc(x)+$_loc(y)"
    } else {
        wm geometry $itk_component(hull) "${w}x${h}+$_loc(x)+$_loc(y)"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -outline
# ----------------------------------------------------------------------
itcl::configbody Rappture::Editor::outline {
    component hull configure -background $itk_option(-outline)
}
