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
#  Copyright (c) 2004-2005  Purdue Research Foundation
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

    constructor {tool args} { # defined below }
    destructor { # defined below }

    public method simulate {args}
    public method reset {{when -eventually}}
    public method load {xmlobj}
    public method clear {}
    public method download {option args}

    protected method _plot {args}
    protected method _reorder {comps}
    protected method _autoLabel {xmlobj path title cntVar}
    protected method _fixResult {}
    protected method _fixSize {}
    protected method _fixSimControl {}
    protected method _fixNotebook {}
    protected method _simState {state args}
    protected method _simOutput {message}
    protected method _resultTooltip {}

    private variable _tool ""          ;# belongs to this tool
    private variable _appName ""       ;# Name of application
    private variable _control "manual" ;# start mode
    private variable _runs ""          ;# list of XML objects with results
    private variable _pages 0          ;# number of pages for result sets
    private variable _label2page       ;# maps output label => result set
    private variable _label2desc       ;# maps output label => description
    private variable _lastlabel ""     ;# label of last example loaded
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

    itk_component add simbg {
        frame $itk_interior.simol.simbg -borderwidth 0
    } {
        usual
        rename -background -simcontrolcolor simControlColor Color
    }
    pack $itk_component(simbg) -expand yes -fill both

    set simtxt [$tool xml get tool.action.label]
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

    # if there's a hub url, then add "About" and "Questions" links
    set _appName [$_tool xml get tool.id]
    set url [Rappture::Tool::resources -huburl]
    if {"" != $url && "" != $_appName} {
        itk_component add hubcntls {
            frame $itk_component(simbg).hubcntls
        } {
            usual
            rename -background -simcontrolcolor simControlColor Color
        }
        pack $itk_component(hubcntls) -side right -padx 4

        itk_component add icon {
            label $itk_component(hubcntls).icon -image [Rappture::icon ask] \
                -highlightthickness 0
        } {
            usual
            ignore -highlightthickness
            rename -background -simcontrolcolor simControlColor Color
        }
        pack $itk_component(icon) -side left

        itk_component add about {
            button $itk_component(hubcntls).about -text "About this tool" \
                -command [list Rappture::filexfer::webpage \
                              "$url/tools/$_appName"]
        } {
            usual
            ignore -font
            rename -background -simcontrolcolor simControlColor Color
            rename -highlightbackground -simcontrolcolor simControlColor Color
        }
        pack $itk_component(about) -side top -anchor w

        itk_component add questions {
            button $itk_component(hubcntls).questions -text Questions? \
                -command [list Rappture::filexfer::webpage \
                              "$url/resources/$_appName/questions"]
        } {
            usual
            ignore -font
            rename -background -simcontrolcolor simControlColor Color
            rename -highlightbackground -simcontrolcolor simControlColor Color
        }
        pack $itk_component(questions) -side top -anchor w
    }

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

    itk_component add resultselector {
        Rappture::Combobox $w.top.sel -width 10 -editable no
    } {
        usual
        rename -font -textfont textFont Font
    }
    pack $itk_component(resultselector) -side left -expand yes -fill x
    bind $itk_component(resultselector) <<Value>> [itcl::code $this _fixResult]
    bind $itk_component(resultselector) <Enter> \
        [itcl::code $this download coming]

    Rappture::Tooltip::for $itk_component(resultselector) \
        "@[itcl::code $this _resultTooltip]"

    $itk_component(resultselector) choices insert end \
        --- "---"

    itk_component add download {
        button $w.top.dl -image [Rappture::icon download] -anchor e \
            -borderwidth 1 -relief flat -overrelief raised \
            -command [itcl::code $this download start $w.top.dl]
    }
    pack $itk_component(download) -side right -padx {4 0}
    bind $itk_component(download) <Enter> \
        [itcl::code $this download coming]

    $itk_component(resultselector) choices insert end \
        @download [Rappture::filexfer::label download]

    if {[Rappture::filexfer::enabled]} {
        Rappture::Tooltip::for $itk_component(download) "Downloads the current result to a new web browser window on your desktop.  From there, you can easily print or save results.

NOTE:  Your web browser must allow pop-ups from this site.  If your output does not appear, look for a 'pop-up blocked' message and enable pop-ups."
    } else {
        Rappture::Tooltip::for $itk_component(download) "Saves the current result to a file on your desktop."
    }

    itk_component add results {
        Rappture::Panes $w.pane -sashwidth 1 -sashrelief solid -sashpadding {4 0}
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
    bind $itk_component(results) <Configure> [itcl::code $this _fixSize]

    eval itk_initialize $args

    $itk_component(runinfo) tag configure ERROR -foreground red
    $itk_component(runinfo) tag configure text -font $itk_option(-textfont)

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
        if {[$itk_component(resultset) contains [$_tool xml object]]
              && ![string equal $_control "manual-resim"]} {
            # not needed -- show results and return
            $itk_component(notebook) current analyze
            return
        }
        set args ""
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
        $itk_component(runinfo) delete 1.0 end
        $itk_component(runinfo) insert end "Problem launching job:\n\n" text
        _simOutput $result
        $itk_component(runinfo) configure -state disabled
        $itk_component(runinfo) see 1.0

        # Try to create a support ticket for this error.
        # It may be a real problem.
        if {[Rappture::bugreport::shouldReport for jobs]} {
            Rappture::bugreport::register "Problem launching job:\n\n$result\n== RAPPTURE INPUT ==\n[$_tool xml xml]"
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
    if {![$itk_component(resultset) contains [$_tool xml object]]
          || [string equal $_control "manual-resim"]} {
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
    if {[$_tool xml get tool.analyzer] == "last"} {
        clear
    }

    # look for all output.load children and load them first
    # each run.xml is loaded as a previous simulation.
    foreach item [$xmlobj children -type run output.load] {
        set loadfile [$xmlobj get output.load.$item]
        set loadobj [Rappture::library $loadfile]
        load $loadobj
    }

    foreach item [$xmlobj children -type run output.include] {
        set id [$xmlobj element -as id output.include.$item]
        set inclfile [$xmlobj get output.include.$item]
        set inclobj [Rappture::library $inclfile]
        foreach c [$inclobj children output] {
            switch -glob -- $c {
                # we don't want to include these tags
                include* - time* - status* - user* {
                    continue
                }
                default {
                    set oldid [$inclobj element -as id output.$c]
                    set oldtype [$inclobj element -as type output.$c]
                    set newcomp "$oldtype\($id-$oldid\)"
                    $xmlobj copy output.$newcomp from $inclobj output.$c
                }
            }
        }
    }

    lappend _runs $xmlobj

    # go through the analysis and find all result sets
    set haveresults 0
    foreach item [_reorder [$xmlobj children output]] {
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
            string* {
                _autoLabel $xmlobj output.$item "String" counters
            }
            histogram* - curve* - field* {
                _autoLabel $xmlobj output.$item "Plot" counters
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
                    set _label2desc($label) \
                        [$xmlobj get output.$item.about.description]
                    Rappture::ResultViewer $page.rviewer
                    pack $page.rviewer -expand yes -fill both -pady 4

                    set end [$itk_component(resultselector) \
                        choices index -value ---]
                    if {$end < 0} {
                        set end "end"
                    }
                    $itk_component(resultselector) choices insert $end \
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

    # show the first page by default
    set max [$itk_component(resultselector) choices size]
    for {set i 0} {$i < $max} {incr i} {
        set first [$itk_component(resultselector) choices get -label $i]
        if {$first != ""} {
            set page [$itk_component(resultselector) choices get -value $i]
            set char [string index $page 0]
            if {$char != "@" && $char != "-"} {
                $itk_component(resultpages) current $page
                $itk_component(resultselector) value $first
                set _lastlabel $first
                break
            }
        }
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

    # reset the size of the controls area
    set ht [winfo height $itk_component(results)]
    set cntlht [$itk_component(resultset) size -controlarea]
    set frac [expr {double($cntlht)/$ht}]
    $itk_component(results) fraction end $frac

    foreach label [array names _label2page] {
        set page $_label2page($label)
        $page.rviewer clear
    }
    $itk_component(resultselector) value ""
    $itk_component(resultselector) choices delete 0 end
    catch {unset _label2page}
    catch {unset _label2desc}
    set _plotlist ""

    $itk_component(resultselector) choices insert end --- "---"
    $itk_component(resultselector) choices insert end \
        @download [Rappture::filexfer::label download]
    set _lastlabel ""

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
# USAGE: download coming
# USAGE: download controls <downloadCommand>
# USAGE: download start ?widget?
# USAGE: download now ?widget?
#
# Spools the current result so the user can download it.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::download {option args} {
    set title [$itk_component(resultselector) value]
    set page [$itk_component(resultselector) translate $title]

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
                set item [$itk_component(resultselector) value]
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
# viewer whenever the resultset settings have changed.  Causes the
# desired results to show up on screen.
# ----------------------------------------------------------------------
itcl::body Rappture::Analyzer::_plot {args} {
    set _plotlist $args

    set page [$itk_component(resultselector) value]
    set page [$itk_component(resultselector) translate $page]
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
    set name [$itk_component(resultselector) value]
    set page ""
    if {"" != $name} {
        set page [$itk_component(resultselector) translate $name]
    }
    if {$page == "@download"} {
        # put the combobox back to its last value
        $itk_component(resultselector) component entry configure -state normal
        $itk_component(resultselector) component entry delete 0 end
        $itk_component(resultselector) component entry insert end $_lastlabel
        $itk_component(resultselector) component entry configure -state disabled
        # perform the actual download
        download start $itk_component(download)
    } elseif {$page == "---"} {
        # put the combobox back to its last value
        $itk_component(resultselector) component entry configure -state normal
        $itk_component(resultselector) component entry delete 0 end
        $itk_component(resultselector) component entry insert end $_lastlabel
        $itk_component(resultselector) component entry configure -state disabled
    } elseif {$page != ""} {
        set _lastlabel $name
        set win [winfo toplevel $itk_component(hull)]
        blt::busy hold $win
        $itk_component(resultpages) current $page

        set f [$itk_component(resultpages) page $page]
        $f.rviewer plot clear
        eval $f.rviewer plot add $_plotlist
        blt::busy release [winfo toplevel $itk_component(hull)]
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
    set cntlht [$itk_component(resultset) size -controlarea]
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
                    set str [$_tool xml get $path.about.label]
                    if {"" == $str} {
1                        set str [$_tool xml element -as id $path]
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

        $itk_component(simulate) configure -state disabled
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
    while {[regexp -indices \
               {=RAPPTURE-PROGRESS=> *([-+]?[0-9]+) +([^\n]*)(\n|$)} $message \
                match percent mesg]} {

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
    # Break up the remaining lines according to =RAPPTURE-ERROR=> messages.
    # Show errors in a special color.
    #
    $itk_component(runinfo) configure -state normal

    while {[regexp -indices \
               {=RAPPTURE-([a-zA-Z]+)=>([^\n]*)(\n|$)} $message \
                match type mesg]} {

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
    set name [$itk_component(resultselector) value]
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
