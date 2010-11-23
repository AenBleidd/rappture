# ----------------------------------------------------------------------
#  COMPONENT: testtree - provides hierarchical view of regression tests
#
#  Used to display a collapsible view of all tests found in the test
#  directory.  Essentially an Itk Widget wrapper for blt::treeview.
#  Provides methods to get all tests or all currently selected tests.
#  Also helps handle data stored in treeview columns.  In each test xml,
#  a label must be located at the path test.label.  Test labels may be
#  be organized hierarchically by using dots to separate components of
#  the test label.  (example: roomtemp.1eV)
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

namespace eval Rappture::Regression::TestTree { #forward declaration }

itcl::class Rappture::Regression::TestTree {
    inherit itk::Widget

    constructor {testdir} { #defined later }
    public method populate {testdir}
    public method getTests {{id 0}}
    public method getSelected {}
    public method getData {id}
    public method setData {id data}
}

# TODO: figure out exactly what should go in here.
itk::usual TestTree {
    keep -background -foreground -font
}
itk::usual TreeView {
    keep -background -foreground -font
}

itcl::body Rappture::Regression::TestTree::constructor {testdir} {
    # TODO: Use separate tree data structure and insert into treeview
    puts "Constructing TestTree."
    itk_component add treeview {
        blt::treeview $itk_interior.treeview -separator . -autocreate true \
            -selectmode multiple
    }
    $itk_component(treeview) column insert end xmlfile ran result diffs
    $itk_component(treeview) column configure xmlfile ran diffs -hide yes
    pack $itk_component(treeview) -expand yes -fill both
    populate $testdir
    # TODO: Fix default column spacing. Column name for the main/first column?
    # TODO: Fix black empty space when columns are shrunk
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
itcl::body Rappture::Regression::TestTree::populate {testdir} {
    puts "Populating TestTree."
    $itk_component(treeview) delete 0
    # TODO: Make file icon background transparent.
    set icon [image create photo -file images/file.gif]
    # TODO: Descend through subdirectories inside testdir?
    foreach testxml [glob -nocomplain -directory $testdir *.xml] {
        set lib [Rappture::library $testxml]
        set testpath [$lib get test.label]
        if {$testpath != ""} {
            $itk_component(treeview) insert end $testpath -data \
                 [list xmlfile $testxml ran no result "" diffs ""] \
                 -icons "$icon $icon" -activeicons "$icon $icon"
            }
        }
    $itk_component(treeview) open -recurse root
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
itcl::body Rappture::Regression::TestTree::getTests {{id 0}} {
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
itcl::body Rappture::Regression::TestTree::getSelected {} {
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
# USAGE: getData id
#
# Returns a list of key-value pairs representing the column data stored
# at the tree node with the given id.
# ----------------------------------------------------------------------
itcl::body Rappture::Regression::TestTree::getData {id} {
    return [$itk_component(treeview) entry cget $id -data]
}

# ----------------------------------------------------------------------
# USAGE: setData id data
#
# Accepts a node id and a list of key-value pairs.  Stored the list as
# column data associated with the tree node with the given id.
# ----------------------------------------------------------------------
itcl::body Rappture::Regression::TestTree::setData {id data} {
    $itk_component(treeview) entry configure $id -data $data
}


