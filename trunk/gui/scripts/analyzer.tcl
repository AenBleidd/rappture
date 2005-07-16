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
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

option add *Analyzer.width 5i widgetDefault
option add *Analyzer.height 5i widgetDefault
option add *Analyzer.simControl "auto" widgetDefault
option add *Analyzer.simControlBackground "" widgetDefault
option add *Analyzer.simControlOutline gray widgetDefault
option add *Analyzer.simControlActiveBackground #ffffcc widgetDefault
option add *Analyzer.simControlActiveOutline black widgetDefault

option add *Analyzer.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault
option add *Analyzer.textFont \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault
option add *Analyzer.boldTextFont \
    -*-helvetica-bold-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::Analyzer {
    inherit itk::Widget

    itk_option define -textfont textFont Font ""
    itk_option define -boldtextfont boldTextFont Font ""
    itk_option define -simcontrol simControl SimControl ""
    itk_option define -simcontroloutline simControlOutline Background ""
    itk_option define -simcontrolbackground simControlBackground Background ""
    itk_option define -simcontrolactiveoutline simControlActiveOutline Background ""
    itk_option define -simcontrolactivebackground simControlActiveBackground Background ""
    itk_option define -holdwindow holdWindow HoldWindow ""

    constructor {tool args} { # defined below }
    destructor { # defined below }

    public method simulate {args}
    public method reset {}
    public method load {file}
    public method clear {}

    protected method _plot {args}
    protected method _reorder {comps}
    protected method _autoLabel {xmlobj path title cntVar}
    protected method _fixResult {}
    protected method _fixSize {}
    protected method _fixSimControl {}
    protected method _simState {state args}

    private variable _tool ""          ;# belongs to this tool
    private variable _control "manual" ;# start mode
    private variable _runs ""          ;# list of XML objects with results
    private variable _pages 0          ;# number of pages for result sets
    private variable _label2page       ;# maps output label => result set
    private variable _plotlist ""      ;# items currently being plotted

    private common job                 ;# array var used for blt::bgexec jobs
}
                                                                                
itk::usual Analyzer {
    keep -background -cursor foreground -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::constructor {tool args} {
    set _tool $tool

    itk_option add hull.width hull.height
    pack propagate $itk_component(hull) no

    frame $itk_interior.simol -borderwidth 1 -relief flat
    pack $itk_interior.simol -fill x

    frame $itk_interior.simol.simbg -borderwidth 0
    pack $itk_interior.simol.simbg -expand yes -fill both

    itk_component add simulate {
        button $itk_interior.simol.simbg.simulate -text "Simulate" \
            -command [itcl::code $this simulate]
    }
    pack $itk_component(simulate) -side left -padx 4 -pady 4

    itk_component add simstatus {
        text $itk_interior.simol.simbg.simstatus -borderwidth 0 \
            -highlightthickness 0 -height 1 -width 1 -wrap none \
            -state disabled
    } {
        usual
        ignore -highlightthickness
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

    Rappture::Scroller $w.info -xscrollmode off -yscrollmode auto
    pack $w.info -expand yes -fill both -padx 4 -pady 4
    itk_component add runinfo {
        text $w.info.text -width 1 -height 1 -wrap word \
            -borderwidth 0 -highlightthickness 0 \
            -state disabled
    } {
        usual
        ignore -borderwidth -relief
        rename -font -textfont textFont Font
    }
    $w.info contents $w.info.text

    # ------------------------------------------------------------------
    # ANALYZE PAGE
    # ------------------------------------------------------------------
    set w [$itk_component(notebook) insert end analyze]

    frame $w.top
    pack $w.top -side top -fill x -pady 8
    label $w.top.l -text "Result:" -font $itk_option(-font)
    pack $w.top.l -side left

    itk_component add resultselector {
        Rappture::Combobox $w.top.sel -width 50 -editable no
    } {
        usual
        rename -font -textfont textFont Font
    }
    pack $itk_component(resultselector) -side left -expand yes -fill x
    bind $itk_component(resultselector) <<Value>> [itcl::code $this _fixResult]

    itk_component add results {
        Rappture::Panes $w.pane
    }
    pack $itk_component(results) -expand yes -fill both
    set f [$itk_component(results) pane 0]

    itk_component add resultpages {
        Rappture::Notebook $f.nb
    }
    pack $itk_component(resultpages) -expand yes -fill both

    set f [$itk_component(results) insert end -fraction 0.1]
    itk_component add resultset {
        Rappture::ResultSet $f.rset \
            -clearcommand [itcl::code $this clear] \
            -settingscommand [itcl::code $this _plot] \
            -promptcommand [itcl::code $this _simState]
    }
    pack $itk_component(resultset) -expand yes -fill both
    bind $itk_component(resultset) <<Control>> [itcl::code $this _fixSize]

    eval itk_initialize $args

    #
    # Load up tool info on the first page.
    #
    $itk_component(toolinfo) tag configure title \
        -font $itk_option(-boldtextfont)

    set mesg [$tool xml get tool.title]
    if {"" != $mesg} {
        $itk_component(toolinfo) insert end $mesg title
        $itk_component(toolinfo) insert end "\n\n"
    }

    set mesg [$tool xml get tool.about]
    if {"" != $mesg} {
        $itk_component(toolinfo) insert end $mesg
    }
    $itk_component(toolinfo) configure -state disabled
    $itk_component(notebook) current about

    # tool can run on "manual" (default) or "auto"
    set cntl [$tool xml get tool.control]
    if {"" == $cntl} {
        set cntl [$tool xml get tool.control.type]
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
    foreach obj $_runs {
        itcl::delete object $obj
    }
    after cancel [itcl::code $this simulate]
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
    if {$args == "-ifneeded"} {
        # check to see if simulation is really needed
        $_tool sync
        if {[$itk_component(resultset) contains [$_tool xml object]]} {
            # not needed -- show results and return
            $itk_component(notebook) current analyze
            return
        }
        set args ""
    }

    # simulation is needed -- go to simulation page
    $itk_component(notebook) current simulate

    _simState off
    $itk_component(runinfo) configure -state normal
    $itk_component(runinfo) delete 1.0 end
    $itk_component(runinfo) insert end "Running simulation..."
    $itk_component(runinfo) configure -state disabled

    # if the hold window is set, then put up a busy cursor
    if {$itk_option(-holdwindow) != ""} {
        blt::busy hold $itk_option(-holdwindow)
        raise $itk_component(hull)
        update
    }

    # execute the job
    foreach {status result} [eval $_tool run $args] break

    # if job was aborted, then allow simulation again
    if {$result == "ABORT"} {
        _simState on "Aborted"
    }

    # read back the results from run.xml
    if {$status == 0 && $result != "ABORT"} {
        if {[regexp {=RAPPTURE-RUN=>([^\n]+)} $result match file]} {
            set status [catch {load $file} msg]
            if {$status != 0} {
                set result $msg
            }
        } else {
            set status 1
            set result "Can't find result file in output:\n\n$result"
        }
    }

    # back to normal
    if {$itk_option(-holdwindow) != ""} {
        blt::busy release $itk_option(-holdwindow)
    }
    $itk_component(abort) configure -state disabled

    if {$status != 0} {
        $itk_component(runinfo) configure -state normal
        $itk_component(runinfo) delete 1.0 end
        $itk_component(runinfo) insert end "Problem launching job:\n\n"
        $itk_component(runinfo) insert end $result
        $itk_component(runinfo) configure -state disabled
    } else {
        $itk_component(notebook) current analyze
    }
}

# ----------------------------------------------------------------------
# USAGE: reset
#
# Used to reset the analyzer whenever the input to a simulation has
# changed.  Sets the mode back to "simulate", so the user has to
# simulate again to see the output.  If the <start> option is set
# to "auto", the simulation is invoked immediately.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::reset {} {
    # check to see if simulation is really needed
    $_tool sync
    if {![$itk_component(resultset) contains [$_tool xml object]]} {
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
# USAGE: load <file>
#
# Loads the data from the given <file> into the appropriate results
# sets.  If necessary, new results sets are created to store the data.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::load {file} {
    # try to load new results from the given file
    set xmlobj [Rappture::library $file]
    lappend _runs $xmlobj

    # go through the analysis and find all result sets
    set haveresults 0
    foreach item [_reorder [$xmlobj children output]] {
        switch -glob -- $item {
            log* {
                _autoLabel $xmlobj output.$item "Output Log" counters
            }
            curve* - field* {
                _autoLabel $xmlobj output.$item "Plot" counters
            }
            structure* {
                _autoLabel $xmlobj output.$item "Structure" counters
            }
            table* {
                _autoLabel $xmlobj output.$item "Energy Levels" counters
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
        set size [$itk_component(resultset) size]
        set index [$itk_component(resultset) add $xmlobj]

        # add each result to a result viewer
        foreach item [_reorder [$xmlobj children output]] {
            set label [$xmlobj get output.$item.about.group]
            if {"" == $label} {
                set label [$xmlobj get output.$item.about.label]
            }

            set hidden [$xmlobj get output.$item.hide]
            set hidden [expr {"" != $hidden && $hidden}]

            if {"" != $label && !$hidden} {
                if {![info exists _label2page($label)]} {
                    set name "page[incr _pages]"
                    set page [$itk_component(resultpages) insert end $name]
                    set _label2page($label) $page
                    Rappture::ResultViewer $page.rviewer
                    pack $page.rviewer -expand yes -fill both -pady 4

                    $itk_component(resultselector) choices insert end \
                        $name $label
                }

                # add/replace the latest result into this viewer
                set page $_label2page($label)

                if {![info exists reset($page)]} {
                    $page.rviewer clear $index
                    set reset($page) 1
                }
                $page.rviewer add $index $xmlobj output.$item
            }
        }
    }

    # if there is only one result page, take down the selector
    set w [$itk_component(notebook) page analyze]
    if {[$itk_component(resultselector) choices size] <= 1} {
        pack forget $w.top
    } else {
        pack $w.top -before $itk_component(results) -side top -fill x
    }

    # show the first page by default
    set first [$itk_component(resultselector) choices get -label 0]
    if {$first != ""} {
        set page [$itk_component(resultselector) choices get -value 0]
        $itk_component(resultpages) current $page
        $itk_component(resultselector) value $first
    }
}

# ----------------------------------------------------------------------
# USAGE: clear
#
# Discards all results previously loaded into the analyzer.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::clear {} {
    foreach obj $_runs {
        itcl::delete object $obj
    }
    set _runs ""

    $itk_component(resultset) clear
    $itk_component(results) fraction end 0.1

    foreach label [array names _label2page] {
        set page $_label2page($label)
        $page.rviewer clear
    }
    $itk_component(resultselector) value ""
    $itk_component(resultselector) choices delete 0 end
    catch {unset _label2page}
    set _plotlist ""

    #
    # HACK ALERT!!
    # The following statement should be in place, but it causes
    # vtk to dump core.  Leave it out until we can fix the core dump.
    # In the mean time, we leak memory...
    #
    #$itk_component(resultpages) delete -all
    #set _pages 0

    _simState on
    _fixSimControl
    reset
}

# ----------------------------------------------------------------------
# USAGE: _plot ?<index> <options> <index> <options>...?
#
# Used internally to update the plot shown in the current result
# viewer whenever the resultset settings have changed.  Causes the
# desired results to show up on screen.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_plot {args} {
    set _plotlist $args

    set page [$itk_component(resultselector) value]
    set page [$itk_component(resultselector) translate $page]
    set f [$itk_component(resultpages) page $page]
    $f.rviewer plot clear
    foreach {index opts} $_plotlist {
        $f.rviewer plot add $index $opts
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
    set page [$itk_component(resultselector) value]
    set page [$itk_component(resultselector) translate $page]
    if {$page != ""} {
        $itk_component(resultpages) current $page

        set f [$itk_component(resultpages) page $page]
        $f.rviewer plot clear
        eval $f.rviewer plot add $_plotlist
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
    set f [$itk_component(results) fraction end]
    if {$f < 0.4} {
        $itk_component(results) fraction end [expr {$f+0.15}]
    }
    _fixSimControl
}

# ----------------------------------------------------------------------
# USAGE: _simState <boolean> ?<message>? ?<settings>?
#
# Used internally to change the "Simulation" button on or off.
# If the <boolean> is on, then any <message> and <settings> are
# displayed as well.  The <message> is a note to the user about
# what will be simulated, and the <settings> are a list of
# tool parameter settings of the form {path1 val1 path2 val2 ...}.
# When these are in place, the next Simulate operation will use
# these settings.  This helps fill in missing data values.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_simState {state args} {
    if {$state} {
        $itk_interior.simol configure \
            -background $itk_option(-simcontrolactiveoutline)
        $itk_interior.simol.simbg configure \
            -background $itk_option(-simcontrolactivebackground)
        $itk_component(simulate) configure \
            -highlightbackground $itk_option(-simcontrolactivebackground)
        $itk_component(simstatus) configure \
            -background $itk_option(-simcontrolactivebackground)

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
                    set str [$_tool xml get $path.about.label]
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
        $itk_interior.simol.simbg configure -background $simcbg
        $itk_component(simulate) configure -highlightbackground $simcbg
        $itk_component(simstatus) configure -background $simcbg

        $itk_component(simulate) configure -state disabled
        $itk_component(abort) configure -state normal

        $itk_component(simstatus) configure -state normal
        $itk_component(simstatus) delete 1.0 end
        $itk_component(simstatus) configure -state disabled
        Rappture::Tooltip::for $itk_component(simstatus) ""
    }
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
            if {[$itk_component(resultset) size -controls] >= 2} {
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
# CONFIGURATION OPTION: -simcontrol
#
# Controls whether or not the Simulate button is showing.  In some
# cases, it is not needed.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Analyzer::simcontrol {
    _fixSimControl
}
