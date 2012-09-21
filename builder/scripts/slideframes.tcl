# ----------------------------------------------------------------------
#  COMPONENT: slideframes - collection of frames with headings
#
#  This widget is similar to a tabbed notebook, but instead of having
#  tabs on the side, it has headings inline with the content.  Clicking
#  on a heading brings up the frame beneath the heading.  Only one
#  frame can be packed at a time.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Slideframes.width 0 widgetDefault
option add *Slideframes.height 0 widgetDefault
option add *Slideframes.headingBackground gray widgetDefault
option add *Slideframes.headingForeground black widgetDefault
option add *Slideframes.headingBorderWidth 2 widgetDefault
option add *Slideframes.headingRelief raised widgetDefault

itcl::class Rappture::Slideframes {
    inherit itk::Widget

    itk_option define -width width Width 0
    itk_option define -height height Height 0
    itk_option define -animstartcommand animStartCommand AnimStartCommand ""
    itk_option define -animendcommand animEndCommand AnimEndCommand ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method insert {pos args}
    public method delete {args}
    public method index {name}
    public method page {name}
    public method current {args}
    public method size {} { return [llength $_pages] }

    private method _fixSize {}
    private method _fixLayout {args}

    private variable _count 0       ;# counter for unique names
    private variable _pages ""      ;# list of page frames
    private variable _name2num      ;# maps name => id num for heading/page
    private variable _current ""    ;# page currently shown
    private variable _tweener ""    ;# tweener object for animations
}

itk::usual Slideframes {
    keep -cursor -font
    keep -headingbackground -headingforeground
    keep -headingborderwidth -headingrelief
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Slideframes::constructor {args} {
    itk_option add hull.width hull.height
    pack propagate $itk_component(hull) no

    bind $itk_component(hull) <Configure> [itcl::code $this _fixLayout]

    set _tweener [Rappture::Tweener #auto]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Slideframes::destructor {} {
    itcl::delete object $_tweener
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> ?<name> <name>...?
#
# Used to insert one or more pages in the list of sliding frames.
# Each page is identified by its <name>.  Returns the widget name
# for each page created.
# ----------------------------------------------------------------------
itcl::body Rappture::Slideframes::insert {pos args} {
    set rlist ""
    foreach name $args {
        if {[lsearch $_pages $name] >= 0} {
            error "page \"$name\" already exists"
        }
        set id [incr _count]

        set hname "heading$id"
        itk_component add $hname {
            button $itk_interior.$hname -text $name -highlightthickness 0 \
                -anchor w -command [itcl::code $this current $name]
        } {
            usual
            rename -background -headingbackground headingBackground Background
            rename -foreground -headingforeground headingForeground Foreground
            rename -borderwidth -headingborderwidth headingBorderWidth BorderWidth
            rename -relief -headingrelief headingRelief Relief
            ignore -highlightthickness
        }

        set pname "page$id"
        itk_component add $pname {
            frame $itk_interior.$pname
        }
        bind $itk_component($pname) <Configure> [itcl::code $this _fixSize]

        if {[llength $_pages] == 0} {
            # select first page by default
            after idle [list catch [list $this current $name]]
        }
        set _pages [linsert $_pages $pos $name]
        set _name2num($name) $id

        lappend rlist $itk_component($pname)
    }

    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: delete -all
# USAGE: delete ?<name> <name>...?
#
# Used to delete one or more pages from the notebook.  With the -all
# flag, it deletes all pages.  Otherwise, it deletes each page
# by name.  You can also delete a page by using an index of the
# form "@n".
# ----------------------------------------------------------------------
itcl::body Rappture::Slideframes::delete {args} {
    if {$args == "-all"} {
        set args $_pages
    }
    foreach name $args {
        set i [index $name]
        set pname [lindex $_pages $i]
        if {$pname != ""} {
            set _pages [lreplace $_pages $i $i]
            set id $_name2num($pname)
            destroy $itk_component(heading$id) $itk_component(page$id)
            unset _name2num($pname)
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: index <name>|@n
#
# Used to convert a page <name> to its corresponding integer index.
# ----------------------------------------------------------------------
itcl::body Rappture::Slideframes::index {name} {
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
# USAGE: page <name>|@n
#
# Used to convert a page <name> to its corresponding frame.
# ----------------------------------------------------------------------
itcl::body Rappture::Slideframes::page {name} {
    set i [index $name]
    set name [lindex $_pages $i]
    set id $_name2num($name)
    return $itk_component(page$id)
}

# ----------------------------------------------------------------------
# USAGE: current ?<name>?
#
# Used to query/set the current page in the notebook.  With no args,
# it returns the name of the current page.  Otherwise, it sets the
# current page.
# ----------------------------------------------------------------------
itcl::body Rappture::Slideframes::current {args} {
    switch -- [llength $args] {
        0 {
            return $_current
        }
        1 {
            set name [lindex $args 0]
            set index [lsearch -exact $_pages $name]
            if {$index < 0} {
                error "can't find page \"$name\""
            }

            set prev $_current
            set _current [lindex $_pages $index]

            if {$prev != $_current} {
                $_tweener configure -from 0 -to 1 -duration 50 -steps 10 \
                    -command [itcl::code $this _fixLayout $_current $prev %v] \
                    -finalize [itcl::code $this _fixLayout end]
            } else {
                _fixLayout
            }
        }
        default {
            error "wrong # args: should be \"current ?name?\""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixSize
#
# Used to handle the requested size for this widget.  If -width or
# -height is 0, then the size is computed based on the requested
# sizes of internal pages.
# ----------------------------------------------------------------------
itcl::body Rappture::Slideframes::_fixSize {} {
    if {"0" == $itk_option(-width)} {
        set wd 0
        foreach pname $_pages {
            set id $_name2num($pname)
            set w [winfo reqwidth $itk_component(page$id)]
            if {$w > $wd} { set wd $w }
        }
        component hull configure -width $wd
    } else {
        component hull configure -width $itk_option(-width)
    }

    if {"0" == $itk_option(-height)} {
        set headht 0
        set pageht 0
        foreach pname $_pages {
            set id $_name2num($pname)
            set h [winfo reqheight $itk_component(heading$id)]
            set headht [expr {$headht + $h}]

            set h [winfo reqheight $itk_component(page$id)]
            if {$h > $pageht} { set pageht $h }
        }
        component hull configure -height [expr {$headht+$pageht}]
    } else {
        component hull configure -height $itk_option(-height)
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixLayout ?<toName>? ?<fromName>? ?<frac>?
#
# Updates the layout of pages within this widget.  With no args, it
# updates the layout based on the current page.  This is good for
# resize operations.  The extra args let us animate from the layout
# with one ID to the layout with another according to a fraction
# <frac> from 0 to 1.
# ----------------------------------------------------------------------
itcl::body Rappture::Slideframes::_fixLayout {args} {
    set atend 0
    if {$args == "end"} {
        set atend 1
        set args ""
    }

    set toName $_current
    set fromName ""
    set frac 1
    if {[llength $args] >= 1} { set toName [lindex $args 0] }
    if {[llength $args] >= 2} { set fromName [lindex $args 1] }
    if {[llength $args] >= 3} { set frac [lindex $args 2] }
    if {[llength $args] > 3} {
        error "wrong # args: should be \"_fixLayout ?id? ?previd? ?frac?\""
    }

    if {$frac == 0 && [string length $itk_option(-animstartcommand)] > 0} {
        if {[catch [list uplevel #0 $itk_option(-animstartcommand)] err]} {
            bgerror $err
        }
    }

    set fromid ""
    if {[info exists _name2num($fromName)]} {
        set fromid $_name2num($fromName)
    }

    set toid ""
    if {[info exists _name2num($toName)]} {
        set toid $_name2num($toName)
    }

    set w [winfo width $itk_component(hull)]
    set h [winfo height $itk_component(hull)]

    # figure out the overall size of title buttons and space left over
    set titlemax 0
    foreach pname $_pages {
        set id $_name2num($pname)
        set titleh [winfo reqheight $itk_component(heading$id)]
        set titlemax [expr {$titlemax + $titleh}]
    }

    set extra [expr {$h - $titlemax}]
    if {$extra < 0} { set extra 0 }

    if {$toid != ""} {
        set pageht1 [winfo reqheight $itk_component(page$toid)]
    } else {
        set pageht1 0
    }
    if {$fromid != ""} {
        set pageht2 [winfo reqheight $itk_component(page$fromid)]
    } else {
        set pageht2 0
    }
    set pageht [expr {$frac*$pageht1 + (1-$frac)*$pageht2}]
    if {$pageht < $extra} {
        set extra $pageht
    }

    # scan through all buttons and place them, along with the pages beneath
    set ypos 0
    foreach pname $_pages {
        set id $_name2num($pname)
        set titleh [winfo reqheight $itk_component(heading$id)]

        # place the heading button
        place $itk_component(heading$id) -x 0 -y $ypos -anchor nw \
            -width $w -height $titleh
        set ypos [expr {$ypos + $titleh}]

        set pageht [winfo reqheight $itk_component(page$id)]
        if {$id == $fromid || $id == $toid} {
            if {$id == $toid} {
                set ht [expr {round($extra * $frac)}]
            } else {
                set ht [expr {round($extra * (1-$frac))}]
            }
            if {$ht > $pageht} { set ht $pageht }
            if {$ht > 0} {
                place $itk_component(page$id) -x 0 -y $ypos -anchor nw \
                    -width $w -height $ht
                set ypos [expr {$ypos + $ht}]
            } else {
                place forget $itk_component(page$id)
            }
        } else {
            place forget $itk_component(page$id)
        }
    }

    if {$atend && [string length $itk_option(-animendcommand)] > 0} {
        if {[catch [list uplevel #0 $itk_option(-animendcommand)] err]} {
            bgerror $err
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -width, -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::Slideframes::width {
    _fixSize
}
itcl::configbody Rappture::Slideframes::height {
    _fixSize
}
