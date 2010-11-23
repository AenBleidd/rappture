# ----------------------------------------------------------------------
#  COMPONENT: mainwin - main application window for regression tester 
#
#  This widget acts as the main window for the Rappture regression
#  tester.  Constructor accepts the location of the new version to be
#  tested, and the location of a directory containg test xml files.
# ======================================================================
#  AUTHOR:  Ben Rafferty, Purdue University
#  Copyright (c) 2010  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require Rappture
package require RapptureGUI

namespace eval Rappture::Regression::MainWin { #forward declaration }

itcl::class Rappture::Regression::MainWin {
    inherit itk::Toplevel

    constructor {tooldir testdir} { #defined later }
    public method runAll {}
    public method runSelected {}
    private method runTest {id}
    private method makeDriver {testxml}

    private variable _testdir
    private variable _tooldir
    private variable _toolxml

}

# TODO: figure out exactly what should go in here.
itk::usual TestView {
    keep -background -foreground -font
}

itk::usual Panedwindow {
    keep -background
}

itcl::body Rappture::Regression::MainWin::constructor {tooldir testdir} {
    puts "Constructing MainWin."

    # TODO: Replace explicit constructor arguments with "args"
    #       If tooldir is not given, use current directory.
    #       If testdir is not given, look for it inside of tooldir.
    if {[file isdirectory $tooldir]} {
        set _tooldir $tooldir
    } else {
        # TODO: Properly format error messages
        error "Given tooldir is not a directory."
    }
    if {[file isdirectory $testdir]} {
        set _testdir $testdir
    } else {
        error "Given testdir is not a directory"
    }

    # TODO: Check other locations for tool.xml and throw error if not found
    set _toolxml [file join $tooldir tool.xml]

    itk_component add topBar {
        frame $itk_interior.topBar
    }
    itk_component add cmdLabel {
        label $itk_component(topBar).cmdLabel -text "Tool command:"
    }
    itk_component add cmdEntry {
        entry $itk_component(topBar).cmdEntry
    }
    itk_component add bRunAll {
        button $itk_component(topBar).bRunAll -text "Run all" \
            -command [itcl::code $itk_interior runAll] 
    }
    itk_component add bRunSelected {
        button $itk_component(topBar).bRunSelected -text "Run selected" \
            -command [itcl::code $itk_interior runSelected]
    }
    pack $itk_component(cmdLabel) -side left
    pack $itk_component(cmdEntry) -side left -expand yes -fill x
    pack $itk_component(bRunAll) -side right
    pack $itk_component(bRunSelected) -side right
    pack $itk_component(topBar) -side top -fill x

    itk_component add pane {
        panedwindow $itk_interior.pane
    }
    itk_component add tree {
        Rappture::Regression::TestTree $itk_component(pane).tree $testdir
    }
    itk_component add view {
        Rappture::Regression::TestView $itk_component(pane).view
    }
    $itk_component(pane) add  $itk_component(pane).tree -sticky nesw
    $itk_component(pane) add  $itk_component(pane).view -sticky nesw
    # TODO: make panes scale proportionally when window grows
    pack $itk_component(pane) -side left -expand yes -fill both
}

# ----------------------------------------------------------------------
# USAGE: runAll
#
# When this method is invoked, all tests contained in the TestTree will
# be ran sequentially.
# ----------------------------------------------------------------------
itcl::body Rappture::Regression::MainWin::runAll {} {
    # TODO: Add force or ifneeded flag and propagate to runTest
    puts "Running all tests."
    set tests [$itk_component(tree) getTests]
    foreach id $tests {
        runTest $id
    }
}

# ----------------------------------------------------------------------
# USAGE: runSelected
#
# When this method is invoked, all tests that are currently selected
# will be ran.  If a branch node (folder) is selected, all of its
# descendant tests will be ran as well.
# ----------------------------------------------------------------------
itcl::body Rappture::Regression::MainWin::runSelected {} {
    # TODO: Add force or ifneeded flag and propagate to runTest
    puts "Running selected tests."
    set selected [$itk_component(tree) getSelected]
    foreach id $selected {
        runTest $id
    }
}

# ----------------------------------------------------------------------
# USAGE: runTest id
#
# Called by runAll and runSelected to run a single test at tree node 
# specified by the given id.  In most cases, this method should not be
# called directly.  Results given by the new version are compared to
# the test xml by the compare procedure in compare.tcl
# ----------------------------------------------------------------------
itcl::body Rappture::Regression::MainWin::runTest {id} {
    # TODO: Add force or ifneeded flag and check the "ran" element of the
    #       data array
    puts "Running test at node $id."
    array set data [$itk_component(tree) getData $id]
    set data(result) "In progress"
    $itk_component(tree) setData $id [array get data]

    set testxml $data(xmlfile)
    set driver [makeDriver $testxml]

    set tool [Rappture::Tool ::#auto $driver $_tooldir]
    set result ""
    foreach {status result} [eval $tool run] break
    set data(ran) yes
    if {$status == 0 && [Rappture::library isvalid $result]} {
        set golden [Rappture::library $testxml]
        set diffs [Rappture::Regression::compare $golden $result output]
        if {$diffs != ""} {
            set data(result) fail
            set data(diffs) $diffs
        } else {
            set data(result) pass
        }
    } else {
        set data(result) error
    }
    $itk_component(tree) setData $id [array get data]
    # TODO: Remove runfile
}

# ----------------------------------------------------------------------
# USAGE: makeDriver testxml
#
# Creates and returns a driver Rappture::library object to be used for 
# running the test specified by testxml.  If any input elements are
# present in the new tool.xml which do not exist in the test xml, use
# the default value specified in the new tool.xml.
# ----------------------------------------------------------------------
itcl::body Rappture::Regression::MainWin::makeDriver {testxml} {
    # Construct a driver file.
    # TODO: Pass through all inputs in the new tool.xml.  If present in the
    #       test xml, copy the value.  If not, use the default from the new
    #       tool.xml.
    # For now, just use test xml with the output deleted and command replaced
    set driver [Rappture::library $testxml]
    set cmd [[Rappture::library $_toolxml] get tool.command]
    $driver put tool.command $cmd
    $driver remove output
    return $driver
}
