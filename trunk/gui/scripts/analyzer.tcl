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
#  Copyright (c) 2004  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *Analyzer.textFont \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::Analyzer {
    inherit itk::Widget

    itk_option define -tool tool Tool ""
    itk_option define -device device Device ""
    itk_option define -analysis analysis Analysis ""
    itk_option define -holdwindow holdWindow HoldWindow ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method simulate {}
    public method reset {}
    public method load {file}

    protected method _fixResult {}

    private variable _run ""           ;# results from last run
    private variable _control "manual" ;# start mode
    private variable _widgets          ;# maps analyze section => widget

    private common job                 ;# array var used for blt::bgexec jobs
}
                                                                                
itk::usual Analyzer {
    keep -background -cursor foreground -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::constructor {args} {
    itk_component add notebook {
        Rappture::Notebook $itk_interior.nb
    }
    pack $itk_interior.nb -expand yes -fill both

    # ------------------------------------------------------------------
    # SIMULATION PAGE
    # ------------------------------------------------------------------
    set w [$itk_component(notebook) insert end simulate]
    frame $w.cntls
    pack $w.cntls -side top -fill x -padx {20 2}

    itk_component add simulate {
        button $w.cntls.sim -text "Simulate" \
            -command [itcl::code $this simulate]
    }
    pack $itk_component(simulate) -side left

    itk_component add status {
        label $w.cntls.info -width 1 -text "" -anchor w
    } {
        usual
        rename -font -textfont textFont Font
    }
    pack $itk_component(status) -side left -expand yes -fill both

    Rappture::Scroller $w.info -xscrollmode off -yscrollmode auto
    pack $w.info -expand yes -fill both -padx {20 2} -pady {20 2}
    itk_component add info {
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

    itk_component add resultselector {
        Rappture::Combobox $w.sel -width 30 -editable no
    } {
        usual
        rename -font -textfont textFont Font
    }
    pack $itk_component(resultselector) -side top -fill x -padx {20 2}
    bind $itk_component(resultselector) <<Value>> [itcl::code $this _fixResult]

    itk_component add results {
        Rappture::Notebook $w.nb
    }
    pack $itk_component(results) -expand yes -fill both -pady 4

    eval itk_initialize $args
    reset
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::destructor {} {
    if {$_run != ""} {
        itcl::delete object $_run
    }
}

# ----------------------------------------------------------------------
# USAGE: simulate
#
# If the simulation page is showing, this kicks off the simulator
# by executing the executable.command associated with the -tool.  While
# the simulation is running, it shows status.  When the simulation is
# finished, it switches automatically to "analyze" mode and shows
# the results.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::simulate {} {
    if {[$itk_component(notebook) current] == "simulate"} {
        $itk_component(status) configure -text "Running simulation..."
        $itk_component(simulate) configure -text "Abort" \
            -command {set ::Rappture::Analyzer::job(control) abort}

        set job(control) ""

        # if the hold window is set, then put up a busy cursor
        if {$itk_option(-holdwindow) != ""} {
            blt::busy hold $itk_option(-holdwindow)
            raise $itk_component(hull)
            update
        }

        # write out the driver.xml file for the tool
        set status [catch {
            set fid [open driver.xml w]
            puts $fid "<?xml version=\"1.0\"?>"
            set xml [$itk_option(-tool) xml]
            set xml2 [$itk_option(-device) xml]
            regsub -all {&} $xml2 {\\\&} xml2
            regsub {</tool>} $xml "$xml2</tool>" xml
            puts $fid $xml
            close $fid
        } result]

        # execute the tool using the path from the tool description
        if {$status == 0} {
            set cmd [$itk_option(-tool) get executable.command]

            set status [catch {eval blt::bgexec \
                ::Rappture::Analyzer::job(control) \
                -output ::Rappture::Analyzer::job(output) \
                -error ::Rappture::Analyzer::job(error) $cmd} result]
        }

        # read back the results from run.xml
        if {$status == 0} {
            set status [catch {load run.xml} result]
        }

        # back to normal
        if {$itk_option(-holdwindow) != ""} {
            blt::busy release $itk_option(-holdwindow)
        }
        $itk_component(status) configure -text ""
        $itk_component(simulate) configure -text "Simulate" \
            -command [itcl::code $this simulate]

        # if anything went wrong, tell the user; otherwise, analyze
        if {[regexp {^KILLED} $job(control)]} {
            # job aborted -- do nothing
        } elseif {$status != 0} {
            $itk_component(info) configure -state normal
            $itk_component(info) delete 1.0 end
            $itk_component(info) insert end "Problem launching job:\n"
            if {[string length $job(error)] > 0} {
                $itk_component(info) insert end $job(error)
            } else {
                $itk_component(info) insert end $result
            }
            $itk_component(info) configure -state disabled
        } else {
            $itk_component(notebook) current analyze
        }
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
    $itk_component(notebook) current simulate

    # if control mode is "auto", then simulate right away
    if {$_control == "auto"} {
        simulate
    }
}

# ----------------------------------------------------------------------
# USAGE: load <file>
#
# Used to reset the analyzer whenever the input to a simulation has
# changed.  Sets the mode back to "simulate", so the user has to
# simulate again to see the output.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::load {file} {
    # clear any old results
    if {$_run != ""} {
        itcl::delete object $_run
        set _run ""
    }

    # try to load new results from the given file
    set _run [Rappture::Library::open $file]

    # go through the analysis and create widgets to display results
    foreach item [array names _widgets] {
        $_widgets($item) configure -run $_run
    }
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
    $itk_component(results) current $page
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -tool
#
# Set to the Rappture::Library object representing the tool being
# run in this analyzer.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Analyzer::tool {
    if {![Rappture::Library::valid $itk_option(-tool)]} {
        error "bad value \"$itk_option(-tool)\": should be Rappture::Library"
    }

    $itk_component(info) configure -state normal
    $itk_component(info) delete 1.0 end
    $itk_component(info) insert end [$itk_option(-tool) get executable.about]
    $itk_component(info) configure -state disabled
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -device
#
# Set to the Rappture::Library object representing the device being
# run in this analyzer.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Analyzer::device {
    if {$itk_option(-device) != ""
          && ![Rappture::Library::valid $itk_option(-device)]} {
        error "bad value \"$itk_option(-device)\": should be Rappture::Library"
    }
    reset
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -analysis
#
# Set to the Rappture::Library object representing the analysis that
# should be shown in this analyzer.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Analyzer::analysis {
    if {![Rappture::Library::valid $itk_option(-analysis)]} {
        error "bad value \"$itk_option(-analysis)\": should be Rappture::Library"
    }
    set _control [$itk_option(-analysis) get control]

    # go through the analysis and create widgets to display results
    $itk_component(results) delete -all
    catch {unset _widgets}

    set counter 0
    foreach item [$itk_option(-analysis) get -children] {
        switch -glob -- $item {
            xyplot* {
                set name "page[incr counter]"
                set label [$itk_option(-analysis) get $item.label]
                if {$label == ""} { set label $name }

                set page [$itk_component(results) insert end $name]
                $itk_component(resultselector) choices insert end \
                    $name $label

                set _widgets($item) [Rappture::Xyplot $page.#auto \
                    -layout [$itk_option(-analysis) get -object $item]]
                pack $_widgets($item) -expand yes -fill both
            }
        }
    }

    # if there is only one page, take down the selector
    if {[$itk_component(resultselector) choices size] <= 1} {
        pack forget $itk_component(resultselector)
    } else {
        pack $itk_component(resultselector) -before $itk_component(results) \
            -side top -fill x -padx {20 2}
    }

    # show the first page by default
    set first [$itk_component(resultselector) choices get -label 0]
    if {$first != ""} {
        $itk_component(results) current page1
        $itk_component(resultselector) value $first
    }
}
