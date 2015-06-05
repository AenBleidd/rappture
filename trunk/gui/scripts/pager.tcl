# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: pager - notebook for displaying pages of widgets
#
#  This widget is something like a tabbed notebook, but with a little
#  more flexibility.  Pages can be inserted and deleted, and then shown
#  in various arrangements.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *Pager.arrangement "pages" widgetDefault
option add *Pager.width 0 widgetDefault
option add *Pager.height 0 widgetDefault
option add *Pager.padding 8 widgetDefault
option add *Pager.crumbColor black widgetDefault
option add *Pager.crumbNumberColor white widgetDefault
option add *Pager.dimCrumbColor gray70 widgetDefault
option add *Pager.activeCrumbColor blue widgetDefault
option add *Pager.crumbFont \
    -*-helvetica-bold-r-normal-*-12-* widgetDefault

blt::bitmap define Pager-arrow {
#define arrow_width 9
#define arrow_height 9
static unsigned char arrow_bits[] = {
   0x10, 0x00, 0x30, 0x00, 0x70, 0x00, 0xff, 0x00, 0xff, 0x01, 0xff, 0x00,
   0x70, 0x00, 0x30, 0x00, 0x10, 0x00};
}

itcl::class Rappture::Pager {
    inherit itk::Widget

    itk_option define -width width Width 0
    itk_option define -height height Height 0
    itk_option define -padding padding Padding 0
    itk_option define -crumbcolor crumbColor Foreground ""
    itk_option define -crumbnumbercolor crumbNumberColor Foreground ""
    itk_option define -crumbfont crumbFont Font ""
    itk_option define -dimcrumbcolor dimCrumbColor DimForeground ""
    itk_option define -activecrumbcolor activeCrumbColor ActiveForeground ""
    itk_option define -arrangement arrangement Arrangement ""

    constructor {args} { # defined below }

    public method insert {pos args}
    public method delete {first {last ""}}
    public method index {name}
    public method page {args}
    public method current {args}

    public method busy { bool }

    protected method _layout {}
    protected method _fixSize {}
    protected method _drawCrumbs {how}

    private variable _counter 0      ;# counter for page names
    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _pages ""       ;# list of known pages
    private variable _page2info      ;# maps page name => -frame,-title,-command
    private variable _current ""     ;# page currently shown
}

itk::usual Pager {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout "[itcl::code $this _layout]; list"
    $_dispatcher register !fixsize
    $_dispatcher dispatch $this !fixsize "[itcl::code $this _fixSize]; list"

    itk_component add controls {
        frame $itk_interior.cntls
    }

    itk_component add next {
        button $itk_component(controls).next -text "Next >" \
            -command [itcl::code $this current next>]
    }
    pack $itk_component(next) -side right

    itk_component add back {
        button $itk_component(controls).back -text "< Back" \
            -command [itcl::code $this current <back]
    }
    pack $itk_component(back) -side left

    set font [$itk_component(next) cget -font]
    set ht [font metrics $font -linespace]
    itk_component add breadcrumbarea {
        frame $itk_interior.bcarea
    }
    itk_component add breadcrumbs {
        canvas $itk_component(breadcrumbarea).breadcrumbs \
            -width 10 -height [expr {$ht+2}]
    }
    pack $itk_component(breadcrumbs) -side left -expand yes -fill both \
        -padx 8 -pady 8

    itk_component add line {
        frame $itk_interior.line -height 2 -borderwidth 1 -relief sunken
    }


    itk_component add inside {
        frame $itk_interior.inside
    }
    pack $itk_component(inside) -expand yes -fill both
    pack propagate $itk_component(inside) no

    eval itk_initialize $args
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> ?-name <name>? ?-title <label>? ?-command <str>?
#
# Clients use this to insert a new page into this pager.  The page is
# inserted into the list at position <pos>, which can be an integer
# starting from 0 or the keyword "end".  The optional <name> can be
# used to identify the page.  If it is not supplied, a name is created
# for the page.  The -title and -command are other values associated
# with the page.
#
# Returns the name used to identify the page.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::insert {pos args} {
    if {"end" == $pos} {
        set pos [llength $_pages]
    } elseif {![string is integer $pos]} {
        error "bad index \"$pos\": should be integer or \"end\""
    }

    Rappture::getopts args params {
        value -name page#auto
        value -title "Page #auto"
        value -command ""
    }
    if {[llength $args] > 0} {
        error "wrong # args: should be \"insert pos ?-name n? ?-title t? ?-command c?\""
    }

    incr _counter
    if {$_counter > 1} {
        set subst "#$_counter"
    } else {
        set subst ""
    }
    if {[regexp {#auto} $params(-name)]} {
        regsub -all {#auto} $params(-name) $subst params(-name)
    }
    if {[regexp {#auto} $params(-title)]} {
        regsub -all {#auto} $params(-title) $subst params(-title)
    }

    # allocate the page
    if {[info exists _page2info($params(-name)-frame)]} {
        error "page \"$params(-name)\" already exists"
    }
    set win $itk_component(inside).page$_counter
    frame $win
    set _page2info($params(-name)-frame) $win
    set _page2info($params(-name)-title) $params(-title)
    set _page2info($params(-name)-command) $params(-command)
    set _pages [linsert $_pages $pos $params(-name)]

    bind $win <Configure> \
        [itcl::code $_dispatcher event -idle !fixsize]

    # the number of pages affects the arrangment -- force an update
    configure -arrangement $itk_option(-arrangement)

    $_dispatcher event -idle !layout

    return $params(-name)
}

# ----------------------------------------------------------------------
# USAGE: delete <first> ?<last>?
#
# Clients use this to delete one or more pages from this widget.
# The <first> and <last> represent the integer index of the desired
# page.  You can use the "index" method to convert a page name to
# its integer index.  If only <first> is specified, then that one
# page is deleted.  If <last> is specified, then all pages in the
# range <first> to <last> are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::delete {first {last ""}} {
    if {$last == ""} {
        set last $first
    }
    if {![regexp {^[0-9]+|end$} $first]} {
        error "bad index \"$first\": should be integer or \"end\""
    }
    if {![regexp {^[0-9]+|end$} $last]} {
        error "bad index \"$last\": should be integer or \"end\""
    }

    foreach name [lrange $_pages $first $last] {
        if {[info exists _page2info($name-frame)]} {
            destroy $_page2info($name-frame)
            unset _page2info($name-frame)
            unset _page2info($name-title)
            unset _page2info($name-command)
        }
    }
    set _pages [lreplace $_pages $first $last]

    # the number of pages affects the arrangment -- force an update
    configure -arrangement $itk_option(-arrangement)

    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# USAGE: index <name>|@n
#
# Clients use this to convert a page <name> into its corresponding
# integer index.  Returns an error if the <name> is not recognized.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::index {name} {
    set i [lsearch $_pages $name]
    if {$i >= 0} {
        return $i
    }
    if {[regexp {^@([0-9]+)$} $name match i]} {
        return $i
    }
    error "bad page name \"$name\": should be @int or one of [join [lsort $_pages] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: page
# USAGE: page <name>|@n ?-frame|-title|-command? ?<newvalue>?
#
# Clients use this to get information about pages.  With no args, it
# returns a list of all page names.  Otherwise, it returns the
# requested information for a page specified by its <name> or index
# @n.  By default, it returns the -frame for the page, but it can
# also return the -title and -command.  The -title and -command
# can also be set to a <newvalue>.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::page {args} {
    if {[llength $args] == 0} {
        return $_pages
    }
    set i [index [lindex $args 0]]
    set name [lindex $_pages $i]

    set args [lrange $args 1 end]
    Rappture::getopts args params {
        flag what -frame default
        flag what -title
        flag what -command
    }

    if {[llength $args] == 0} {
        set opt $params(what)
        return $_page2info($name$opt)
    } elseif {[llength $args] == 1} {
        set val [lindex $args 0]
        if {$params(-title)} {
            set _page2info($name-title) $val
        } elseif {$params(-command)} {
            set _page2info($name-command) $val
        }
    } else {
        error "wrong # args: should be \"page ?which? ?-frame|-title|-command? ?newvalue?\""
    }
}

# ----------------------------------------------------------------------
# USAGE: current ?<name>|next>|<back?
#
# Used to query/set the current page in the notebook.  With no args,
# it returns the name of the current page.  Otherwise, it sets the
# current page.  The special token "next>" is used to set the pager
# to the next logical page, and "<back" sets to the previous.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::current {args} {
    switch -- [llength $args] {
        0 {
            return $_current
        }
        1 {
            if {$itk_option(-arrangement) != "pages"} {
                return ""
            }
            set name [lindex $args 0]
            set index 0
            if {$name == "next>"} {
                if {$_current == ""} {
                    set index 0
                } else {
                    set i [lsearch -exact $_pages $_current]
                    set index [expr {$i+1}]
                    if {$index >= [llength $_pages]} {
                        set index [expr {[llength $_pages]-1}]
                    }
                }
                set _current [lindex $_pages $index]
            } elseif {$name == "<back"} {
                if {$_current == ""} {
                    set index end
                } else {
                    set i [lsearch -exact $_pages $_current]
                    set index [expr {$i-1}]
                    if {$index < 0} {
                        set index 0
                    }
                }
                set _current [lindex $_pages $index]
            } else {
                if {$name == ""} {
                    set _current ""
                    set index 0
                } else {
                    set index [lsearch -exact $_pages $name]
                    if {$index < 0} {
                        error "can't move to page \"$name\""
                    }
                    set _current [lindex $_pages $index]
                }
            }

            foreach w [pack slaves $itk_component(inside)] {
                pack forget $w
            }
            if {$_current != ""} {
                pack $_page2info($_current-frame) -expand yes -fill both \
                    -padx $itk_option(-padding) -pady $itk_option(-padding)
            }

            if {$index == 0} {
                pack forget $itk_component(back)
            } else {
                set prev [expr {$index-1}]
                if {$prev >= 0} {
                    set label "< [page @$prev -title]"
                } else {
                    set label "< Back"
                }
                $itk_component(back) configure -text $label
                pack $itk_component(back) -side left
            }
            if {$index == [expr {[llength $_pages]-1}]} {
                pack forget $itk_component(next)
            } else {
                set next [expr {$index+1}]
                if {$next <= [llength $_pages]} {
                    set label "[page @$next -title] >"
                } else {
                    set label "Next >"
                }
                $itk_component(next) configure -text $label
                pack $itk_component(next) -side right
            }
            _drawCrumbs current

            #
            # If this new page has a command associated with it, then
            # invoke it now.
            #
            if {"" != $_current
                  && [string length $_page2info($_current-command)] > 0} {
                uplevel #0 $_page2info($_current-command)
            }
        }
        default {
            error "wrong # args: should be \"current name|next>|<back\""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _layout
#
# Used internally to fix the current page management whenever pages
# are added or deleted, or when the page arrangement changes.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::_layout {} {
    if {$itk_option(-arrangement) == "pages"} {
        if {$_current == ""} {
            set _current [lindex $_pages 0]
            if {$_current != ""} {
                current $_current
            }
        }
        _drawCrumbs all
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixSize
#
# Invoked automatically whenever a page changes size or the -width
# or -height options change.  When the -width/-height are zero, this
# method computes the minimum size needed to accommodate all pages.
# Otherwise, it passes the size request onto the hull.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::_fixSize {} {
    set sw [expr {[winfo screenwidth $itk_component(hull)]-200}]
    set sh [expr {[winfo screenheight $itk_component(hull)]-200}]

    update  ;# force layout changes so sizes are correct
    switch -- $itk_option(-arrangement) {
        pages {
            if {$itk_option(-width) <= 0} {
                set maxw [expr {
                    [winfo reqwidth $itk_component(next)]
                    + 10
                    + [winfo reqwidth $itk_component(back)]}]

                foreach name $_pages {
                    set w [winfo reqwidth $_page2info($name-frame)]
                    if {$w > $maxw} { set maxw $w }
                }
                set maxw [expr {$maxw + 2*$itk_option(-padding)}]
                if {$maxw > $sw} { set maxw $sw }
                $itk_component(inside) configure -width $maxw
            } else {
                $itk_component(inside) configure -width $itk_option(-width)
            }

            if {$itk_option(-height) <= 0} {
                set maxh 0
                foreach name $_pages {
                    set h [winfo reqheight $_page2info($name-frame)]
                    if {$h > $maxh} { set maxh $h }
                }
                set maxh [expr {$maxh + 2*$itk_option(-padding)}]
                if {$maxh > $sh} { set maxh $sh }
                $itk_component(inside) configure -height $maxh
            } else {
                $itk_component(inside) configure -height $itk_option(-height)
            }
        }
        side-by-side {
            if {$itk_option(-width) <= 0} {
                set maxw [expr {
                    [winfo reqwidth $itk_component(next)]
                    + 10
                    + [winfo reqwidth $itk_component(back)]}]

                set wtotal 0
                foreach name $_pages {
                    set w [winfo reqwidth $_page2info($name-frame)]
                    set wtotal [expr {$wtotal + $w + 2*$itk_option(-padding)}]
                }
                if {$wtotal > $maxw} { set maxw $wtotal }
                if {$maxw > $sw} { set maxw $sw }
                $itk_component(inside) configure -width $maxw
            } else {
                $itk_component(inside) configure -width $itk_option(-width)
            }

            if {$itk_option(-height) <= 0} {
                set maxh 0
                foreach name $_pages {
                    set h [winfo reqheight $_page2info($name-frame)]
                    if {$h > $maxh} { set maxh $h }
                }
                set maxh [expr {$maxh + 2*$itk_option(-padding)}]
                if {$maxh > $sh} { set maxh $sh }
                $itk_component(inside) configure -height $maxh
            } else {
                $itk_component(inside) configure -height $itk_option(-height)
            }
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -arrangement
# ----------------------------------------------------------------------
itcl::configbody Rappture::Pager::arrangement {
    switch -- $itk_option(-arrangement) {
        pages {
            pack forget $itk_component(inside)
            pack $itk_component(controls) -side bottom -fill x -padx 60 -pady 8
            pack $itk_component(breadcrumbarea) -side top -fill x
            pack $itk_component(line) -side top -fill x
            pack $itk_component(inside) -expand yes -fill both
            current [lindex $_pages 0]
        }
        side-by-side {
            pack forget $itk_component(controls)
            pack forget $itk_component(line)
            pack forget $itk_component(breadcrumbarea)

            foreach w [pack slaves $itk_component(inside)] {
                pack forget $w
            }
            foreach name $_pages {
                pack $_page2info($name-frame) -side left \
                    -expand yes -fill both \
                    -padx $itk_option(-padding) -pady $itk_option(-padding)
            }
        }
        default {
            error "bad value \"$itk_option(-arrangement)\": should be pages or side-by-side"
        }
    }
    $_dispatcher event -now !fixsize
}

# ----------------------------------------------------------------------
# OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::Pager::width {
    $_dispatcher event -idle !fixsize
}

# ----------------------------------------------------------------------
# OPTION: -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::Pager::height {
    $_dispatcher event -idle !fixsize
}

# ----------------------------------------------------------------------
# OPTION: -padding
# ----------------------------------------------------------------------
itcl::configbody Rappture::Pager::padding {
    if {$_current != ""} {
        pack $_page2info($_current-frame) -expand yes -fill both \
            -padx $itk_option(-padding) -pady $itk_option(-padding)
    }
    $_dispatcher event -idle !fixsize
}

# ----------------------------------------------------------------------
# USAGE: _drawCrumbs all|current
#
# Invoked automatically whenever the pages change.  The value "all"
# signifies that the number of pages has changed, so all should be
# redrawn.  The value "current" means that the current page has
# changed, so there is just a simple color change.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::_drawCrumbs {how} {
    set c $itk_component(breadcrumbs)
    set fnt $itk_option(-crumbfont)

    switch -- $how {
        all {
            $c delete all

            set x 0
            set y [expr {[winfo reqheight $c]/2}]
            set last [lindex $_pages end]

            set num 1
            foreach name $_pages {
                set ht [expr {[font metrics $fnt -linespace]+2}]
                set id [$c create oval $x [expr {$y-$ht/2}] \
                    [expr {$x+$ht}] [expr {$y+$ht/2}] \
                    -outline "" -fill $itk_option(-dimcrumbcolor) \
                    -tags $name]
                set id [$c create text [expr {$x+$ht/2}] [expr {$y+1}] \
                    -text $num -fill $itk_option(-crumbnumbercolor) \
                    -tags [list $name $name-num]]
                set x [expr {$x + $ht+2}]

                set id [$c create text $x [expr {$y+1}] -anchor w \
                    -text [page $name -title] -font $fnt -tags $name]

                $c bind $name <Enter> [itcl::code $this _drawCrumbs active]
                $c bind $name <Leave> [itcl::code $this _drawCrumbs current]
                $c bind $name <ButtonPress> [itcl::code $this current $name]

                foreach {x0 y0 x1 y1} [$c bbox $id] break
                set x [expr {$x + ($x1-$x0)+6}]

                if {$name != $last} {
                    set id [$c create bitmap $x $y -anchor w \
                        -bitmap Pager-arrow \
                        -foreground $itk_option(-dimcrumbcolor)]
                    foreach {x0 y0 x1 y1} [$c bbox $id] break
                    set x [expr {$x + ($x1-$x0)+6}]
                }

                incr num
            }

            # fix the scrollregion in case we go off screen
            $c configure -scrollregion [$c bbox all]

            _drawCrumbs current
        }
        current {
            # make all crumbs dim
            foreach name $_pages {
                $c itemconfigure $name \
                    -fill $itk_option(-dimcrumbcolor)
                $c itemconfigure $name-num \
                    -fill $itk_option(-crumbnumbercolor)
            }

            # make all the current crumb bright
            if {$_current != ""} {
                $c itemconfigure $_current \
                    -fill $itk_option(-crumbcolor)
                $c itemconfigure $_current-num \
                    -fill $itk_option(-crumbnumbercolor)

                # scroll the view to see the crumb
                if {[$c bbox $_current] != ""} {
                    foreach {x0 y0 x1 y1} [$c bbox $_current] break
                    foreach {xm0 ym0 xm1 ym1} [$c bbox all] break
                    set xm [expr {double($x0)/($xm1-$xm0)}]
                    $c xview moveto $xm
                }
            } else {
                $c xview moveto 0
            }
        }
        active {
            foreach tag [$c gettags current] {
                if {[lsearch -exact $_pages $tag] >= 0} {
                    $c itemconfigure $tag -fill $itk_option(-activecrumbcolor)
                    $c itemconfigure $tag-num -fill white
                }
            }
        }
    }
}

#
# busy --
#
#       If true (this indicates a simulation is occurring), the widget
#       should prevent the user from
#               1) clicking an item previous in the breadcrumbs, and
#               2) using the "back" button.
#
itcl::body Rappture::Pager::busy { bool } {
    if { $bool } {
        blt::busy hold $itk_component(breadcrumbs)
        $itk_component(back) configure -state disabled
    } else {
        blt::busy release $itk_component(breadcrumbs)
        $itk_component(back) configure -state normal
    }
}
