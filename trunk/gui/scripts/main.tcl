# -*- mode: tcl; indent-tabs-mode: nil -*-
#!/bin/sh
# ----------------------------------------------------------------------
#  USER INTERFACE DRIVER
#
#  This driver program loads a tool description from a tool.xml file,
#  and produces a user interface automatically to drive an application.
#  The user can set up input, click and button to launch a tool, and
#  browse through output.
#
#  RUN AS FOLLOWS:
#    wish main.tcl ?-tool <toolfile>?
#
#  If the <toolfile> is not specified, it defaults to "tool.xml" in
#  the current working directory.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

# take the main window down for now, so we can avoid a flash on the screen
wm withdraw .

package require Itcl
package require Img
package require Rappture
package require RapptureGUI

option add *MainWin.mode desktop startupFile
option add *MainWin.borderWidth 0 startupFile
option add *MainWin.anchor fill startupFile

# "web site" look
option add *MainWin.bgScript {
    rectangle 0 0 200 <h> -outline "" -fill #5880BB
    rectangle 200 0 300 <h> -outline "" -fill #425F8B
    rectangle 300 0 <w> <h> -outline "" -fill #324565
} startupFile

# "clean" look
option add *MainWin.bgScript "" startupFile
option add *MainWin.bgColor white startupFile
option add *Tooltip.background white
option add *Editor.background white
option add *Gauge.textBackground white
option add *TemperatureGauge.textBackground white
option add *Switch.textBackground white
option add *Progress.barColor #ffffcc
option add *Balloon.titleBackground #6666cc
option add *Balloon.titleForeground white
option add *Balloon*Label.font -*-helvetica-medium-r-normal-*-12-*
option add *Balloon*Radiobutton.font -*-helvetica-medium-r-normal-*-12-*
option add *Balloon*Checkbutton.font -*-helvetica-medium-r-normal-*-12-*
option add *ResultSelector.controlbarBackground #6666cc
option add *ResultSelector.controlbarForeground white
option add *ResultSelector.activeControlBackground #ccccff
option add *ResultSelector.activeControlForeground black
option add *Radiodial.length 3i
option add *BugReport*banner*foreground white
option add *BugReport*banner*background #a9a9a9
option add *BugReport*banner*highlightBackground #a9a9a9
option add *BugReport*banner*font -*-helvetica-bold-r-normal-*-18-*
option add *hubcntls*Button.padX 0 widgetDefault
option add *hubcntls*Button.padY 0 widgetDefault
option add *hubcntls*Button.relief flat widgetDefault
option add *hubcntls*Button.overRelief raised widgetDefault
option add *hubcntls*Button.borderWidth 1 widgetDefault
option add *hubcntls*Button.font \
    -*-helvetica-medium-r-normal-*-8-* widgetDefault

switch $tcl_platform(platform) {
    unix - windows {
        event add <<PopupMenu>> <ButtonPress-3>
    }
    macintosh {
        event add <<PopupMenu>> <Control-ButtonPress-1>
    }
}

# install a better bug handler
Rappture::bugreport::install
# fix the "grab" command to support a stack of grab windows
Rappture::grab::init

#
# Process command line args to get the names of files to load...
#
Rappture::getopts argv params {
    value -tool tool.xml
    list  -load ""
    value -input ""
    value -nosim 0
}

set loadobjs {}
foreach runfile $params(-load) {
    if {![file exists $runfile]} {
        puts stderr "can't find run: \"$runfile\""
        exit 1
    }
    set status [catch {Rappture::library $runfile} result]
    lappend loadobjs $result
}

set inputobj {}
if {$params(-input) ne ""} {
    if {![file exists $params(-input)]} {
        puts stderr "can't find input file: \"$params(-input)\""
        exit 1
    }
    if {[catch {Rappture::library $params(-input)} result] == 0} {
        set inputobj $result
    }
}

# open the XML file containing the tool parameters
if {![file exists $params(-tool)]} {
    # check to see if the user specified any run.xml files to load.
    # if so, we can use that as the tool.xml. if we can find where
    # the original application was installed using the xml tag
    # tool.version.application.directory(top), the user can
    # run new simulations, otherwise they can only revisualize the
    # run.xml files they are loading.
    set pseudotool ""
    if {[llength $loadobjs] == 0 && $inputobj eq ""} {
        puts stderr "can't find tool \"$params(-tool)\""
        exit 1
    }
    # search the loadfiles for the install location
    # we could just use run.xml files as tool.xml, but
    # if there are loaders or notes, they will still need
    # examples/ and docs/ dirs from the install location
    set check [concat $loadobjs $inputobj]
    foreach runobj $check {
        set tdir \
            [string trim [$runobj get tool.version.application.directory(tool)]]
        if {[file isdirectory $tdir] && \
            [file exists $tdir/tool.xml]} {
            set pseudotool $tdir/tool.xml
            break
        }
    }
    if {![file exists $pseudotool]} {
        # we didn't find a tool.xml file,
        # use info from a runfile to build gui
        # disable simulation, because no tool.xml
        set pseudotool [lindex $params(-load) 0]
        array set params [list -nosim true]
    }
    if {![file exists $pseudotool]} {
        puts stderr "can't find tool \"$params(-tool)\""
        exit 1
    }
    array set params [list -tool $pseudotool]
}

set xmlobj [Rappture::library $params(-tool)]

set installdir [file normalize [file dirname $params(-tool)]]
$xmlobj put tool.version.application.directory(tool) $installdir

set tool [Rappture::Tool ::#auto $xmlobj $installdir]

# ----------------------------------------------------------------------
# CHECK JOB FAILURE REPORTING
#
# If this tool might fail when it launches jobs (i.e., Rappture
# can't check some inputs, such as strings), then disable the
# automatic ticket submission for job failures
# ----------------------------------------------------------------------
set val [string trim [$tool xml get tool.reportJobFailures]]
if { "" != $val} {
    if {[catch {Rappture::bugreport::shouldReport jobfailures $val} result]} {
        puts stderr "WARNING for reportJobFailures: $result"
    }
}

# ----------------------------------------------------------------------
# LOAD RESOURCE SETTINGS
#
# Try to load the $SESSIONDIR/resources file, which contains
# middleware settings, such as the application name and the
# filexfer settings.
# ----------------------------------------------------------------------
Rappture::resources::load

# ----------------------------------------------------------------------
# START LOGGING
#
# If the $RAPPTURE_LOG directory is set to a directory used for
# logging, then open a log file and start logging.
# ----------------------------------------------------------------------
Rappture::Logger::init

# ----------------------------------------------------------------------
# INITIALIZE THE DESKTOP CONNECTION
#
# If there's a SESSION ID, then this must be running within the
# nanoHUB.  Try to initialize the server handling the desktop
# connection.
# ----------------------------------------------------------------------
Rappture::filexfer::init

# ----------------------------------------------------------------------
# MAIN WINDOW
# ----------------------------------------------------------------------
Rappture::MainWin .main -borderwidth 0
.main configure -title [string trim [$tool xml get tool.title]]
wm withdraw .main
wm protocol .main WM_DELETE_WINDOW {Rappture::Logger::cleanup; exit}

# if the FULLSCREEN variable is set, then nanoHUB wants us to go full screen
if {[info exists env(FULLSCREEN)]} {
    .main configure -mode web
}

#
# The main window has a pager that acts as a notebook for the
# various parts.  This notebook as at least two pages--an input
# page and an output (analysis) page.  If there are <phase>'s
# for input, then there are more pages in the notebook.
#
set win [.main component app]
Rappture::Postern $win.postern
pack $win.postern -side bottom -fill x

Rappture::Pager $win.pager
pack $win.pager -expand yes -fill both

#
# Add a place for about/questions in the breadcrumbs area of this pager.
#
set app [string trim [$tool xml get tool.id]]
set url [Rappture::Tool::resources -huburl]
if {"" != $url && "" != $app} {
    set f [$win.pager component breadcrumbarea]
    frame $f.hubcntls
    pack $f.hubcntls -side right -padx 4
    label $f.hubcntls.icon -image [Rappture::icon ask] -highlightthickness 0
    pack $f.hubcntls.icon -side left
    button $f.hubcntls.about -text "About this tool" -command "
        [list Rappture::filexfer::webpage $url/tools/$app]
        Rappture::Logger::log help about
    "
    pack $f.hubcntls.about -side top -anchor w
    button $f.hubcntls.questions -text Questions? -command "
        [list Rappture::filexfer::webpage $url/resources/$app/questions]
        Rappture::Logger::log help questions
    "
    pack $f.hubcntls.questions -side top -anchor w
}

#
# Load up the components in the various phases of input.
#
set phases [$tool xml children -type phase input]
if {[llength $phases] > 0} {
    set plist ""
    foreach name $phases {
        lappend plist input.$name
    }
    set phases $plist
} else {
    set phases input
}

foreach comp $phases {
    set title [string trim [$tool xml get $comp.about.label]]
    if {$title == ""} {
        set title "Input #auto"
    }
    $win.pager insert end -name $comp -title $title

    #
    # Build the page of input controls for this phase.
    #
    set f [$win.pager page $comp]
    Rappture::Page $f.cntls $tool $comp
    pack $f.cntls -expand yes -fill both
}

# let components (loaders) in the newly created pages settle
update

# ----------------------------------------------------------------------
# OUTPUT AREA
# ----------------------------------------------------------------------

# adjust the title of the page here.
# to adjust the button text, look in analyzer.tcl
set simtxt [string trim [$xmlobj get tool.action.label]]
if {"" == $simtxt} {
    set simtxt "Simulate"
}
$win.pager insert end -name analyzer -title $simtxt
set f [$win.pager page analyzer]
$win.pager page analyzer -command [subst {
    if { !$params(-nosim) } {
        $win.pager busy yes
        update
        $f.analyze simulate -ifneeded
        $win.pager busy no
    }
}]

Rappture::Analyzer $f.analyze $tool -simcontrol auto -notebookpage about
pack $f.analyze -expand yes -fill both

$tool notify add analyzer * [list $f.analyze reset]

# ----------------------------------------------------------------------
# Finalize the arrangement
# ----------------------------------------------------------------------
if {[llength [$win.pager page]] > 2} {
    # We have phases, so we shouldn't allow the "Simulate" button.
    # If it pops up, there are two ways to push simulate and duplicate
    # links for "About" and "Questions?".
    $f.analyze configure -simcontrol off
} elseif {[llength [$win.pager page]] == 2} {
    set style [string trim [$xmlobj get tool.layout]]
    set screenw [winfo screenwidth .]

    update idletasks
    set w0 [winfo reqwidth [$win.pager page @0]]
    set w1 [winfo reqwidth [$win.pager page @1]]

    if { $style != "wizard" } {
        # If only two windows and they're small enough, put them up
        # side-by-side
        if {$w0+$w1 < $screenw } {
            $win.pager configure -arrangement side-by-side
            $f.analyze configure -holdwindow [$win.pager page @0]
        }
    }
    set type [string trim [$tool xml get tool.control]]
    if {$type == ""} {
        set type [string trim [$tool xml get tool.control.type]]
    }
    set arrangement [$win.pager cget -arrangement]
    if { $type == "" } {
        if { $arrangement != "side-by-side" } {
           set type auto
        }
    }
    if { $arrangement != "side-by-side" &&
            ($type == "manual" || $type == "manual-resim" ||
             $type == "auto" || $style == "wizard") } {
        # in "auto" mode, we don't need a simulate button
        $f.analyze configure -simcontrol off
    } else {
        # not in "auto" mode but side-by-side, we always need the button
        $f.analyze configure -simcontrol on
    }
}

# load previous xml runfiles
if {[llength $params(-load)] > 0} {
    foreach runobj $loadobjs {
        $f.analyze load $runobj
    }
    # load the inputs for the very last run
    $tool load $runobj

    # don't need simulate button if we cannot simulate
    if {$params(-nosim)} {
        $f.analyze configure -simcontrol off
    }
    $f.analyze configure -notebookpage analyze
    $win.pager current analyzer
} elseif {$params(-input) ne ""} {
    $tool load $inputobj
}

# let components (loaders) settle after the newly loaded runs
update

foreach path [array names ::Rappture::parameters] {
    set fname $::Rappture::parameters($path)
    if {[catch {
          set fid [open $fname r]
          set info [read $fid]
          close $fid}] == 0} {

        set w [$tool widgetfor $path]
        if {$w ne ""} {
            if {[catch {$w value [string trim $info]} result]} {
                puts stderr "WARNING: bad tool parameter value for \"$path\""
                puts stderr "  $result"
            }
        } else {
            puts stderr "WARNING: can't find control for tool parameter: $path"
        }
    }
}

wm deiconify .main
