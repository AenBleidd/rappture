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
#  Copyright (c) 2010  Purdue Research Foundation
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

    itk_option define -selectcommand selectCommand SelectCommand ""
    itk_option define -testdir testDir TestDir ""
    itk_option define -toolxml toolXml ToolXml ""

    constructor {args} { #defined later }

    public method getTest {args}
    public method refresh {args}

    protected method getData {id}
    protected method getLeaves {{id 0}}
    protected method getSelected {}
    protected method populate {}
    protected method runSelected {}
    protected method runTest {id} 
    protected method setData {id data}
    protected method updateLabel {}

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
    itk_component add treeview {
        blt::treeview $itk_component(scrollbars).treeview -separator | \
            -autocreate true -selectmode multiple
    } {
        keep -foreground -font -cursor
    }
    $itk_component(treeview) column insert 0 result -width 75 
    $itk_component(treeview) column insert end test -hide yes
    $itk_component(scrollbars) contents $itk_component(treeview)

    itk_component add bottomBar {
        frame $itk_interior.bottomBar
    }
    pack $itk_component(bottomBar) -fill x -side bottom

    itk_component add bSelectAll {
        button $itk_component(bottomBar).bSelectAll -text "Select all" \
            -command "$itk_component(treeview) selection set 0 end" 
    }
    pack $itk_component(bSelectAll) -side left

    itk_component add bSelectNone {
        button $itk_component(bottomBar).bSelectNone -text "Select none" \
            -command "$itk_component(treeview) selection clearall"
    }
    pack $itk_component(bSelectNone) -side left

    itk_component add bRun {
        button $itk_component(bottomBar).bRun -text "Run" -state disabled \
            -command [itcl::code $this runSelected]
    } 
    pack $itk_component(bRun) -side right

    itk_component add lSelected {
        label $itk_component(bottomBar).lSelected -text "0 tests selected"
    }
    pack $itk_component(lSelected) -side right -padx 5

    # TODO: Fix black empty space when columns are shrunk

    pack $itk_component(scrollbars) -side left -expand yes -fill both

    eval itk_initialize $args

    if {$itk_option(-testdir) == ""} {
        error "no -testdir configuration option given."
    }
    if {$itk_option(-toolxml) == ""} {
        error "no -toolxml configuration option given."
    }
}

# TODO: destructor

# ----------------------------------------------------------------------
# Repopulate tree if test directory or toolxml have been changed.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::TestTree::testdir {
    if {$itk_option(-toolxml) != ""} {
        populate
    }
}

itcl::configbody Rappture::Tester::TestTree::toolxml {
    if {$itk_option(-testdir) != ""} {
        populate
    }
}

# ----------------------------------------------------------------------
# Forward the TestTree's selectcommand to the treeview, but tack on the
# updateLabel method to keep the label refreshed when selection is
# changed
# ----------------------------------------------------------------------
itcl::configbody Rappture::Tester::TestTree::selectcommand {
    $itk_component(treeview) configure -selectcommand \
        "[itcl::code $this updateLabel]; $itk_option(-selectcommand)"
}

# ----------------------------------------------------------------------
# USAGE getTest ?id?
#
# Returns the test object associated with a given treeview node id.  If
# no id is given, return the test associated with the currently focused
# node.  Returns empty string if the given id / focused node is a 
# branch node.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::getTest {args} {
    if {[llength $args] == 0} {
         set id [$itk_component(treeview) index focus]
    } elseif {[llength $args] == 1} {
        set id [lindex $args 0]
    } else {
        error "wrong # args: should be getTest ?id?"
    } 
    array set darray [getData $id]
    if {[lsearch -exact [getLeaves] $id] == -1} {
        # Return empty string if branch node selected
        return ""
    }
    return $darray(test)
}

# ----------------------------------------------------------------------
# USAGE: refresh ?id?
#
# Refreshes the result column and any other information which may be
# added later for the given tree node id.  Mainly needed to update the
# result from Fail to Pass after regoldenizing a test.  If no id is 
# given, return the test associated with the currently focused node.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::refresh {args} {
    if {[llength $args] == 0} {
         set id [$itk_component(treeview) index focus]
    } elseif {[llength $args] == 1} {
        set id [lindex $args 0]
    } else {
        error "wrong # args: should be refresh ?id?"
    }
    if {[lsearch -exact [getLeaves] $id] == -1} {
         error "given id $id is not a leaf node."
    }
    set test [getTest $id]
    setData $id [list result [$test getResult] test $test]
}

# ----------------------------------------------------------------------
# USAGE: getData <id>
#
# Returns a list of key-value pairs representing the column data stored
# at the tree node with the given id.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::getData {id} {
    return [$itk_component(treeview) entry cget $id -data]
}

# ----------------------------------------------------------------------
# USAGE: getLeaves ?id?
#
# Returns a list of ids for all tests contained in the tree.  If an
# optional id is given as an input parameter, then the returned list
# will contain the ids of all tests that are descendants of the given
# id.  Tests can only be leaf nodes.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::getLeaves {{id 0}} {
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
        set tests [concat $tests [getLeaves $child]]
    }
    return $tests
}

# ----------------------------------------------------------------------
# USAGE: getSelected
#
# Returns a list ids for all currently selected tests (leaf nodes) and 
# the child tests of any currently selected branch nodes.  Tests can 
# only be leaf nodes in the tree (the ids in the returned list will 
# correspond to leaf nodes only).
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::getSelected {} {
    set selection [$itk_component(treeview) curselection]
    set selectedTests [list]
    foreach id $selection {
        foreach node [getLeaves $id] {
            if {[lsearch -exact $selectedTests $node] == -1} {
                lappend selectedTests $node
            }
        }
    }
    return $selectedTests
}

# ----------------------------------------------------------------------
# USAGE: populate
#
# Used internally to insert nodes into the treeview for each test xml
# found in the test directory.  Skips any xml files that do not contain
# information at path test.label.  Relies on the autocreate treeview
# option so that branch nodes need not be explicitly created.  Deletes
# any existing contents.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::populate {} {
    # TODO: Delete existing test objects
    $itk_component(treeview) delete 0
    # TODO: add an appropriate icon
    set icon [Rappture::icon molvis-3dorth]
    # TODO: Descend through subdirectories inside testdir?
    foreach testxml [glob -nocomplain -directory $itk_option(-testdir) *.xml] {
        set lib [Rappture::library $testxml]
        set testpath [$lib get test.label]
        if {$testpath != ""} {
            set test [Rappture::Tester::Test ::#auto \
                $itk_option(-toolxml) $testxml]
            $itk_component(treeview) insert end $testpath -data \
                 [list test $test] -icons "$icon $icon" \
                 -activeicons "$icon $icon"
        }
    }
    $itk_component(treeview) open -recurse root
    # TODO: Fix width of main treeview column
}

# ----------------------------------------------------------------------
# USAGE: runSelected
#
# Invoked by the run button to run all currently selected tests.
# After completion, call selectcommand to re-select the newly completed
# focused node.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::runSelected {} {
    foreach id [$this getSelected] {
        runTest $id
    }
    # Try calling selectcommand with the -refresh option.  If selectcommand
    # does not accept this argument, then call it with no arguments.
    if {[catch {eval $itk_option(-selectcommand) -refresh}]} {
        eval $itk_option(-selectcommand)
    }
}

# ----------------------------------------------------------------------
# USAGE: runTest id
#
# Runs the test located at the tree node with the given id.  The id
# must be a leaf node, because tests may not be located at branch nodes.
# ---------------------------------------------------------------------- 
itcl::body Rappture::Tester::TestTree::runTest {id} {
    if {[lsearch -exact [getLeaves] $id] == -1} {
        error "given id $id is not a leaf node"
    }
    set test [getTest $id]
    setData $id [list result Running test $test]
    $test run
    setData $id [list result [$test getResult] test $test]
}

# ----------------------------------------------------------------------
# USAGE: setData <id> <data>
#
# Accepts a node id and a list of key-value pairs.  Stored the list as
# column data associated with the tree node with the given id.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::setData {id data} {
    $itk_component(treeview) entry configure $id -data $data
}

# ----------------------------------------------------------------------
# USAGE: updateLabel
#
# Used internally to update the label which indicates how many tests
# are currently selected.  Also disables the run button if no tests are
# selected.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::updateLabel {} {
    set n [llength [getSelected]]
    if {$n == 1} {
        $itk_component(lSelected) configure -text "1 test selected"
    } else {
        $itk_component(lSelected) configure -text "$n tests selected"
    }

    if {$n > 0} {
        $itk_component(bRun) configure -state normal
    } else {
        $itk_component(bRun) configure -state disabled
    }
}

