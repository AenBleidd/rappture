# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: analyzer - output area for Rappture
#
#  This widget acts as the output side of a Rappture application.
#  When the input has changed, it displays a Simulate button that
#  launches the simulation.  When a simulation is running, this
#  area shows status.  When it is finished, the results appear
#  in place of the button, according to the specs in the <analyze>
#  XML data.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Analyzer.width 3.5i widgetDefault
option add *Analyzer.height 4i widgetDefault
option add *Analyzer.simControl "auto" widgetDefault
option add *Analyzer.simControlBackground "" widgetDefault
option add *Analyzer.simControlOutline gray widgetDefault
option add *Analyzer.simControlActiveBackground #ffffcc widgetDefault
option add *Analyzer.simControlActiveOutline black widgetDefault
option add *Analyzer.notebookpage "about" widgetDefault

option add *Analyzer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *Analyzer.codeFont \
    -*-courier-medium-r-normal-*-12-* widgetDefault
option add *Analyzer.textFont \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *Analyzer.boldTextFont \
    -*-helvetica-bold-r-normal-*-12-* widgetDefault

itcl::class Rappture::Analyzer {
    inherit itk::Widget

    itk_option define -codefont codeFont Font ""
    itk_option define -textfont textFont Font ""
    itk_option define -boldtextfont boldTextFont Font ""
    itk_option define -simcontrol simControl SimControl ""
    itk_option define -simcontroloutline simControlOutline Background ""
    itk_option define -simcontrolbackground simControlBackground Background ""
    itk_option define -simcontrolactiveoutline simControlActiveOutline Background ""
    itk_option define -simcontrolactivebackground simControlActiveBackground Background ""
    itk_option define -holdwindow holdWindow HoldWindow ""
    itk_option define -notebookpage notebookPage NotebookPage ""

    constructor {tool args} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method simulate {args}
    public method reset {{when -eventually}}
    public method load {xmlobj}
    public method reload { fileName }
    public method clear {{xmlobj "all"}}
    public method download {option args}
    public method buildMenu { w url appName }

    protected method _plot {args}
    protected method _reorder {comps}
    protected method _autoLabel {xmlobj path title cntVar}
    protected method _fixResult {}
    protected method _fixResultSet {args}
    protected method _fixSize {}
    protected method _fixSimControl {}
    protected method _fixNotebook {}
    protected method _simState {state args}
    protected method _simOutput {message}
    protected method _resultTooltip {}
    protected method _isPdbTrajectory {data}
    protected method _isLammpsTrajectory {data}
    protected method _pdbToSequence {xmlobj path id child data}
    protected method _lammpsToSequence {xmlobj path id child data}
    protected method _trajToSequence {xmlobj {path ""}}
    protected method _pop_uq_dialog {win}
    protected method _setWaitVariable {state}
    protected method _adjust_level {win}

    private variable _done 0
    private variable _tree ""
    private variable _saved 
    private variable _name "" 
    private variable _revision 0;       # Tool revision
    private variable _tool ""          ;# belongs to this tool
    private variable _appName ""       ;# Name of application
    private variable _control "manual" ;# start mode
    private variable _resultset ""     ;# ResultSet object with all results
    private variable _pages 0          ;# number of pages for result sets
    private variable _label2page       ;# maps output label => result set
    private variable _label2desc       ;# maps output label => description
    private variable _label2item       ;# maps output label => output.xxx item
    private variable _lastlabel ""     ;# label of last example loaded
    private variable _plotlist ""      ;# items currently being plotted
    private variable _lastPlot
    private variable _uq_active 0      ;# a UQ variables has been used
    private variable _wait_uq 0
    private common job                 ;# array var used for blt::bgexec jobs

    private method BuildQuestionDialog { popup }
    private method BuildSimulationTable { w }
    private method Cancel {}
    private method CheckSimsetDetails {}
    private method CleanName { name }
    private method CreateSharedPath { path } 
    private method EditSimset {}
    private method ExportFile { src msgFile }
    private method ExportURL { appName installdir } 
    private method FindSimsetsForApp { appName }
    private method GetSimset { name }
    private method InstallSharedFile { installdir file } 
    private method LoadSimulations { runfiles }
    private method Ok {}
    private method OverwriteSaveFile {}
    private method PostMenu { menu }
    private method ReadSimsetFile { fileName }
    private method Save {}
    private method SaveSimulations {}
    private method SelectSimsetForDeletion {}
    private method SelectSimsetForLoading {}
    private method SelectSimsetNameAndSave {}
    private method SelectSimsetToShare {}
    private method WriteSimsetFile { appName fileName share }
}

itk::usual Analyzer {
    keep -background -cursor foreground -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::constructor {tool args} {
    set _tool $tool
    set _tree [blt::tree create]

    # use this to store all simulation results
    set _resultset [Rappture::ResultSet ::\#auto]
    $_resultset notify add $this [itcl::code $this _fixResultSet]

    # widget settings...
    itk_option add hull.width hull.height
    pack propagate $itk_component(hull) no

    frame $itk_interior.simol -borderwidth 1 -relief flat
    pack $itk_interior.simol -fill x

    itk_component add simbg {
        frame $itk_interior.simol.simbg -borderwidth 0
    } {
        usual
        rename -background -simcontrolcolor simControlColor Color
    }
    pack $itk_component(simbg) -expand yes -fill both

    set simtxt [string trim [$tool xml get tool.action.label]]
    if {"" == $simtxt} {
        set simtxt "Simulate"
    }
    itk_component add simulate {
        button $itk_component(simbg).simulate -text $simtxt \
            -command [itcl::code $this simulate]
    } {
        usual
        rename -highlightbackground -simcontrolcolor simControlColor Color
    }
    pack $itk_component(simulate) -side left -padx 4 -pady 4

    # BE CAREFUL: Shift focus when we click on "Simulate".
    #   This shifts focus away from input widgets and causes them to
    #   finalize and log any pending changes.  A better way would be
    #   to have the "sync" operation finalize/sync, but this works
    #   for now.
    bind $itk_component(simulate) <ButtonPress> {focus %W}

    # if there's a hub url, then add "About" and "Questions" links
    global rapptureInfo
    if { $rapptureInfo(appName) == "" } {
        set _appName [$_tool xml get "tool.id"]
    } else {
        set _appName $rapptureInfo(appName)
    }
    set num [$_tool xml get "tool.version.application.revision"]
    if { $num == "" } {
        set num 0
    }
    set _revision $num
    set url [Rappture::Tool::resources -huburl]

    itk_component add hubcntls {
        frame $itk_component(simbg).hubcntls
    } {
        usual
        rename -background -simcontrolcolor simControlColor Color
    }
    pack $itk_component(hubcntls) -side right -padx 4
    
    itk_component add help {
        menubutton $itk_component(hubcntls).help \
            -image [Rappture::icon hamburger_menu] \
            -highlightthickness 0 \
            -menu $itk_component(hubcntls).help.menu
    } {
        usual
        ignore -highlightthickness
        rename -background -simcontrolcolor simControlColor Color
    }
    pack $itk_component(help) -side left
    buildMenu $itk_component(help).menu $url $_appName
    
    itk_component add simstatus {
        text $itk_component(simbg).simstatus -borderwidth 0 \
            -highlightthickness 0 -height 1 -width 1 -wrap none \
            -state disabled
    } {
        usual
        ignore -highlightthickness
        rename -background -simcontrolcolor simControlColor Color
        rename -font -textfont textFont Font
    }
    pack $itk_component(simstatus) -side left -expand yes -fill x

    $itk_component(simstatus) tag configure popup \
        -underline 1 -foreground blue

    $itk_component(simstatus) tag bind popup \
        <Enter> {%W configure -cursor center_ptr}
    $itk_component(simstatus) tag bind popup \
        <Leave> {%W configure -cursor ""}
    $itk_component(simstatus) tag bind popup \
        <ButtonPress> {after idle {Rappture::Tooltip::tooltip show %W}}


    itk_component add notebook {
        Rappture::Notebook $itk_interior.nb
    }
    pack $itk_interior.nb -expand yes -fill both

    # ------------------------------------------------------------------
    # ABOUT PAGE
    # ------------------------------------------------------------------
    set w [$itk_component(notebook) insert end about]

    Rappture::Scroller $w.info -xscrollmode off -yscrollmode auto
    pack $w.info -expand yes -fill both -padx 4 -pady 20
    itk_component add toolinfo {
        text $w.info.text -width 1 -height 1 -wrap word \
            -borderwidth 0 -highlightthickness 0
    } {
        usual
        ignore -borderwidth -relief
        rename -font -textfont textFont Font
    }
    $w.info contents $w.info.text

    # ------------------------------------------------------------------
    # SIMULATION PAGE
    # ------------------------------------------------------------------
    set w [$itk_component(notebook) insert end simulate]
    frame $w.cntls
    pack $w.cntls -side bottom -fill x -pady 12
    frame $w.cntls.sep -background black -height 1
    pack $w.cntls.sep -side top -fill x

    itk_component add abort {
        button $w.cntls.abort -text "Abort" \
            -command [itcl::code $_tool abort]
    }
    pack $itk_component(abort) -side left -expand yes -padx 4 -pady 4

    Rappture::Scroller $w.info -xscrollmode auto -yscrollmode auto
    pack $w.info -expand yes -fill both -padx 4 -pady 4
    itk_component add runinfo {
        text $w.info.text -width 1 -height 1 -wrap none \
            -borderwidth 0 -highlightthickness 0 \
            -state disabled
    } {
        usual
        ignore -borderwidth -relief
        rename -font -codefont codeFont Font
    }
    $w.info contents $w.info.text

    itk_component add progress {
        Rappture::Progress $w.progress
    }

    # ------------------------------------------------------------------
    # ANALYZE PAGE
    # ------------------------------------------------------------------
    set w [$itk_component(notebook) insert end analyze]

    frame $w.top
    pack $w.top -side top -fill x -pady 8
    label $w.top.l -text "Result:" -font $itk_option(-font)
    pack $w.top.l -side left

    itk_component add viewselector {
        Rappture::Combobox $w.top.sel -width 10 -editable no
    } {
        usual
        rename -font -textfont textFont Font
    }
    pack $itk_component(viewselector) -side left -expand yes -fill x
    bind $itk_component(viewselector) <<Value>> [itcl::code $this _fixResult]
    bind $itk_component(viewselector) <Enter> \
        [itcl::code $this download coming]

    Rappture::Tooltip::for $itk_component(viewselector) \
        "@[itcl::code $this _resultTooltip]"

    $itk_component(viewselector) choices insert end \
        --- "---"

    itk_component add download {
        button $w.top.dl -image [Rappture::icon download] -anchor e \
            -borderwidth 1 -relief flat -overrelief raised \
            -command [itcl::code $this download start $w.top.dl]
    }
    pack $itk_component(download) -side right -padx {4 0}
    bind $itk_component(download) <Enter> \
        [itcl::code $this download coming]

    $itk_component(viewselector) choices insert end \
        @download [Rappture::filexfer::label download]

    if {[Rappture::filexfer::enabled]} {
        Rappture::Tooltip::for $itk_component(download) "Downloads the current result to a new web browser window on your desktop.  From there, you can easily print or save results.

NOTE:  Your web browser must allow pop-ups from this site.  If your output does not appear, look for a 'pop-up blocked' message and enable pop-ups."
    } else {
        Rappture::Tooltip::for $itk_component(download) "Saves the current result to a file on your desktop."
    }

    itk_component add results {
        Rappture::Panes $w.pane \
            -sashwidth 2 -sashrelief solid -sashpadding {2 0}
    } {
        usual
        ignore -sashwidth -sashrelief -sashpadding
    }
    pack $itk_component(results) -expand yes -fill both
    set f [$itk_component(results) pane 0]

    itk_component add resultpages {
        Rappture::Notebook $f.nb
    }
    pack $itk_component(resultpages) -expand yes -fill both
    set f [$itk_component(results) insert end -fraction 0.1]
    itk_component add resultselector {
        Rappture::ResultSelector $f.rsel -resultset $_resultset \
            -settingscommand [itcl::code $this _plot]
    }
    pack $itk_component(resultselector) -expand yes -fill both
    bind $itk_component(resultselector) <<Layout>> [itcl::code $this _fixSize]
    bind $itk_component(results) <Configure> [itcl::code $this _fixSize]

    eval itk_initialize $args

    $itk_component(runinfo) tag configure ERROR -foreground red
    $itk_component(runinfo) tag configure text -font $itk_option(-textfont)

    #
    # Load up tool info on the first page.
    #
    $itk_component(toolinfo) tag configure title \
        -font $itk_option(-boldtextfont)

    set mesg [string trim [$tool xml get tool.title]]
    if {"" != $mesg} {
        $itk_component(toolinfo) insert end $mesg title
        $itk_component(toolinfo) insert end "\n\n"
    }

    set mesg [string trim [$tool xml get tool.about]]
    if {"" != $mesg} {
        $itk_component(toolinfo) insert end $mesg
    }
    $itk_component(toolinfo) configure -state disabled
    $itk_component(notebook) current about

    # tool can run on "manual" (default) or "auto"
    set cntl [string trim [$tool xml get tool.control]]
    if {"" == $cntl} {
        set cntl [string trim [$tool xml get tool.control.type]]
    }
    if {"" != $cntl} {
        set _control $cntl
    }

    # reset everything to a clean state
    reset
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::destructor {} {
    after cancel [itcl::code $this simulate]
    $_resultset notify remove $this
    itcl::delete object $_resultset
}

# ----------------------------------------------------------------------
# USAGE: simulate ?-ifneeded?
# USAGE: simulate ?<path1> <value1> <path2> <value2> ...?
#
# Kicks off the simulator by executing the tool.command associated
# with the tool.  If any arguments are specified, they are used to
# set parameters for the simulation.  While the simulation is running,
# it shows status.  When the simulation is finished, it switches
# automatically to "analyze" mode and shows the results.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::simulate {args} {

    set uq [$_tool get_uq -uq_type smolyak -uq_args 2]

    # pop up UQ window
    if {[$uq num_runs] > 1} {
        set _uq_active 1
        set status [$uq run_dialog $itk_component(simulate)]
        if {$status == 0} {
            # cancelled
            return
        }
        lappend args -uq_type [$uq type]
        lappend args -uq_args [$uq args]
        # Need to put these UQ values into the driver file
        # so the call to resultset::contains will be correct.
        set _xml [$_tool xml object]
        $_xml put uq.type.current [$uq type]
        $_xml put uq.args.current [$uq args]
        $_xml put uq.args.about.label "level"
        $_xml put uq.args.about.description "Polynomial Degree of Smolyak GPC method."
    }
    #puts "simulate args=$args"

    if {[lindex $args 0] == "-ifneeded"} {
        # check to see if simulation is really needed
        $_tool sync
        if {[$_resultset contains [$_tool xml object]] &&
            ![string equal $_control "manual-resim"]} {
            # not needed -- show results and return
            $itk_component(notebook) current analyze
            return
        }
        set args [lreplace $args 0 0]
    }

    # simulation is needed -- go to simulation page
    $itk_component(notebook) current simulate

    # no progress messages yet
    pack forget $itk_component(progress)
    lappend args -output [itcl::code $this _simOutput]

    _simState off
    $itk_component(runinfo) configure -state normal
    $itk_component(runinfo) delete 1.0 end
    $itk_component(runinfo) insert end "Running simulation...\n\n" text
    $itk_component(runinfo) configure -state disabled

    # if the hold window is set, then put up a busy cursor
    if {$itk_option(-holdwindow) != ""} {
        blt::busy hold $itk_option(-holdwindow)
        raise $itk_component(hull)
    }

    # execute the job
    #puts "$_tool run $args"

    foreach {status result} [eval $_tool run $args] break

    # if job was aborted, then allow simulation again
    if {$result == "ABORT"} {
        _simState on "Aborted"
    }

    # load results from run.xml into analyzer
    if {$status == 0 && $result != "ABORT"} {
        set status [catch {load $result} result]
    }

    # back to normal
    if {$itk_option(-holdwindow) != ""} {
        blt::busy release $itk_option(-holdwindow)
    }
    $itk_component(abort) configure -state disabled

    if {$status != 0} {
        $itk_component(runinfo) configure -state normal
        # Don't erase program error messages.
        # $itk_component(runinfo) delete 1.0 end
        $itk_component(runinfo) insert end "\n\nProblem launching job:\n\n" text
        _simOutput $result
        $itk_component(runinfo) configure -state disabled
        $itk_component(runinfo) see 1.0

        # Try to create a support ticket for this error.
        # It may be a real problem.
        if {[Rappture::bugreport::shouldReport for jobs]} {
            set ::errorInfo "\n\n== RAPPTURE INPUT ==\n[$_tool xml xml]"
            Rappture::bugreport::register "Problem launching job:\n$result"
            Rappture::bugreport::attachment [$_tool xml xml]
            Rappture::bugreport::send
        }
    } else {
        $itk_component(notebook) current analyze
    }

    # do this last -- after _simOutput above
    pack forget $itk_component(progress)
}


# ----------------------------------------------------------------------
# USAGE: reset ?-eventually|-now?
#
# Used to reset the analyzer whenever the input to a simulation has
# changed.  Sets the mode back to "simulate", so the user has to
# simulate again to see the output.  If the <start> option is set
# to "auto", the simulation is invoked immediately.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::reset {{when -eventually}} {
    if {$when == "-eventually"} {
        after cancel [list catch [itcl::code $this reset -now]]
        after idle [list catch [itcl::code $this reset -now]]
        return
    }

    # check to see if simulation is really needed
    $_tool sync
    if {![$_resultset contains [$_tool xml object]] ||
        [string equal $_control "manual-resim"]} {
        # if control mode is "auto", then simulate right away
        if {[string match auto* $_control]} {
            # auto control -- don't need button
            pack forget $itk_interior.simol

            after cancel [itcl::code $this simulate]
            after idle [itcl::code $this simulate]
        } else {
            _simState on "new input parameters"
        }
    } else {
        _simState off
    }
}

# ----------------------------------------------------------------------
# USAGE: load <xmlobj>
#
# Loads the data from the given <xmlobj> into the appropriate results
# sets.  If necessary, new results sets are created to store the data.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::load {xmlobj} {
    # only show the last result? then clear first
    if {[string trim [$_tool xml get tool.analyzer]] == "last"} {
        clear
    }
    #puts "Analyzer::load"
    $_resultset add $xmlobj

    # NOTE: Adding will trigger a !change event on the ResultSet
    # object, which will trigger calls to _fixResultSet to add
    # the results to display.
}

# ----------------------------------------------------------------------
# USAGE: clear ?<xmlobj>?
#
# Discards one or more results previously loaded into the analyzer.
# If an <xmlobj> is specified, then that one result is cleared.
# Otherwise, all results are cleared.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::clear {{xmlobj "all"}} {
    if {$xmlobj eq "" || $xmlobj eq "all"} {
        $_resultset clear
    } else {
        $_resultset clear $xmlobj
    }

    # NOTE: Clearing will trigger a !change event on the ResultSet
    # object, which will trigger calls to _fixResultSet to clean up
    # the results being displayed.
}

# ----------------------------------------------------------------------
# USAGE: download coming
# USAGE: download controls <downloadCommand>
# USAGE: download start ?widget?
# USAGE: download now ?widget?
#
# Spools the current result so the user can download it.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::download {option args} {
    set title [$itk_component(viewselector) value]
    set page [$itk_component(viewselector) translate $title]

    switch -- $option {
        coming {
            #
            # Warn result that a download is coming, in case
            # it needs to take a screen snap.
            #
            if {![regexp {^(|@download|---)$} $page]} {
                set f [$itk_component(resultpages) page $page]
                $f.rviewer download coming
            }
        }
        controls {
            # no controls for this download yet
            return ""
        }
        start {
            set widget $itk_component(download)
            if {[llength $args] > 0} {
                set widget [lindex $args 0]
                if {[catch {winfo class $widget}]} {
                    set widget $itk_component(download)
                }
            }
            #
            # See if this download has any controls.  If so, then
            # post them now and let the user continue the download
            # after selecting a file format.
            #
            if {$page != ""} {
                set ext ""
                set f [$itk_component(resultpages) page $page]
                set arg [itcl::code $this download now $widget]
                set popup [$f.rviewer download controls $arg]
                if {"" != $popup} {
                    $popup activate $widget below
                } else {
                    download now $widget
                }
            } else {
                # this shouldn't happen
                set file error.html
                set data "<h1>Not Found</h1>There is no result selected."
            }
        }
        now {
            set widget $itk_component(download)
            if {[llength $args] > 0} {
                set widget [lindex $args 0]
                if {[catch {winfo class $widget}]} {
                    set widget $itk_component(download)
                }
            }
            #
            # Perform the actual download.
            #
            if {$page != ""} {
                set ext ""
                set f [$itk_component(resultpages) page $page]
                set item [$itk_component(viewselector) value]
                set result [$f.rviewer download now $widget $_appName $item]
                if { $result == "" } {
                    return;                # User cancelled the download.
                }
                foreach {ext data} $result break
                if {"" == $ext} {
                    if {"" != $widget} {
                        Rappture::Tooltip::cue $widget \
                            "Can't download this result."
                    }
                    return
                }
                regsub -all {[\ -\/\:-\@\{-\~]} $title {} title
                set file "$title$ext"
            } else {
                # this shouldn't happen
                set file error.html
                set data "<h1>Not Found</h1>There is no result selected."
            }

            Rappture::Logger::log download [$itk_component(viewselector) value]
            set mesg [Rappture::filexfer::download $data $file]
            if {[string length $mesg] > 0} {
                Rappture::Tooltip::cue $widget $mesg
            }
        }
        default {
            error "bad option \"$option\": should be coming, controls, now, start"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _plot ?<index> <options> <index> <options>...?
#
# Used internally to update the plot shown in the current result
# viewer whenever the resultselector settings have changed.  Causes the
# desired results to show up on screen.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_plot {args} {
    #puts "analyzer::_plot"
    set _plotlist $args

    set page [$itk_component(viewselector) value]
    set page [$itk_component(viewselector) translate $page]
    if {"" != $page} {
        set f [$itk_component(resultpages) page $page]
        $f.rviewer plot clear
        foreach {index opts} $_plotlist {
            $f.rviewer plot add $index $opts
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _reorder <compList>
#
# Used internally to change the order of a series of output components
# found in the <output> section.  Moves the <log> elements to the end
# and returns the updated list.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_reorder {comps} {
    set i 0
    set max [llength $comps]
    while {$i < $max} {
        set c [lindex $comps $i]
        if {[string match log* $c]} {
            set comps [lreplace $comps $i $i]
            lappend comps $c
            incr max -1
        } else {
            incr i
        }
    }
    return $comps
}

# ----------------------------------------------------------------------
# USAGE: _autoLabel <xmlobj> <path> <title> <cntVar>
#
# Used internally to check for an about.label property at the <path>
# in <xmlobj>.  If this object doesn't have a label, then one is
# supplied using the given <title>.  The <cntVar> is an array of
# counters in the calling scopes for titles that have been used
# in the past.  This is used to create titles like "Plot #2" the
# second time it is encountered.
#
# The <xmlobj> is updated so that the label is inserted directly in
# the tree.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_autoLabel {xmlobj path title cntVar} {
    upvar $cntVar counters

    set group [$xmlobj get $path.about.group]
    set label [$xmlobj get $path.about.label]
    if {"" == $label} {
        # no label -- make one up using the title specified
        if {![info exists counters($group-$title)]} {
            set counters($group-$title) 1
            set label $title
        } else {
            set label "$title (#[incr counters($group-$title)])"
        }
        $xmlobj put $path.about.label $label
    } else {
        # handle the case of two identical labels in <output>
        if {![info exists counters($group-$label)]} {
            set counters($group-$label) 1
        } else {
            set label "$label (#[incr counters($group-$label)])"
            $xmlobj put $path.about.label $label
        }
    }
    return $label
}

# ----------------------------------------------------------------------
# USAGE: _fixResult
#
# Used internally to change the result page being displayed whenever
# the user selects a page from the results combobox.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_fixResult {} {
    set name [$itk_component(viewselector) value]
    set page ""
    if {"" != $name} {
        set page [$itk_component(viewselector) translate $name]
    }
    if {$page == "@download"} {
        # put the combobox back to its last value
        $itk_component(viewselector) component entry configure -state normal
        $itk_component(viewselector) component entry delete 0 end
        $itk_component(viewselector) component entry insert end $_lastlabel
        $itk_component(viewselector) component entry configure -state disabled
        # perform the actual download
        download start $itk_component(download)
    } elseif {$page == "---"} {
        # put the combobox back to its last value
        $itk_component(viewselector) component entry configure -state normal
        $itk_component(viewselector) component entry delete 0 end
        $itk_component(viewselector) component entry insert end $_lastlabel
        $itk_component(viewselector) component entry configure -state disabled
    } elseif {$page != ""} {
        set _lastlabel $name
        $itk_component(resultpages) current $page
        set f [$itk_component(resultpages) page $page]
        # We don't want to replot if we're using an existing viewer with the
        # the same list of objects to plot.  So track the viewer and the list.
        if { ![info exists _lastPlot($f)] || $_plotlist != $_lastPlot($f) } {
            set _lastPlot($f) $_plotlist
            set win [winfo toplevel $itk_component(hull)]
            blt::busy hold $win
            #puts "rviewer = $f.rviewer"
            #puts "_plotlist = $_plotlist"
            $f.rviewer plot clear
            eval $f.rviewer plot add $_plotlist
            blt::busy release $win
        }
        Rappture::Logger::log output $_label2item($name)
        Rappture::Tooltip::for $itk_component(viewselector) \
            "@[itcl::code $this _resultTooltip]" -log $_label2item($name)
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixResultSet ?<eventData>...?
#
# Used internally to react to changes within the ResultSet.  When a
# result is added, a new result viewer is created for the object.
# When all results are cleared, the viewers are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_fixResultSet {args} {
    #puts "Analyzer::_fixResultSet $args"
    array set eventData $args
    switch -- $eventData(op) {
        add {
            set xmlobj $eventData(what)

            # Detect molecule elements that contain trajectory data
            # and convert to sequences.
            _trajToSequence $xmlobj output

            # Go through the analysis and find all result sets.
            set haveresults 0
            foreach item [_reorder [$xmlobj children output]] {
                if {[$xmlobj get output.$item.about.uqtype] == ""} {
                    switch -glob -- $item {
                        log* {
                            _autoLabel $xmlobj output.$item "Output Log" counters
                        }
                        number* {
                            _autoLabel $xmlobj output.$item "Number" counters
                        }
                        integer* {
                            _autoLabel $xmlobj output.$item "Integer" counters
                        }
                        mesh* {
                            _autoLabel $xmlobj output.$item "Mesh" counters
                        }
                        string* {
                            _autoLabel $xmlobj output.$item "String" counters
                        }
                        histogram* - curve* - field* {
                            _autoLabel $xmlobj output.$item "Plot" counters
                        }
                        map* {
                            _autoLabel $xmlobj output.$item "Map" counters
                        }
                        drawing* {
                            _autoLabel $xmlobj output.$item "Drawing" counters
                        }
                        structure* {
                            _autoLabel $xmlobj output.$item "Structure" counters
                        }
                        table* {
                            _autoLabel $xmlobj output.$item "Energy Levels" counters
                        }
                        sequence* {
                            _autoLabel $xmlobj output.$item "Sequence" counters
                        }
                    }
                }
                set label [$xmlobj get output.$item.about.group]
                if {"" == $label} {
                    set label [$xmlobj get output.$item.about.label]
                }

                set hidden [$xmlobj get output.$item.hide]
                set hidden [expr {"" != $hidden && $hidden}]

                if {"" != $label && !$hidden} {
                    set haveresults 1
                }
            }

            # if there are any valid results, add them to the resultset
            if {$haveresults} {
                set index [$_resultset get simnum $xmlobj]

                # add each result to a result viewer
                foreach item [_reorder [$xmlobj children output]] {
                    set label [$xmlobj get output.$item.about.group]
                    if {"" == $label} {
                        set label [$xmlobj get output.$item.about.label]
                    }
                    set hidden [$xmlobj get output.$item.hide]
                    if {$hidden == ""} {
                        set hidden 0
                    }
                    if {"" != $label && !$hidden} {
                        set uq_part [$xmlobj get output.$item.about.uqtype]

                        #puts "label=$label uq_part=$uq_part"

                        if {![info exists _label2page($label)]} {
                            #puts "Adding label: '$label'"
                            set name "page[incr _pages]"
                            #puts "Inserting $name into resultpages"
                            set page [$itk_component(resultpages) \
                                insert end $name]
                            set _label2page($label) $page
                            set _label2item($label) output.$item
                            set _label2desc($label) \
                                [$xmlobj get output.$item.about.description]
                            Rappture::ResultViewer $page.rviewer
                            pack $page.rviewer -expand yes -fill both -pady 4

                            set end [$itk_component(viewselector) \
                                choices index -value ---]
                            if {$end < 0} {
                                set end "end"
                            }
                            $itk_component(viewselector) choices insert $end \
                                $name $label
                        }

                        # add/replace the latest result into this viewer
                        set page $_label2page($label)

                        if {![info exists reset($page)]} {
                            $page.rviewer clear $index
                            set reset($page) 1
                        }
                        $page.rviewer add $index $xmlobj output.$item $label $uq_part
                    }
                }
            }

            # show the first page by default
            set max [$itk_component(viewselector) choices size]
            for {set i 0} {$i < $max} {incr i} {
                set first [$itk_component(viewselector) choices get -label $i]
                if {$first != ""} {
                    set page [$itk_component(viewselector) choices get -value $i]
                    set char [string index $page 0]
                    if {$char != "@" && $char != "-"} {
                        $itk_component(resultpages) current $page
                        $itk_component(viewselector) value $first
                        set _lastlabel $first
                        break
                    }
                }
            }
        }
        clear {
            if {$eventData(what) ne "all"} {
                # delete this result from all viewers
                array set params $eventData(what)
                foreach label [array names _label2page] {
                    set page $_label2page($label)
                    $page.rviewer clear $params(simnum)
                }
            }

            if {[$_resultset size] == 0} {
                # reset the size of the controls area
                set ht [winfo height $itk_component(results)]
                set cntlht [$itk_component(resultselector) size -controlarea]
                set frac [expr {double($cntlht)/$ht}]
                $itk_component(results) fraction end $frac

                foreach label [array names _label2page] {
                    set page $_label2page($label)
                    destroy $page.rviewer
                }
                $itk_component(resultpages) delete -all
                set _pages 0

                $itk_component(viewselector) value ""
                $itk_component(viewselector) choices delete 0 end
                catch {unset _label2page}
                catch {unset _label2item}
                catch {unset _label2desc}
                set _plotlist ""

                $itk_component(viewselector) choices insert end --- "---"
                $itk_component(viewselector) choices insert end \
                    @download [Rappture::filexfer::label download]
                set _lastlabel ""
            }

            # fix Simulate button state
            reset
        }
        default {
            error "don't know how to handle op \"$eventData(op)\""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixSize
#
# Used internally to change the size of the result set area whenever
# a new control appears.  Adjusts the size available for the result
# set up to some maximum.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_fixSize {} {
    set ht [winfo height $itk_component(results)]
    if {$ht <= 1} { set ht [winfo reqheight $itk_component(results)] }
    set cntlht [$itk_component(resultselector) size -controlarea]
    set frac [expr {double($cntlht)/$ht}]

    if {$frac < 0.4} {
        $itk_component(results) fraction end $frac
    }
    _fixSimControl
}

# ----------------------------------------------------------------------
# USAGE: _simState <boolean> ?<message>? ?<settings>?
#
# Used internally to change the "Simulation" button on or off.
# If the <boolean> is on, then any <message> and <settings> are
# displayed as well.  If the <boolean> is off, then only display
# the message. The <message> is a note to the user about
# what will be simulated, and the <settings> are a list of
# tool parameter settings of the form {path1 val1 path2 val2 ...}.
# When these are in place, the next Simulate operation will use
# these settings.  This helps fill in missing data values.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_simState {state args} {
    if {$state} {
        $itk_interior.simol configure \
            -background $itk_option(-simcontrolactiveoutline)
        configure -simcontrolcolor $itk_option(-simcontrolactivebackground)

        $itk_component(abort) configure -state disabled
        $itk_component(simulate) configure -state normal \
            -command [itcl::code $this simulate]

        #
        # If there's a special message, then put it up next to the button.
        #
        set mesg [lindex $args 0]
        if {"" != $mesg} {
            $itk_component(simstatus) configure -state normal
            $itk_component(simstatus) delete 1.0 end
            $itk_component(simstatus) insert end $mesg

            #
            # If there are any settings, then install them in the
            # "Simulate" button.  Also, pop them up as a tooltip
            # for the message.
            #
            set settings [lindex $args 1]
            if {[llength $settings] > 0} {
                $itk_component(simulate) configure \
                    -command [eval itcl::code $this simulate $settings]

                set details ""
                foreach {path val} $settings {
                    set str [string trim [$_tool xml get $path.about.label]]
                    if {"" == $str} {
                        set str [$_tool xml element -as id $path]
                    }
                    append details "$str = $val\n"
                }
                set details [string trim $details]

                Rappture::Tooltip::for $itk_component(simstatus) $details
                $itk_component(simstatus) insert end " "
                $itk_component(simstatus) insert end "(details...)" popup
            }
            $itk_component(simstatus) configure -state disabled
        }
    } else {
        if {"" != $itk_option(-simcontrolbackground)} {
            set simcbg $itk_option(-simcontrolbackground)
        } else {
            set simcbg $itk_option(-background)
        }
        $itk_interior.simol configure \
            -background $itk_option(-simcontroloutline)
        configure -simcontrolcolor $simcbg

        if {$_uq_active == 0} {
            $itk_component(simulate) configure -state disabled
        }
        $itk_component(abort) configure -state normal

        $itk_component(simstatus) configure -state normal
        $itk_component(simstatus) delete 1.0 end
        set mesg [lindex $args 0]
        if {"" != $mesg} {
            $itk_component(simstatus) insert end $mesg
        }
        $itk_component(simstatus) configure -state disabled
    }
}

# ----------------------------------------------------------------------
# USAGE: _simOutput <message>
#
# Invoked automatically whenever output comes in while running the
# tool.  Extracts any =RAPPTURE-???=> messages from the output and
# sends the output to the display.  For example, any
# =RAPPTURE-PROGRESS=> message pops up the progress meter and updates
# it to show the latest progress message.  This is useful for
# long-running tools, to let the user know how much longer the
# simulation will take.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_simOutput {message} {
    #
    # Scan through and pick out any =RAPPTURE-PROGRESS=> messages first.
    #
    set pattern {=RAPPTURE-PROGRESS=> *([-+]?[0-9]+) +([^\n]*)(\n|$)}
    while {[regexp -indices $pattern $message match percent mesg]} {

        foreach {i0 i1} $percent break
        set percent [string range $message $i0 $i1]

        foreach {i0 i1} $mesg break
        set mesg [string range $message $i0 $i1]

        pack $itk_component(progress) -fill x -padx 10 -pady 10
        $itk_component(progress) settings -percent $percent -message $mesg

        foreach {i0 i1} $match break
        set message [string replace $message $i0 $i1]
    }

    #
    # Now handle SUBMIT-PROGRESS
    #
    while {[regexp -indices {=SUBMIT-PROGRESS=> aborted=([0-9]+) finished=([0-9]+) failed=([0-9]+) executing=([0-9]+)\
        waiting=([0-9]+) setting_up=([0-9]+) setup=([0-9]+) %done=([0-9.]+) timestamp=([0-9.]+)(\n|$)} $message \
        match aborted finished failed executing waiting setting_up setup percent ts mesg]} {

        set mesg ""
        foreach {i0 i1} $percent break
        set percent [string range $message $i0 $i1]
        foreach {i0 i1} $failed break
        set failed [string range $message $i0 $i1]
        foreach {i0 i1} $match break
        set message [string replace $message $i0 $i1]

        if {$failed != 0} {set mesg "$failed jobs failed!"}
        if {$percent >= 100} { set mesg "Jobs finished.  Analyzing results..."}

        pack $itk_component(progress) -fill x -padx 10 -pady 10
        $itk_component(progress) settings -percent $percent -message $mesg
    }

    #
    # Break up the remaining lines according to =RAPPTURE-ERROR=> messages.
    # Show errors in a special color.
    #
    $itk_component(runinfo) configure -state normal
    set pattern {=RAPPTURE-([a-zA-Z]+)=>([^\n]*)(\n|$)}
    while {[regexp -indices $pattern $message match type mesg]} {
        foreach {i0 i1} $match break
        set first [string range $message 0 [expr {$i0-1}]]
        if {[string length $first] > 0} {
            $itk_component(runinfo) insert end $first
            $itk_component(runinfo) insert end \n
        }

        foreach {t0 t1} $type break
        set type [string range $message $t0 $t1]
        foreach {m0 m1} $mesg break
        set mesg [string range $message $m0 $m1]
        if {[string length $mesg] > 0 && $type != "RUN"} {
            $itk_component(runinfo) insert end $mesg $type
            $itk_component(runinfo) insert end \n $type
        }

        set message [string range $message [expr {$i1+1}] end]
    }

    if {[string length $message] > 0} {
        $itk_component(runinfo) insert end $message
        if {[$itk_component(runinfo) get end-2char] != "\n"} {
            $itk_component(runinfo) insert end "\n"
        }
        $itk_component(runinfo) see end
    }
    $itk_component(runinfo) configure -state disabled
}

# ----------------------------------------------------------------------
# USAGE: _resultTooltip
#
# Used internally to build the tooltip string displayed for the
# result selector.  If the current page has an associated description,
# then it is displayed beneath the result.
#
# Returns the string for the tooltip.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_resultTooltip {} {
    set tip ""
    set name [$itk_component(viewselector) value]
    if {[info exists _label2desc($name)] &&
        [string length $_label2desc($name)] > 0} {
        append tip "$_label2desc($name)\n\n"
    }
    if {[array size _label2page] > 1} {
        append tip "Use this control to display other output results."
    }
    return $tip
}

# ----------------------------------------------------------------------
# USAGE: _fixSimControl
#
# Used internally to add or remove the simulation control at the
# top of the analysis area.  This is controlled by the -simcontrol
# option.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_fixSimControl {} {
    switch -- $itk_option(-simcontrol) {
        on {
            pack $itk_interior.simol -fill x -before $itk_interior.nb
        }
        off {
            pack forget $itk_interior.simol
        }
        auto {
            #
            # If we have two or more radiodials, then there is a
            # chance of encountering a combination of parameters
            # with no data, requiring simulation.
            #
            if {[$itk_component(resultselector) size -controls] >= 2} {
                pack $itk_interior.simol -fill x -before $itk_interior.nb
            } else {
                pack forget $itk_interior.simol
            }
        }
        default {
            error "bad value \"$itk_option(-simcontrol)\": should be on, off, auto"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixNotebook
#
# Used internally to switch the active notebook page
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_fixNotebook {} {
    switch -- $itk_option(-notebookpage) {
        about {
            $itk_component(notebook) current about
        }
        simulate {
            $itk_component(notebook) current simulate
        }
        analyze {
            $itk_component(notebook) current analyze
        }
        default {
            error "bad value \"$itk_option(-notebookpage)\": should be about, simulate, analyze"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _isPdbTrajectory <data>
#
# Used internally to determine whether pdb or lammps data represents a
# trajectory rather than a single frame
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_isPdbTrajectory {data} {
    if { [llength $data] == 0 } {
        return 0
    }
    set nModels 0
    foreach line $data {
        if { [string match "MODEL*" $line] }  {
            incr nModels
            if { $nModels > 1 } {
                # Stop if more than one model found.  No need to count them
                # all.
                return 1
            }
        }
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: _isLammpsTrajectory <data>
#
# Used internally to determine whether pdb or lammps data represents a
# trajectory rather than a single frame
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_isLammpsTrajectory { data } {
    if { [llength $data] == 0 } {
        return 0
    }
    set nModels 0
    foreach line $data {
        if { [regexp {^[\t ]*ITEM:[ \t]+TIMESTEP} $line] } {
            incr nModels
            if { $nModels > 1 } {
                # Stop if more than one model found.  No need to count them
                # all.
                return 1
            }
        }
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: _pdbToSequence <xmlobj> ?<path>?
#
# If the molecule element is a trajectory, delete the original
# and create a sequence of individual molecules.
# Used internally to detect any molecule output elements that contain
# trajectory data.  Trajectories will be converted into sequences of
# individual molecules.  All other elements will be unaffected. Scans
# the entire xml tree if a starting path is not specified.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_pdbToSequence {xmlobj path id child data} {

    set seqLabel [$xmlobj get ${child}.about.label]
    set descr    [$xmlobj get ${child}.about.description]
    set formula  [$xmlobj get ${child}.components.molecule.formula]
    $xmlobj remove $child

    set sequence  $path.sequence($id)
    $xmlobj put ${sequence}.about.label $seqLabel
    $xmlobj put ${sequence}.about.description $descr
    $xmlobj put ${sequence}.index.label "Frame"

    set frameNum 0
    set numLines [llength $data]
    for { set i 0 } { $i < $numLines } { incr i } {
        set line [lindex $data $i]
        set line [string trim $line]
        set contents {}
        if { [string match "MODEL*" $line] } {
            # Save the contents until we get an ENDMDL record.
            for {} { $i < $numLines } { incr i } {
                set line [lindex $data $i]
                set line [string trim $line]
                if { $line == "" } {
                    continue;           # Skip blank lines.
                }
                if { [string match "ENDMDL*" $line] } {
                    break;
                }
                append contents $line\n
            }
            set frame ${sequence}.element($frameNum)
            $xmlobj put ${frame}.index $frameNum

            set molecule ${frame}.structure.components.molecule
            $xmlobj put ${molecule}.pdb $contents
            $xmlobj put ${molecule}.formula $formula
            incr frameNum
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _lammpsToSequence <xmlobj> ?<path>?
#
# If the molecule element is a trajectory, delete the original
# and create a sequence of individual molecules.
# Used internally to detect any molecule output elements that contain
# trajectory data.  Trajectories will be converted into sequences of
# individual molecules.  All other elements will be unaffected. Scans
# the entire xml tree if a starting path is not specified.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_lammpsToSequence {xmlobj path id child data} {

    set seqLabel [$xmlobj get ${child}.about.label]
    set descr    [$xmlobj get ${child}.about.description]
    set typemap  [$xmlobj get ${child}.components.molecule.lammpstypemap]
    $xmlobj remove $child

    set sequence ${path}.sequence($id)
    $xmlobj put ${sequence}.about.label $seqLabel
    $xmlobj put ${sequence}.about.description $descr
    $xmlobj put ${sequence}.index.label "Frame"

    set frameNum 0
    set frameContents ""
    set inModel 0
    foreach line $data {
        set line [string trim $line]
        if { $line == "" } {
            continue;                   # Skip blank lines
        }
        if {[regexp {^[\t ]*ITEM:[ \t]+ATOMS} $line] } {
            if { $inModel && $frameContents != "" } {
                set frame ${sequence}.element($frameNum)
                $xmlobj put ${frame}.index $frameNum

                set molecule ${frame}.structure.components.molecule
                $xmlobj put ${molecule}.lammps $frameContents
                $xmlobj put ${molecule}.lammpstypemap $typemap

                incr frameNum
                set frameContents ""
            }
            set inModel 1
        } elseif { [scan $line "%d %d %f %f %f" a b c d e] == 5 } {
            if { !$inModel } {
                puts stderr "found \"$line\" without previous \"ITEM: ATOMS\""
                set inModel 1
            }
            append frameContents $line\n
        }
    }
    if { $frameContents != "" } {
        set frame ${sequence}.element($frameNum)
        $xmlobj put ${frame}.index $frameNum

        set molecule ${frame}.structure.components.molecule
        $xmlobj put ${molecule}.lammps $frameContents
        $xmlobj put ${molecule}.lammpstypemap $typemap
    }
}

# ----------------------------------------------------------------------
# USAGE: _trajToSequence <xmlobj> ?<path>?
#
#       Check for PDB and LAMMPS trajectories in molecule data and rewrite
#       the individual models as a sequence of molecules.  Used internally
#       to detect any molecule output elements that contain trajectory data.
#       Trajectories will be converted into sequences of individual molecules.
#       All other elements will be unaffected. Scans the entire xml tree if a
#       starting path is not specified.
#
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_trajToSequence {xmlobj {path ""}} {
    # Remove leading dot from path, if present.
    if { [string index $path 0] == "." } {
        set path [string range $path 1 end]
    }
    # Otherwise check each child.
    foreach child [$xmlobj children $path] {
        set current ${path}.${child}
        if { [string match "structure*" $child] } {
            set isTraj [$xmlobj get ${current}.components.molecule.trajectory]
            if { $isTraj == "" || !$isTraj } {
                continue;               # Not a trajectory.
            }
            # Look for trajectory if molecule element found.  Check both pdb
            # data and lammps data.
            set type [$xmlobj element -as type $current]
            set id   [$xmlobj element -as id $current]
            set pdbdata    [$xmlobj get ${current}.components.molecule.pdb]
            set lammpsdata [$xmlobj get ${current}.components.molecule.lammps]
            if { $pdbdata != "" && $lammpsdata != "" } {
                puts stderr \
                    "found both <pdb> and <lammps> elements: picking pdb"
            }
            set pdbdata [split $pdbdata \n]
            set lammpsdata [split $lammpsdata \n]
            if { [_isPdbTrajectory $pdbdata] } {
                _pdbToSequence $xmlobj $path $id $current $pdbdata
            } elseif { [_isLammpsTrajectory $lammpsdata] } {
                _lammpsToSequence $xmlobj $path $id $current $lammpsdata
            }
            continue
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -simcontrol
#
# Controls whether or not the Simulate button is showing.  In some
# cases, it is not needed.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Analyzer::simcontrol {
    _fixSimControl
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -notebookpage
#
# Controls which page of the analyzer notebook is shown. It is
# particularly needed when using rerun, when you don't want to
# "simulate -ifneeded" because an actual simulation might be
# kicked off due to differences between tool.xml and run.xml
# ----------------------------------------------------------------------
itcl::configbody Rappture::Analyzer::notebookpage {
    _fixNotebook
}

itcl::body Rappture::Analyzer::PostMenu { m } {
    if { [FindSimsetsForApp $_appName] == 0 } {
        $m entryconfigure "Load Simulations" -state disable
        $m entryconfigure "Share Simulations" -state disable
        $m entryconfigure "Delete Simulations" -state disabled
    } else {
        $m entryconfigure "Load Simulations" -state normal
        $m entryconfigure "Share Simulations" -state normal
        $m entryconfigure "Delete Simulations" -state normal
    }
    if { [$_resultset size] == 0 } {
        $m entryconfigure "Save Simulations" -state disabled
    } else {
        $m entryconfigure "Save Simulations" -state normal
    }
}
        
#
# Build Help menu 
#
itcl::body Rappture::Analyzer::buildMenu { m url appName } {
    menu $m \
        -tearoff 0 \
        -postcommand [itcl::code $this PostMenu $m]
    if { $appName == "" || $url == "" } {
        set state disabled
    } else {
        set state normal
    }
    set webcmd Rappture::filexfer::webpage
    set group "app-$_appName"
    if { [info exists env(SESSION)] }  {
        set referer "&referrer=$url/tools/$appName/session?sess=$env(SESSION)"
    } else {
        set referer ""
    }
    $m add command -label "About this tool" \
        -command [list $webcmd "$url/tools/$appName"] \
        -state $state
    $m add command -label "Questions?" \
        -command [list $webcmd "$url/resources/$_appName/questions"] \
        -state $state
    $m add command -label "Tickets" -state $state \
        -command [list $webcmd "$url/feedback/report_problems?group=$group$referer"]
    $m add command -label "Wish List" -state disabled
    $m add separator
    $m add command -label "Save Simulations" \
        -command [itcl::code $this SelectSimsetNameAndSave] \
        -state $state
    $m add command -label "Load Simulations" \
        -command [itcl::code $this SelectSimsetForLoading] \
        -state $state
    $m add command -label "Delete Simulations" \
        -command [itcl::code $this SelectSimsetForDeletion] \
        -state $state
    $m add command -label "Share Simulations" \
        -command [itcl::code $this SelectSimsetToShare] \
        -state $state
    return $m
}

itcl::body Rappture::Analyzer::BuildSimulationTable { top } {
    # All weirdness is due to the Resultset object.

    # First create nodes for each simulation.  The node label is the
    # simulation number (such as #1, #2, etc).  
    eval $_tree delete 0
    set labels "selected simnum"
    foreach {name index} [$_resultset diff values simnum] {
        set node [$_tree insert 0 -label $name]
        $_tree set $node "simnum" $name
        set index2name($index) $name
        
    }
    # Next fill in the xmlobj associated with each simulation.  We assume
    # that the order is the the same as the simulation number.
    set index 0
    foreach {xmlobj dummy} [$_resultset diff values xmlobj]  {
        set node [$_tree findchild 0 $index2name($index)]
        set runfile [$xmlobj get output.filename]
        set version [$xmlobj get output.version]
        $_tree set $node \
            "selected" 1 "xmlobj" $xmlobj "runfile" $runfile "version" $version
        incr index
    }
    # Finally for each different input, for each simulation add the
    # label and value for the specific input.
    foreach name [$_resultset diff names] {
        # Ignore non-input names.
        if { $name == "xmlobj" || $name == "simnum" } {
            continue
        }
        foreach node [$_tree children 0] {
            set xmlobj [$_tree get $node xmlobj]
            set label [$xmlobj get $name.about.label]
            set value [$xmlobj get $name.current]
            if { [$_tree exists $node $label] } {
                puts stderr "This can't be: $label already exists in $node"
            }
            if { [lsearch $labels $label] < 0 } {
                lappend labels $label
            }
            $_tree set $node $label $value
        }
    }
    frame $top
    set tv $top.tv
    blt::treeview $top.tv -tree $_tree \
        -height 1i \
        -xscrollcommand [list $top.xs set] \
        -yscrollcommand [list $top.ys set] \
        -font "Arial 10"
    scrollbar $top.xs -orient horizontal -command "$tv xview"
    scrollbar $top.ys -orient vertical -command "$tv yview"
        
    eval $tv column insert end $labels
    $tv style checkbox checkbox -showvalue no 
    $tv column configure simnum -title "Simulation" 
    $tv column configure selected -title "Save" -style checkbox \
        -width .5i -edit yes 
    $tv column configure treeView -hide yes 
    blt::table $top \
        0,0 $top.tv -fill both \
        0,1 $top.ys -fill y \
        1,0 $top.xs -fill x
    blt::table configure $top r* c* -resize none
    blt::table configure $top r0 c0 -resize both
}

itcl::body Rappture::Analyzer::CleanName { name } {
    regsub -all {/} $name {_} name
    return $name
}

#
# Get the results and display them in a table.
#
# Use the table to omit specific results
# Use file chooser to save to specific 
itcl::body Rappture::Analyzer::SelectSimsetNameAndSave {} {
    if { [EditSimset] } {
        set name [CleanName $_saved(Name)]
        set saveDir [file join ~/data/saved $_appName $name]
        set saveDir [file normalize $saveDir]
        file mkdir $saveDir 
        set fileName [file join $saveDir $name.sav]
        if { [file exists $fileName] } {
            if { ![OverwriteSaveFile] } {
                return
            }
        }
        WriteSimsetFile $_appName $fileName 0
    }
}

#
# Get the results and display them in a table.
#
# Use the table to omit specific results
# Use file chooser to save to specific 
itcl::body Rappture::Analyzer::SelectSimsetToShare {} {
    if { [GetSimset "share"] } {
        global env
        set name [CleanName $_saved(Name)]
        set shareDir [file join /data/tools/shared $env(USER) $_appName $name]
        if { [catch {
            CreateSharedPath $shareDir
        } errs] != 0 } {
            puts stderr errs=$errs
            return 
        }
        set fileName [file join $shareDir $name.sav]
        if { [file exists $fileName] } {
            if { ![OverwriteSaveFile] } {
                return
            }
        }
        WriteSimsetFile $_appName $fileName 1
        ExportURL $_appName $shareDir
    }
}

#
# Checks editor widgets to see if the name and description have been
# set.  If this is so it enables the save button in the editor.
#
itcl::body Rappture::Analyzer::CheckSimsetDetails {} {
    set popup .savesimset
    if { ![winfo exists $popup] } {
        return
    }
    set inner [$popup component inner]
    set name [string trim [$inner.name get]]
    set desc [string trim [$inner.desc get 0.0 end]]
    $inner.controls.save configure -state disabled
    if { $name == "" || $desc == "" } {
        return
    }
    $inner.controls.save configure -state normal
}

#
# Gathers all the information from the simset editor into the _saved array
#
itcl::body Rappture::Analyzer::SaveSimulations {} {
    set popup .savesimset
    if { ![winfo exists $popup] } {
        return
    }
    set inner [$popup component inner]
    array unset _saved
    set name [string trim [$inner.name get]]
    set desc [string trim [$inner.desc get 0.0 end]]
    set _saved(Application) $_appName
    set _saved(Revision) $_revision
    set _saved(Date) [clock format [clock seconds]]
    set _saved(Name) $name
    set _saved(Description) $desc
    set files {}
    foreach node [$_tree children root] {
        set fileName [$_tree get $node runfile]
        lappend files $fileName
    }
    set _saved(Files) $files
    set _done 1 
}

itcl::body Rappture::Analyzer::Cancel {} {
    set _done 0
}

itcl::body Rappture::Analyzer::Ok {} {
    set popup .selectsimset
    if { ![winfo exists $popup] } {
        return
    }
    set inner [$popup component inner]
    set tv $inner.tv
    if { ![$tv selection present] } {
        return
    }
    set node [$tv curselection]
    array unset _saved
    array set _saved [$_tree get $node]
    set saveDir [file dirname $_saved(FileName)]
    set files {}
    foreach file $_saved(Files) {
        if { [file pathtype $file] == "relative" } {
            set file [file join $saveDir $file]
        }
        lappend files $file
    }
    set _saved(Files) $files
    set _done 1 
}

#
# WriteSimsetFile --
#
#       Write the .sav file and the runfiles in the 
#            ~/data/saved/$_appName/$name
#       directory.  
# The run files will be in the same directory as the .sav file.  
# For example, if the simset file is /path/to/appName/myName/myName.sav, 
# the runfile directory is /path/to/appName/myName.
#
itcl::body Rappture::Analyzer::WriteSimsetFile { appName fileName share } {
    set saveDir [file dirname $fileName]
    if { [file exists $saveDir] } {
        file delete -force $saveDir
    }
    if { [catch {
        if { $share } {
            CreateSharedPath $saveDir
        } else {
            file mkdir $saveDir
        }
        set f [open $fileName "w"]
        puts $f [list "Name"        $_saved(Name)]
        puts $f [list "Description" $_saved(Description)]
        puts $f [list "Date"        $_saved(Date)]
        global env
        puts $f [list "Creator"     $env(USER)]
        puts $f [list "Application" $_saved(Application)]
        puts $f [list "Revision"    $_saved(Revision)]
        set runfiles ""
        foreach file $_saved(Files) {
            set tail [file tail $file]
            set dest [file join $saveDir $tail]
            if { $share } {
                InstallSharedFile $saveDir $file
            } else {
                file copy -force $file $dest
            }
            lappend runfiles $tail
        }
        puts $f [list "Files" $runfiles]
        close $f
        if { $share } {
            file attributes $fileName -permissions o+r,g+rw
        }
    } errs] != 0 } {
        global errorInfo
        puts stderr "errs=$errs errorInfo=$errorInfo"
    }
}

itcl::body Rappture::Analyzer::EditSimset {} {
    set popup .savesimset
    if { [winfo exists $popup] } {
        destroy $popup
    }
    # Create a popup for the print dialog
    Rappture::Balloon $popup -title "Save set of simulations..."
    set inner [$popup component inner]

    label $inner.name_l -text "Simulation Set Name"
    entry $inner.name -background white
    label $inner.desc_l -text "Simulation Set Description" -height 5 
    text $inner.desc  -background white
    label $inner.select_l -text "Selected Simulations"
    BuildSimulationTable $inner.select
    frame $inner.controls
    bind $inner.desc <KeyPress> [itcl::code $this CheckSimsetDetails]
    bind $inner.name <KeyPress> [itcl::code $this CheckSimsetDetails]
    bind $inner.desc <Motion> [itcl::code $this CheckSimsetDetails]
    bind $inner.name <Motion> [itcl::code $this CheckSimsetDetails]
    button $inner.controls.cancel -text "Cancel" \
        -command [itcl::code $this Cancel]
    button $inner.controls.save -text "Save" \
        -command [itcl::code $this SaveSimulations]
    blt::table $inner.controls \
        0,0 $inner.controls.cancel \
        0,1 $inner.controls.save
    
    blt::table $inner \
        0,0 $inner.name_l -anchor ne \
        0,1 $inner.name -fill x \
        1,0 $inner.desc_l -anchor ne \
        1,1 $inner.desc -fill both \
        2,0 $inner.select_l -anchor ne \
        2,1 $inner.select -fill both \
        3,1 $inner.controls  -fill x
    blt::table configure $inner r1 -height 1i
    blt::table configure $inner r2 -pad 20
    $popup configure \
        -deactivatecommand [itcl::code $this Cancel] 

    set inner [$popup component inner]
    if {[$_resultset size] > 1} {
        blt::table $inner \
            2,0 $inner.select_l -anchor ne \
            2,1 $inner.select -fill both 
    } else {
        blt::table forget $inner.select $inner.select_l
    }
    CheckSimsetDetails
    update
    # Activate the popup and call for the output.
    $popup activate $itk_component(help) below
    Cancel
    tkwait variable [itcl::scope _done]
    set doSave $_done
    $popup deactivate 
    return $doSave
}


itcl::body Rappture::Analyzer::ReadSimsetFile { fileName } {
    if { [catch {
        set f [open $fileName "r"]
        set contents [read $f]
        close $f
        array set _saved $contents
    } errs] != 0 } {
        return 0
    }
    if { ![info exists _saved(Files)] } {
        return 0
    }
    set saveDir [file dirname $fileName]
    foreach file $_saved(Files) {
        if { [file pathtype $file] == "relative" } {
            set file [file join $saveDir $file]
        }
        if { ![file readable $file] } {
            puts stderr "runfile $file isn't readable"
            return 0
        }
    }
    set _saved(FileName) $fileName
    return 1
}

#
# Run files can be either explicitly named with their absolute or
# relative path.  If the path is relative it is assumed the parent
# directory is relative to the simset file.
#
#   /my/path/to/myfile.sav 
#   /my/path/to/myfile/runfiles...
#
itcl::body Rappture::Analyzer::LoadSimulations { files } {
    set loadobjs {}
    set saveDir [file dirname $_saved(FileName)]
    foreach runfile $files {
        if { [file pathtype $runfile] == "relative" } {
            set runfile [file join $saveDir $runfile]
        }
        if { ![file exists $runfile] } {
            puts stderr "can't find run: \"$runfile\""
            continue
        }
        set status [catch {Rappture::library $runfile} result]
        lappend loadobjs $result
    }
    clear
    foreach runobj $loadobjs {
        load $runobj
    }
    configure -notebookpage analyze
    global win
    $win.pager configure -nosim 1 
    $win.pager current analyzer
    $win.pager configure -nosim 0 
}

#
# Delete selected simulation set.
#
itcl::body Rappture::Analyzer::SelectSimsetForDeletion {} {
    if { [FindSimsetsForApp $_appName] == 0 } {
        return
    }
    if { [GetSimset "delete"] } {
        set saveDir [file dirname $_saved(FileName)]
        file delete -force $saveDir
    }
}

#
# Find all .sav files for an application and load them into the tree.
#
itcl::body Rappture::Analyzer::FindSimsetsForApp { appName } {
    if { $appName == "" } {
        puts stderr "No application name found"
        return 0
    }
    $_tree delete 0
    foreach fileName [glob -nocomplain ~/data/saved/$appName/*/*.sav] {
        if { [ReadSimsetFile $fileName] } {
            if { $_revision > 0 && [info exists _saved(Revision)] &&
                 $_revision != $_saved(Revision) } {
                continue;               # Revision doesn't match.
            }
            $_tree insert 0 -label $_saved(Name) -data [array get _saved] 
        }
    }
    return [$_tree degree 0]
}

#
# Create dialog to select a simset from the currently available simsets.
#
itcl::body Rappture::Analyzer::GetSimset { name } {
    set popup .selectsimset
    if { [winfo exists $popup] } {
        destroy $popup
    }
    # Create a popup for the print dialog
    set title [string tolower $name]
    Rappture::Balloon $popup -title "Select set of simulations to $title..."
    set inner [$popup component inner]
    
    set tv $inner.tv
    blt::treeview $inner.tv -tree $_tree \
        -height 1i \
        -width 5i \
        -xscrollcommand [list $inner.xs set] \
        -yscrollcommand [list $inner.ys set] \
        -font "Arial 10"
    scrollbar $inner.xs -orient horizontal -command "$tv xview"
    scrollbar $inner.ys -orient vertical -command "$tv yview"
    frame $inner.controls
    
    $tv column insert end "Name" "Description" "Date" "Creator"
    $tv column configure treeView -hide yes 
    blt::table $inner \
        0,0 $inner.tv -fill both \
        0,1 $inner.ys -fill y \
        1,0 $inner.xs -fill x
    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r0 c0 -resize both

    button $inner.controls.cancel -text "Cancel" \
        -command [itcl::code $this Cancel]
    set title [string totitle $name]
    button $inner.controls.save -text $title \
        -command [itcl::code $this Ok]
    blt::table $inner.controls \
        0,0 $inner.controls.cancel \
        0,1 $inner.controls.save
    
    blt::table $inner \
        0,0 $inner.tv -fill both \
        0,1 $inner.ys -fill y \
        1,0 $inner.xs -fill x \
        3,0 $inner.controls  -fill x -cspan 2
    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r0 c0 -resize both
    blt::table configure $inner r2 -height 0.1i
    
    $popup configure \
        -deactivatecommand [itcl::code $this Cancel] 

    $tv selection set [$_tree firstchild 0]
    set inner [$popup component inner]
    update
    # Activate the popup and call for the output.
    $popup activate $itk_component(help) below
    Cancel
    tkwait variable [itcl::scope _done]
    set result $_done
    $popup deactivate 
    return $result
}

itcl::body Rappture::Analyzer::SelectSimsetForLoading {} {
    if { [FindSimsetsForApp $_appName] == 0 } {
        return
    }
    if { [GetSimset "load"] } {
        LoadSimulations $_saved(Files)
    }
}

itcl::body Rappture::Analyzer::reload { fileName } {
    if { [catch {
        ReadSimsetFile $fileName
        if { [llength $_saved(Files)] > 0 } {
            #StartSplashScreen
            LoadSimulations $_saved(Files)
            #HideSplashScreen
        }
    } errs] != 0 } {
        puts stderr "can't load \"$fileName\": errs=$errs"
    }
}

itcl::body Rappture::Analyzer::BuildQuestionDialog { popup } {
    toplevel $popup -background grey92 -bd 2 -relief raised
    wm withdraw $popup
    wm overrideredirect $popup true
    set inner $popup
    # Create the print dialog widget and add it to the the balloon popup.
    label $popup.title -text " " \
        -padx 10 -pady 10 \
        -background grey92 
    button $popup.yes -text "Yes" \
        -highlightthickness 0 \
        -bd 2  \
        -command [itcl::code $this Save] 
    button $popup.no -text "No" \
        -highlightthickness 0 \
        -bd 2 \
        -command [itcl::code $this Cancel] 
    blt::table $popup \
        0,0 $popup.title -cspan 2  -fill x -pady 4 \
        1,0 $popup.yes -width { 0 1i .6i } -pady 4 \
        1,1 $popup.no -width { 0 1i .6i }  -pady 4 
    blt::table configure $popup  r2 -height 0.125i
}

itcl::body Rappture::Analyzer::Save {} {
    set _done 1
}

itcl::body Rappture::Analyzer::OverwriteSaveFile {} {
    set popup .question
    if { ![winfo exists $popup] } {
        BuildQuestionDialog $popup 
    }
    set text "Simulation set \"$_saved(Name)\" already exists. Overwrite?"
    $popup.title configure -text $text 
    $popup.yes configure -text "Save" 
    $popup.no configure -text "Cancel" 
    set main [winfo toplevel $itk_component(help)]
    set pw [winfo reqwidth $popup]
    set ph [winfo reqheight $popup]
    set sw [winfo reqwidth $main]
    set sh [winfo reqheight $main]
    set rootx [winfo rootx $main]
    set rooty [winfo rooty $main]
    set x [expr $rootx + (($sw - $pw) / 2)]
    set y [expr $rooty + (($sh - $ph) / 2)]
    wm geometry $popup +$x+$y
    wm deiconify $popup 
    update
    grab $popup 
    # Activate the popup and call for the output.
    Cancel
    tkwait variable [itcl::scope _done]
    set doSave $_done
    grab release $popup 
    destroy $popup 
    return $doSave
}

itcl::body Rappture::Analyzer::InstallSharedFile { installdir file } {
    set dst [file join $installdir [file tail $file]]
    file copy -force $file $dst
    file attributes $dst -permissions g+rw,o+r
    return $dst
}

itcl::body Rappture::Analyzer::CreateSharedPath { path } {
    set dir ""
    foreach file [file split $path] {
        set dir [file join $dir $file]
        if { [file exists $dir] } {
            if { ![file isdirectory $dir] } {
                error "error in path \"$path\": \"$dir\" is not a directory."
            }
        } else {
            file mkdir $dir 
            file attributes $dir -permissions g+rwx,o+rx
        }
    }
}

itcl::body Rappture::Analyzer::ExportURL { appName installdir } {
    set fileName [file join $installdir url.txt]
    set name [file tail $installdir]
    set f [open $fileName "w"]
    puts $f "https://nanohub.org/tools/$appName/invoke?params=file(simset):$installdir/$name.sav\n"
    close $f
    file attributes $fileName -permissions g+rw,o+r
    if { [file exists /usr/bin/exportfile] } {
        set msgFile [file join $::Rappture::installdir export.html]
        ExportFile $fileName $msgFile
    }
}

itcl::body Rappture::Analyzer::ExportFile { src msgFile } {
    set dstdir ~/.filexfer/exportfile
    file mkdir $dstdir
    set dst [file join $dstdir [file tail $src]]
    if { [file exists $dst] } {
        puts stderr "could be a problem: already waiting to export $dst"
    }
    file copy -force $src $dst
    # Now export the file.
    if { [catch {
        global exportFileVar 
        set exportFileVar 0;            # Kill any previous export.
        set mesg "Select file to export to desktop/laptop"
        update
        set cmd "/usr/bin/exportfile --delete $dst --timeout 3 "
        if { [file exists $msgFile] } { 
            append cmd "--message $msgFile"
        }
        blt::bgexec exportFileVar /bin/sh -c $cmd &
    } errs]  != 0 } {
        global errorInfo
        puts stderr "err: $errorInfo"
        file delete -force $dst
    }
}

