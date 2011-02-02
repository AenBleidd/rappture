# ----------------------------------------------------------------------
#  COMPONENT: testview - display the results of a test
#
#  Entire right hand side of the regression tester.  Displays the
#  golden test results, and compares them to the new results if the test
#  has been run.  Also show tree representation of all inputs and
#  outputs.  The -test configuration option is used to provide a Test
#  object to display.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2010-2011  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require RapptureGUI

namespace eval Rappture::Tester { # forward declaration }

option add *StatusList.font {Arial -12} widgetDefault
option add *StatusList.titleFont {Arial -12 bold} widgetDefault
option add *StatusList.subTitleFont {Arial -10} widgetDefault

itcl::class Rappture::Tester::StatusEntry {
    public variable title ""
    public variable subtitle ""
    public variable body ""
    public variable icon ""
    public variable clientdata ""

    constructor {args} { eval configure $args }
}

itcl::class Rappture::Tester::StatusList {
    inherit itk::Widget 

    itk_option define -font font Font ""
    itk_option define -titlefont titleFont Font ""
    itk_option define -subtitlefont subTitleFont Font ""
    itk_option define -selectcommand selectCommand SelectCommand ""
    itk_option define -selectbackground selectBackground Foreground ""

    constructor {args} { # defined later }
    destructor { # defined later }

    public method insert {pos args}
    public method delete {from {to ""}}
    public method size {} { return [llength $_entries] }
    public method get {pos args}

    public method xview {args} {
        return [eval $itk_component(listview) xview $args]
    }
    public method yview {args} {
        return [eval $itk_component(listview) yview $args]
    }

    protected method _redraw {}
    protected method _select {tag}

    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _entries ""     ;# list of status entries
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusList::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw "[itcl::code $this _redraw]; list"

    itk_component add listview {
        canvas $itk_interior.lv -relief flat
    } {
        usual
        keep -xscrollcommand -yscrollcommand
    }
    pack $itk_component(listview) -expand yes -fill both

    eval itk_initialize $args
}

itk::usual StatusList {
    keep -background -foreground -cursor
    keep -selectbackground
    keep -font -titlefont -subtitlefont
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusList::destructor {} {
    delete 0 end
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> ?-option value -option value ...?
#
# Inserts a new entry into the list at the given <pos>.  The options
# are those recognized by a StatusEntry object.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusList::insert {pos args} {
    set entry [eval Rappture::Tester::StatusEntry #auto $args]
    set _entries [linsert $_entries $pos $entry]
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# USAGE: delete <pos> ?<toPos>?
#
# Deletes a single entry or a range of entries from the list displayed
# in this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusList::delete {pos {to ""}} {
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
# USAGE: _redraw
#
# Used internally to update the detailed list of items maintained
# by this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusList::_redraw {} {
    set c $itk_component(listview)
    $c delete all

    # figure out the maximum size of all icons
    set iw 0
    set ih 0
    foreach obj $_entries {
        set icon [$obj cget -icon]
        if {$icon ne ""} {
            if {[image width $icon] > $iw} { set iw [image width $icon] }
            if {[image height $icon] > $ih} { set ih [image height $icon] }
        }
    }

    set tlineh [font metrics $itk_option(-titlefont) -linespace]
    set stlineh [font metrics $itk_option(-subtitlefont) -linespace]

    set x0 2
    set y0 2
    set n 0
    foreach obj $_entries {
        set tag "entry$n"

        set icon [$obj cget -icon]
        set iconh 0
        if {$icon ne ""} {
            $c create image [expr {$x0+$iw}] $y0 -anchor ne -image $icon \
                -tags [list $tag main]
            set iconh [image height $icon]
        }
        set x1 [expr {$x0+$iw+3}]
        set y1 $y0

        set title [$obj cget -title]
        if {$title ne ""} {
            $c create text $x1 $y1 -anchor nw -text $title \
                -font $itk_option(-titlefont) -tags [list $tag main]
            set y1 [expr {$y1+$tlineh+2}]
        }

        set subtitle [$obj cget -subtitle]
        if {$subtitle ne ""} {
            $c create text $x1 $y1 -anchor nw -text $subtitle \
                -font $itk_option(-subtitlefont) -tags [list $tag main]
            set y1 [expr {$y1+$stlineh+2}]
        }

        set body [$obj cget -body]
        if {$body ne ""} {
            # a little space between the title/subtitle and the body
            if {$y1 != $y0} { incr y1 4 }

            set id [$c create text $x1 $y1 -anchor nw -text $body \
                -font $itk_option(-font) -tags [list $tag main]]

            foreach {tx0 ty0 tx1 ty1} [$c bbox $id] break
            set y1 [expr {$y1 + ($ty1-$ty0)}]
        }

        # make sure that y1 is at the bottom of the icon too
        if {$y1 < $y0+$iconh+2} {
            set y1 [expr {$y0+$iconh+2}]
        }

        # make a background selection rectangle
        set id [$c create rectangle 0 [expr {$y0-2}] 1000 $y1 \
            -outline "" -fill "" -tags [list allbg $tag:bg]]
        $c lower $id

        foreach item [list $tag $tag:bg] {
            $c bind $item <ButtonPress> \
                [itcl::code $this _select $tag]
        }

        set y0 [expr {$y1+10}]
        incr n
    }

    # set the scrolling region to the "main" part (no bg boxes)
    foreach {x0 y0 x1 y1} [$c bbox main] break
    $c configure -scrollregion [list 0 0 [expr {$x1+4}] [expr {$y1+4}]]
}

# ----------------------------------------------------------------------
# USAGE: _select <tag>
#
# Called internally when the user clicks on an item in the status
# list that shows specific test failures.  Highlights the item and
# invokes any -statuscommand configured for the widget.  Additional
# details about the item are appended onto the command as a list of
# options and values.  These include the integer -index for the
# position of the selected item, along with details defined when
# the item was inserted into the list.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::StatusList::_select {tag} {
    set c $itk_component(listview)
    $c itemconfigure allbg -fill ""
    $c itemconfigure $tag:bg -fill $itk_option(-selectbackground)

    if {[string length $itk_option(-selectcommand)] > 0} {
        set id ""; regexp {[0-9]+$} $tag id
        set vlist ""
        set obj [lindex $_entries $id]
        if {$obj ne ""} {
            foreach opt [$obj configure] {
                lappend vlist [lindex $opt 0] [lindex $opt end]
            }
        }
        uplevel #0 $itk_option(-selectcommand) -index $id $vlist
    }
}
