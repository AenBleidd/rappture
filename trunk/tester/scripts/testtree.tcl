# ----------------------------------------------------------------------
#  COMPONENT: testtree - provides hierarchical view of regression tests
#
#  Used to display a collapsible view of all tests found in the test
#  directory.  In each test xml, a label must be located at the path 
#  test.label.  Test labels may be organized hierarchically by using 
#  dots to separate components of the test label.  The directory
#  containing a set of test xml files, as well as the location of the
#  new tool.xml must be given as configuration options.
# ======================================================================
#  AUTHOR:  Ben Rafferty, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT
package require Rappture

namespace eval Rappture::Tester::TestTree { #forward declaration }

option add *TestTree.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *TestTree.codeFont \
    -*-courier-medium-r-normal-*-12-* widgetDefault
option add *TestTree.textFont \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *TestTree.boldTextFont \
    -*-helvetica-bold-r-normal-*-12-* widgetDefault

itcl::class Rappture::Tester::TestTree {
    inherit itk::Widget 

    constructor {args} { #defined later }
    destructor { #defined later }

    public method add {args}
    public method clear {}
    public method curselection {}

    protected method _getLeaves {{id 0}}
    protected method _getTest {id}
    protected method _refresh {args}

    # add support for a spinning icon
    proc spinner {op}

    private common spinner
    set spinner(frames) 8
    set spinner(current) 0
    set spinner(pending) ""
    set spinner(uses) 0

    for {set n 0} {$n < $spinner(frames)} {incr n} {
        set spinner(frame$n) [Rappture::icon circle-ball[expr {$n+1}]]
    }
    set spinner(image) [image create photo -width [image width $spinner(frame0)] -height [image height $spinner(frame0)]]
}
 
itk::usual TestTree {
    keep -background -foreground -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::constructor {args} {
    itk_component add scrollbars {
        Rappture::Scroller $itk_interior.scroller \
            -xscrollmode auto -yscrollmode auto
    }
    pack $itk_component(scrollbars) -expand yes -fill both

    itk_component add treeview {
        blt::treeview $itk_component(scrollbars).treeview -separator | \
            -autocreate true -selectmode multiple \
            -icons [list [Rappture::icon folder] [Rappture::icon folder2]] \
            -activeicons ""
    } {
        keep -foreground -font -cursor
        keep -selectcommand
    }
    $itk_component(treeview) column insert 0 result -title "Result"
    $itk_component(treeview) column insert end test -hide yes
    $itk_component(treeview) column configure treeView -justify left -title "Test Case"
    $itk_component(treeview) sort configure -mode dictionary -column treeView
    $itk_component(treeview) sort auto yes

    $itk_component(scrollbars) contents $itk_component(treeview)

    itk_component add bottomBar {
        frame $itk_interior.bottomBar
    }
    pack $itk_component(bottomBar) -fill x -side bottom -pady {8 0}

    itk_component add selLabel {
        label $itk_component(bottomBar).selLabel -anchor w -text "Select:"
    }
    pack $itk_component(selLabel) -side left

    itk_component add bSelectAll {
        button $itk_component(bottomBar).bSelectAll -text "All" \
            -command "$itk_component(treeview) selection set 0 end" 
    }
    pack $itk_component(bSelectAll) -side left

    itk_component add bSelectNone {
        button $itk_component(bottomBar).bSelectNone -text "None" \
            -command "$itk_component(treeview) selection clearall"
    }
    pack $itk_component(bSelectNone) -side left


    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::destructor {} {
    clear
}

# ----------------------------------------------------------------------
# USAGE: add ?<testObj> <testObj> ...?
#
# Adds one or more Test objects to the tree shown in this viewer.
# Once added, these objects become property of this widget and
# are destroyed when the widget is cleared or deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::add {args} {
    set icon [Rappture::icon testcase]

    foreach obj $args {
        if {[catch {$obj isa Rappture::Tester::Test} valid] || !$valid} {
            error "bad value \"$obj\": should be Test object"
        }

        # add each Test object into the tree
        set testpath [$obj getTestInfo test.label]
        set n [$itk_component(treeview) insert end $testpath \
             -data [list test $obj] -icons [list $icon $icon]]

        # tag this node so we can find it easily later
        $itk_component(treeview) tag add $obj $n

        # monitor state changes on the object
        $obj configure -notifycommand [itcl::code $this _refresh]
    }
}

# ----------------------------------------------------------------------
# USAGE: clear
#
# Clears the contents of the tree so that it's completely empty.
# All Test objects stored internally are destroyed.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::clear {} {
    foreach id [_getLeaves] {
        itcl::delete object [_getTest $id]
    }
    $itk_component(treeview) delete 0
}

# ----------------------------------------------------------------------
# USAGE: curselection
#
# Returns a list ids for all currently selected tests (leaf nodes) and 
# the child tests of any currently selected branch nodes.  Tests can 
# only be leaf nodes in the tree (the ids in the returned list will 
# correspond to leaf nodes only).
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::curselection {} {
    set rlist ""
    foreach id [$itk_component(treeview) curselection] {
        foreach node [_getLeaves $id] {
            catch {unset data}
            array set data [$itk_component(treeview) entry cget $node -data]

            if {[lsearch -exact $rlist $data(test)] < 0} {
                lappend rlist $data(test)
            }
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE _getTest <nodeId>
#
# Returns the test object associated with a given treeview node id.  If
# no id is given, return the test associated with the currently focused
# node.  Returns empty string if the given id / focused node is a 
# branch node.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::_getTest {id} {
    if {[lsearch -exact [_getLeaves] $id] < 0} {
        # Return empty string if branch node selected
        return ""
    }
    array set darray [$itk_component(treeview) entry cget $id -data]
    return $darray(test)
}

# ----------------------------------------------------------------------
# USAGE: _refresh ?<testObj> <testObj> ...?
#
# Invoked whenever the state of a <testObj> changes.  Finds the
# corresponding entry in the tree and updates the "Result" column
# to show the new status.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::_refresh {args} {
    foreach obj $args {
        set n [$itk_component(treeview) index $obj]
        if {$n ne ""} {
            catch {unset data}
            array set data [$itk_component(treeview) entry cget $n -data]

            # getting rid of a spinner? then drop it
            if {[info exists data(result)]
                  && $data(result) == "@$spinner(image)"} {
                spinner drop
            }

            # plug in the new icon
            switch -- [$obj getResult] {
                Pass    { set data(result) "@[Rappture::icon pass16]" }
                Fail    { set data(result) "@[Rappture::icon fail16]" }
                Waiting { set data(result) "@[Rappture::icon wait16]" }
                Running { set data(result) "@[spinner use]" }
                default { set data(result) "" }
            }
            $itk_component(treeview) entry configure $n -data [array get data]

            # if the node that's changed is selected, invoke the
            # -selectcommand code so the GUI will react to the new state
            if {[$itk_component(treeview) selection includes $n]} {
                set cmd [$itk_component(treeview) cget -selectcommand]
                if {[string length $cmd] > 0} {
                    uplevel #0 $cmd
                }
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _getLeaves ?id?
#
# Returns a list of ids for all tests contained in the tree.  If an
# optional id is given as an input parameter, then the returned list
# will contain the ids of all tests that are descendants of the given
# id.  Tests can only be leaf nodes.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::_getLeaves {{id 0}} {
    set clist [$itk_component(treeview) entry children $id]
    if {$clist == "" && $id == 0} {
        # Return nothing if tree is empty 
        return ""
    }
    if {$clist == ""} {
        return $id
    }
    set tests [list]
    foreach child $clist {
        set tests [concat $tests [_getLeaves $child]]
    }
    return $tests
}

# ----------------------------------------------------------------------
# USAGE: spinner use|drop|next
#
# Used to update the spinner icon that represents running test cases.
# The "use" option returns the spinner icon and starts the animation,
# if it isn't already running.  The "drop" operation lets go of the
# spinner.  If nobody is using it, the animation stops.  The "next"
# option is used internally to change the animation to the next frame.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::spinner {op} {
    switch -- $op {
        use {
            if {$spinner(pending) == ""} {
                set spinner(current) 0
                set spinner(pending) [after 100 Rappture::Tester::TestTree::spinner next]
            }
            incr spinner(uses)
            return $spinner(image)
        }
        drop {
            if {[incr spinner(uses) -1] <= 0} {
                after cancel $spinner(pending)
                set spinner(pending) ""
                set spinner(uses) 0
            }
        }
        next {
            set n $spinner(current)
            $spinner(image) copy $spinner(frame$n)

            # go to the next frame
            if {[incr spinner(current)] >= $spinner(frames)} {
                set spinner(current) 0
            }

            # update again after a short delay
            set spinner(pending) [after 100 Rappture::Tester::TestTree::spinner next]
        }
        default {
            error "bad option \"$op\": should be use, drop, next"
        }
    }
}
