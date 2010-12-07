# ----------------------------------------------------------------------
#  COMPONENT: testtree - provides hierarchical view of regression tests
#
#  Used to display a collapsible view of all tests found in the test
#  directory.  The -command configuration option will be called when
#  the run button is clicked. Provides methods to get all tests or all 
#  currently selected tests. Also helps handle data stored in treeview 
#  columns.  In each test xml, a label must be located at the path 
#  test.label.  Test labels may be organized hierarchically by using 
#  dots to separate components of the test label.
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

    public variable command
    public variable selectcommand
    public variable testdir

    constructor {testdir args} { #defined later }
    public method getTests {{id 0}}
    public method getSelected {}
    public method getData {id}
    public method setData {id data}

    private method updateLabel {}
    private method populate {}

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
        blt::treeview $itk_component(scrollbars).treeview -separator . \
            -autocreate true -selectmode multiple 
    } {
        keep -foreground -font -cursor
    }
    $itk_component(treeview) column insert 0 result -width 75 
    $itk_component(treeview) column insert end testxml ran diffs runfile 
    $itk_component(treeview) column configure testxml ran diffs runfile \
        -hide yes
    $itk_component(treeview) column configure runfile -hide no
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
        button $itk_component(bottomBar).bRun -text "Run" -state disabled
    } {
        keep -command
    }
    pack $itk_component(bRun) -side right

    itk_component add lSelected {
        label $itk_component(bottomBar).lSelected -text "0 tests selected"
    }
    pack $itk_component(lSelected) -side right -padx 5

    # TODO: Fix black empty space when columns are shrunk

    pack $itk_component(scrollbars) -side left -expand yes -fill both

    eval itk_initialize $args
}

# Repopulate tree if test directory changed
itcl::configbody Rappture::Tester::TestTree::testdir {
    populate
}

# Forward the TestTree's selectcommand to the treeview, and update the label
# as well.
itcl::configbody Rappture::Tester::TestTree::selectcommand {
    $itk_component(treeview) configure -selectcommand \
        "[itcl::code $itk_interior updateLabel]; $selectcommand"
}

# ----------------------------------------------------------------------
# USAGE: populate
#
# Used internally to insert nodes into the treeview for each test xml
# found in the test directory.  Skips any xml files that do not contain
# information at path test.label.  Relies on the autocreate treeview
# option so that branch nodes need not be explicitly created.  Deletes
# any nodes previously contained by the treeview.
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::populate {} {
    $itk_component(treeview) delete 0
    # TODO: add an appropriate icon
    set icon [Rappture::icon download]
    # TODO: Descend through subdirectories inside testdir?
    foreach testxml [glob -nocomplain -directory $testdir *.xml] {
        set lib [Rappture::library $testxml]
        set testpath [$lib get test.label]
        if {$testpath != ""} {
            $itk_component(treeview) insert end $testpath -data \
                 [list testxml $testxml ran no result "" diffs ""] \
                 -icons "$icon $icon" -activeicons "$icon $icon"
        }
    }
    $itk_component(treeview) open -recurse root
    # TODO: Fix width of main treeview column
}

# ----------------------------------------------------------------------
# USAGE: getTests ?id?
#
# Returns a list of ids for all tests contained in the tree.  If an
# optional id is given as an input parameter, then the returned list
# will contain the ids of all tests that are descendants of the given
# id.  Tests can only be leaf nodes in the tree (the ids in the returned
# list will correspond to leaf nodes only).
# ----------------------------------------------------------------------
itcl::body Rappture::Tester::TestTree::getTests {{id 0}} {
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
        set tests [concat $tests [getTests $child]]
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
        foreach node [getTests $id] {
            if {[lsearch -exact $selectedTests $node] == -1} {
                lappend selectedTests $node
            }
        }
    }
    return $selectedTests
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

