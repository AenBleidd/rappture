# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: notebook - series of pages, but only one packed at a time
#
#  This widget acts as the core of a tabbed notebook, without the tabs.
#  It allows you to create a series of pages, and display one page at
#  time.  The overall widget is just big enough to accommodate all of
#  the pages within it.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Notebook.width 0 widgetDefault
option add *Notebook.height 0 widgetDefault

itcl::class Rappture::Notebook {
    inherit itk::Widget

    itk_option define -width width Width 0
    itk_option define -height height Height 0

    constructor {args} { # defined below }
    destructor { # defined below }

    public method insert {pos args}
    public method delete {args}
    public method index {name}
    public method page {name}
    public method current {args}

    protected method _fixSize {}

    private variable _count 0       ;# counter for unique names
    private variable _dispatcher "" ;# dispatcher for !events
    private variable _pages ""      ;# list of page frames
    private variable _name2page     ;# maps name => frame for page
    private variable _current ""    ;# page currently shown
}

itk::usual Notebook {
    keep -background -cursor
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Notebook::constructor {args} {
    pack propagate $itk_component(hull) no

    Rappture::dispatcher _dispatcher
    $_dispatcher register !fixsize
    $_dispatcher dispatch $this !fixsize "[itcl::code $this _fixSize]; list"

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> ?<name> <name>...?
#
# Used to insert one or more pages in the notebook.  Each page is
# identified by its <name>.  Returns the widget name for each
# page created.
# ----------------------------------------------------------------------
itcl::body Rappture::Notebook::insert {pos args} {
    set rlist ""
    foreach name $args {
        if {[lsearch $_pages $name] >= 0} {
            error "page \"$name\" already exists"
        }
        set pname "page[incr _count]"
        itk_component add $pname {
            frame $itk_interior.$pname
        }
        set _pages [linsert $_pages $pos $name]
        set _name2page($name) $itk_component($pname)

        bind $itk_component($pname) <Configure> \
            [itcl::code $_dispatcher event -after 100 !fixsize]

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
itcl::body Rappture::Notebook::delete {args} {
    if {$args == "-all"} {
        set args $_pages
    }
    foreach name $args {
        set i [index $name]
        set pname [lindex $_pages $i]
        if {$pname != ""} {
            set _pages [lreplace $_pages $i $i]
            destroy $_name2page($pname)
            unset _name2page($pname)
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: index <name>|@n
#
# Used to convert a page <name> to its corresponding integer index.
# ----------------------------------------------------------------------
itcl::body Rappture::Notebook::index {name} {
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
itcl::body Rappture::Notebook::page {name} {
    set i [index $name]
    set name [lindex $_pages $i]
    return $_name2page($name)
}

# ----------------------------------------------------------------------
# USAGE: current ?<name>|next>|<back?
#
# Used to query/set the current page in the notebook.  With no args,
# it returns the name of the current page.  Otherwise, it sets the
# current page.  The special token "next>" is used to set the notebook
# to the next logical page, and "<back" sets to the previous.
# ----------------------------------------------------------------------
itcl::body Rappture::Notebook::current {args} {
    switch -- [llength $args] {
        0 {
            return $_current
        }
        1 {
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
            } else {
                set index [lsearch -exact $_pages $name]
                if {$index < 0} {
                    error "can't move to page \"$name\""
                }
            }

            set _current [lindex $_pages $index]

            foreach w [pack slaves $itk_component(hull)] {
                pack forget $w
            }
            pack $_name2page($_current) -expand yes -fill both
        }
        default {
            error "wrong # args: should be \"current name|next>|<back\""
        }
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
itcl::body Rappture::Notebook::_fixSize {} {
    if {$itk_option(-width) <= 0} {
        set maxw 0
        foreach name $_pages {
            set w [winfo reqwidth $_name2page($name)]
            if {$w > $maxw} { set maxw $w }
        }
        component hull configure -width $maxw
    } else {
        component hull configure -width $itk_option(-width)
    }

    if {$itk_option(-height) <= 0} {
        set maxh 0
        foreach name $_pages {
            set h [winfo reqheight $_name2page($name)]
            if {$h > $maxh} { set maxh $h }
        }
        component hull configure -height $maxh
    } else {
        component hull configure -height $itk_option(-height)
    }
}

# ----------------------------------------------------------------------
# OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::Notebook::width {
    $_dispatcher event -idle !fixsize
}

# ----------------------------------------------------------------------
# OPTION: -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::Notebook::height {
    $_dispatcher event -idle !fixsize
}
