# ----------------------------------------------------------------------
#  COMPONENT: pager - notebook for displaying pages of widgets
#
#  This widget is something like a tabbed notebook, but with a little
#  more flexibility.  Pages can be inserted and deleted, and then shown
#  in various arrangements.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *Pager.width 0 widgetDefault
option add *Pager.height 0 widgetDefault
option add *Pager.arrangement "tabs/top" widgetDefault
option add *Pager.tearoff 0 widgetDefault

itcl::class Rappture::Pager {
    inherit itk::Widget

    itk_option define -arrangement arrangement Arrangement ""
    itk_option define -width width Width 0
    itk_option define -height height Height 0

    constructor {args} { # defined below }

    public method insert {pos args}
    public method delete {first {last ""}}
    public method index {name}
    public method get {{name ""}}

    protected method _layout {}

    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _pages ""       ;# list of known pages
    private variable _page2frame     ;# maps page name => frame
    private variable _counter 0      ;# counter for frame names
    private variable _arrangement "" ;# last value of -arrangment
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

    itk_component add tabs {
        blt::tabset $itk_interior.tabs -borderwidth 0 -relief flat \
            -side bottom -selectcommand [itcl::code $this _layout]
    } {
        keep -activebackground -activeforeground
        keep -background -cursor -font
        rename -highlightbackground -background background Background
        keep -highlightcolor -highlightthickness
        keep -selectbackground -selectforeground
        keep -tabbackground -tabforeground
        keep -tearoff
    }
    pack $itk_component(tabs) -expand yes -fill both

    itk_component add inside {
        frame $itk_component(tabs).inside
    }
    $_dispatcher event -idle !layout

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> <name> ?<name>...?
#
# Clients use this to insert one or more new pages into this pager.
# The pages are inserted into the list at position <pos>, which can
# be an integer starting from 0 or the keyword "end".  Each <name>
# is the name used to identify the page.  Returns the name of a frame
# for each page created.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::insert {pos args} {
    if {"end" == $pos} {
        set pos [llength $_pages]
    } elseif {![string is integer $pos]} {
        error "bad index \"$pos\": should be integer or \"end\""
    }

    set rlist ""
    foreach name $args {
        if {[info exists _page2frame($name)]} {
            error "page \"$name\" already exists"
        }
        set win $itk_component(inside).page[incr _counter]
        frame $win
        set _page2frame($name) $win
        set _pages [linsert $_pages $pos $name]
        lappend rlist $win

        if {[string match tabs/* $_arrangement]} {
            $itk_component(tabs) insert $pos $name
        }
    }
    $_dispatcher event -idle !layout

    return $rlist
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
        if {[info exists _page2frame($name)]} {
            destroy $_page2frame($name)
            unset _page2frame($name)
        }
    }
    set _pages [lreplace $_pages $first $last]

    if {[string match tabs/* $_arrangement]} {
        $itk_component(tabs) delete $first $last
    }
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# USAGE: index <name>
#
# Clients use this to convert a page <name> into its corresponding
# integer index.  Returns -1 if the <name> is not recognized.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::index {name} {
    return [lsearch -exact $_pages $name]
}

# ----------------------------------------------------------------------
# USAGE: get ?<name>?
#
# Clients use this to get information about pages.  With no args, it
# returns a list of all page names.  Otherwise, it returns the frame
# associated with a page name.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::get {{name ""}} {
    if {$name == ""} {
        return $_pages
    }
    if {[info exists _page2frame($name)]} {
        return $_page2frame($name)
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: _layout
#
# Used internally to fix the current page management whenever pages
# are added or deleted, or when the page arrangement changes.
# ----------------------------------------------------------------------
itcl::body Rappture::Pager::_layout {} {
    #
    # If the new arrangement doesn't match the last one, then
    # clear the effects of the old arrangement.
    #
    regexp {(.*)/?} $_arrangement match oldatype
    regexp {(.*)/?} $itk_option(-arrangement) match newatype

    if {$newatype != $oldatype} {
        switch -glob -- $_arrangement {
            tabs/* {
                foreach name $_pages {
                    pack forget $_page2frame($name)
                }
                pack forget $itk_component(inside)
                catch {$itk_component(tabs) delete 0 end}
            }
            stack {
                foreach name $_pages {
                    pack forget $_page2frame($name)
                }
            }
        }
        switch -glob -- $itk_option(-arrangement) {
            tabs/* {
                foreach name $_pages {
                    $itk_component(tabs) insert end $name
                }
                if {[llength $_pages] > 0} {
                    $itk_component(tabs) select 0
                }
            }
        }
    }
    set _arrangement $itk_option(-arrangement)

    #
    # Apply the new arrangement.
    #
    switch -glob -- $itk_option(-arrangement) {
        tabs/* {
            set side [lindex [split $itk_option(-arrangement) /] 1]
            if {$side == ""} { set side "top" }
            $itk_component(tabs) configure -side $side

            if {[llength $_pages] <= 1} {
                pack $itk_component(inside) -expand yes -fill both
                set first [lindex $_pages 0]
                if {$first != ""} {
                    pack $_page2frame($first) -expand yes -fill both
                }
            } else {
                pack forget $itk_component(inside)
                set i [$itk_component(tabs) index select]
                if {$i != ""} {
                    set name [$itk_component(tabs) get $i]
                    $itk_component(tabs) tab configure $name \
                        -window $itk_component(inside) -fill both
                }

                foreach name $_pages {
                    pack forget $_page2frame($name)
                }
                if {$i != ""} {
                    set name [lindex $_pages $i]
                    if {$name != ""} {
                        pack $_page2frame($name) -expand yes -fill both
                    }
                }
            }
        }
        stack {
            foreach name $_pages {
                pack forget $_page2frame($name)
            }
            foreach name $_pages {
                pack $_page2frame($name) -expand yes -fill both
            }
            pack $itk_component(inside) -expand yes -fill both
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -arrangement
# ----------------------------------------------------------------------
itcl::configbody Rappture::Pager::arrangement {
    set legal {tabs/top tabs/bottom tabs/left tabs/right stack}
    if {[lsearch -exact $legal $itk_option(-arrangement)] < 0} {
        error "bad option \"$itk_option(-arrangement)\": should be one of [join [lsort $legal] {, }]"
    }
    $_dispatcher event -idle !layout
}

source dispatcher.tcl

Rappture::Pager .p
pack .p -expand yes -fill both

set f [.p component inside]
label $f.top -text "top"
pack $f.top -fill x

set f [.p insert end "Electrical"]
label $f.l -text "Electrical" -background black -foreground white
pack $f.l -expand yes -fill both

set f [.p insert end "Doping"]
label $f.l -text "Doping" -background black -foreground white
pack $f.l -expand yes -fill both
