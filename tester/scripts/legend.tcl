# ----------------------------------------------------------------------
#  COMPONENT: legend - show a legend of color/line samples
#
#  This widget acts as a legend for the differences view.  It manages
#  a series of samples, each with a block or line sample and a label.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

namespace eval Rappture::Tester { # forward declaration }

option add *Legend.font {Arial -12} widgetDefault
option add *Legend.padX 12 widgetDefault

itcl::class Rappture::Tester::LegendEntry {
    public variable title ""
    public variable shape "box" {
        if {[lsearch {box line} $shape] < 0} {
            error "bad value \"$shape\": should be box, line"
        }
    }
    public variable color ""
    public variable state "normal" {
        if {[lsearch {normal disabled} $state] < 0} {
            error "bad value \"$state\": should be normal, disabled"
        }
    }
    public variable anchor "w" {
        if {[lsearch {e w} $anchor] < 0} {
            error "bad value \"$anchor\": should be e, w"
        }
    }

    constructor {args} { eval configure $args }
}

itcl::class Rappture::Tester::Legend {
    inherit itk::Widget 

    itk_option define -font font Font ""
    itk_option define -padx padX PadX 1

    constructor {args} { # defined later }
    destructor { # defined later }

    public method insert {pos args}
    public method delete {from {to ""}}
    public method itemconfigure {what args}
    public method size {} { return [llength $_entries] }
    public method get {pos args}

    protected method _redraw {}

    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _entries ""     ;# list of status entries
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Legend::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw "[itcl::code $this _redraw]; list"

    itk_component add area {
        canvas $itk_interior.area -relief flat
    }
    pack $itk_component(area) -expand yes -fill both

    bind $itk_component(hull) <Configure> \
        [list $_dispatcher event -idle !redraw]

    eval itk_initialize $args
}

itk::usual Legend {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Legend::destructor {} {
    delete 0 end
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> ?-option value -option value ...?
#
# Inserts a new entry into the legend at the given <pos>.  The options
# are those recognized by a LegendEntry object.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Legend::insert {pos args} {
    set entry [eval Rappture::Tester::LegendEntry #auto $args]
    set _entries [linsert $_entries $pos $entry]
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# USAGE: delete <pos> ?<toPos>?
#
# Deletes a single entry or a range of entries from the legend
# displayed in this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Legend::delete {pos {to ""}} {
    if {$to eq ""} {
        set to $pos
    }
    foreach obj [lrange $_entries $pos $to] {
        itcl::delete object $obj
    }
    set _entries [lreplace $_entries $pos $to]
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# USAGE: itemconfigure <what> ?-option? ?value -option value ...?
#
# Changes the options of a particular entry.  The <what> can be the
# -title of the entry, or an integer index.  The options are those
# recognized by a LegendEntry object.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Legend::itemconfigure {what args} {
    # first, see if the <what> parameter matches a title
    set obj ""
    foreach entry $_entries {
        if {[$entry cget -title] eq $what} {
            set obj $entry
            break
        }
    }

    # if not, see if it's an integer index
    if {$obj eq "" && [string is integer -strict $what]} {
        set obj [lindex $_entries $what]
    }

    if {$obj eq ""} {
        error "bad option \"$what\": should be entry title or integer index"
    }

    # if this is a query operation, return the info
    if {[llength $args] < 2} {
        return [eval $obj configure $args]
    }

    # configure the entry and then schedule a redraw to show the change
    eval $obj configure $args
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# USAGE: get <pos> ?-key?
#
# Queries information about a particular entry at index <pos>.  With
# no extra args, it returns a list of "-key value -key value ..."
# representing all of the data about that entry.  Otherwise, the value
# for a particular -key can be requested.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Legend::get {pos {option ""}} {
    set obj [lindex $_entries $pos]
    if {$obj eq ""} {
        return ""
    }
    if {$option eq ""} {
        set vlist ""
        foreach opt [$obj configure] {
            lappend vlist [lindex $opt 0] [lindex $opt end]
        }
        return $vlist
    }
    return [$obj cget $option]
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to update the detailed list of items maintained
# by this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::Legend::_redraw {} {
    set c $itk_component(area)
    set w [winfo width $c]
    set ymid [expr {[winfo height $c]/2}]
    set padx $itk_option(-padx)
    set fn $itk_option(-font)
    set ssize [expr {[font metrics $fn -linespace]-2}]

    $c delete all

    # left/right edges of the drawing area
    set x0 2
    set x1 [expr {[winfo width $c]-2}]

    # overall label on the left
    set id [$c create text $x0 $ymid -anchor w -text "Legend:" -font $fn]
    foreach {bx0 by0 bx1 by1} [$c bbox $id] break
    set x0 [expr {$x0 + $bx1-$bx0 + $padx}]

    foreach obj $_entries {
        if {[$obj cget -state] eq "disabled" || [$obj cget -title] eq ""
              || [$obj cget -color] eq ""} {
            continue
        }

        set labelw [font measure $fn [$obj cget -title]]
        set entryw [expr {$labelw + $ssize + 3}]

        switch -- [$obj cget -anchor] {
            w {
                set xpos $x0
                set x0 [expr {$x0 + $entryw + $padx}]
            }
            e {
                set xpos [expr {$x1 - $entryw}]
                set x1 [expr {$x1 - $entryw - $padx}]
            }
        }

        switch -- [$obj cget -shape] {
            box {
                # draw the box style
                $c create rectangle \
                    $xpos [expr {$ymid-$ssize/2}] \
                    [expr {$xpos+$ssize}] [expr {$ymid+$ssize/2}] \
                    -outline black -fill [$obj cget -color]
            }
            line {
                # draw the line style
                $c create line $xpos $ymid [expr {$xpos+$ssize}] $ymid \
                    -width 2 -fill [$obj cget -color]
            }
        }
        set xpos [expr {$xpos+$ssize+3}]

        $c create text $xpos $ymid -anchor w -text [$obj cget -title] -font $fn

        if {$x0 >= $x1} break
    }

    # fix the requested size for the widget based on the layout
    $c configure -width [expr {$x0 + [winfo width $c]-$x1}]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTIONS
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::Legend::font {
    set lineh [font metrics $itk_option(-font) -linespace]
    $itk_component(area) configure -height [expr {$lineh+4}]
    $_dispatcher event -idle !redraw
}
