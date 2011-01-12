package require Itk

option add *ResultsPage.width 3.5i widgetDefault
option add *ResultsPage.height 4i widgetDefault
option add *ResultsPage.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *ResultsPage.codeFont \
    -*-courier-medium-r-normal-*-12-* widgetDefault
option add *ResultsPage.textFont \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *ResultsPage.boldTextFont \
    -*-helvetica-bold-r-normal-*-12-* widgetDefault

itcl::class Rappture::ResultsPage {
    inherit itk::Widget

    itk_option define -codefont codeFont Font ""
    itk_option define -textfont textFont Font ""
    itk_option define -boldtextfont boldTextFont Font ""
    itk_option define -clearcommand clearCommand ClearCommand ""
    itk_option define -appname appName AppName ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method load {xmlobj}
    public method clear {}
    public method download {option args}
    public method resultset {}

    protected method _plot {args}
    protected method _fixResult {}
    protected method _fixSize {}
    protected method _resultTooltip {}
    protected method _autoLabel {xmlobj path title cntVar}
    protected method _reorder {comps}

    private variable _pages 0 
    private variable _label2page
    private variable _label2desc
    private variable _runs ""
    private variable _lastlabel ""
    private variable _plotlist ""

    # TODO: ???
    public variable promptcommand ""
}

itk::usual ResultsPage {
    keep -background -cursor -font
    #TODO: keep -background -cursor foreground -font
}

itcl::body Rappture::ResultsPage::constructor {args} {

    itk_component add top {
        frame $itk_interior.top
    }
    pack $itk_component(top) -side top -fill x -pady 8

    itk_component add toplabel {
        label $itk_component(top).l -text "Result:" 
        #TODO: -font $itk_option(font)
    }
    pack $itk_component(toplabel) -side left

    itk_component add resultselector {
        Rappture::Combobox $itk_component(top).sel -width 10 -editable no
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

    $itk_component(resultselector) choices insert end --- "---"

    itk_component add download {
        button $itk_component(top).dl -image [Rappture::icon download] \
            -anchor e -borderwidth 1 -relief flat -overrelief raised \
            -command [itcl::code $this download start $itk_component(top).dl]
    }
    pack $itk_component(download) -side right -padx {4 0}
    bind $itk_component(download) <Enter> [itcl::code $this download coming]

    $itk_component(resultselector) choices insert end \
        @download [Rappture::filexfer::label download]

    if {[Rappture::filexfer::enabled]} {
        Rappture::Tooltip::for $itk_component(download) "Downloads the current result to a new web browser window on your desktop.  From there, you can easily print or save results.

NOTE:  Your web browser must allow pop-ups from this site.  If your output does not appear, look for a 'pop-up blocked' message and enable pop-ups."
    } else {
        Rappture::Tooltip::for $itk_component(download) "Saves the current result to a file on your desktop."
    }

    itk_component add results {
        Rappture::Panes $itk_interior.pane -sashwidth 1 \
            -sashrelief solid -sashpadding {4 0}
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
            -settingscommand [itcl::code $this _plot] 
    }
    pack $itk_component(resultset) -expand yes -fill both
    bind $itk_component(resultset) <<Control>> [itcl::code $this _fixSize]
    bind $itk_component(results) <Configure> [itcl::code $this _fixSize]

    eval itk_initialize $args
}

itcl::body Rappture::ResultsPage::destructor {} {
    foreach obj $_runs {
        itcl::delete object $obj
    }
}
  
itcl::body Rappture::ResultsPage::_fixResult {} {
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

itcl::body Rappture::ResultsPage::download {option args} {
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
                set result [$f.rviewer download now $widget \
                    $itk_option(-appname) $item]
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

itcl::body Rappture::ResultsPage::load {xmlobj} {
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
        set inclobj [Rappture::library $inclfil]
        foreach c [$inclobj children output] {
            switch -glob -- $c {
                # We don't want to include these tags.
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

    # Detect molecule elements that contain trajectory data and convert
    # to sequences.
    # TODO: _trajToSequence $xmlobj output

    # Go through the analysis and find all result sets.
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
            histogram* - curve* - field* - drawing3d* {
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


itcl::body Rappture::ResultsPage::clear {} {
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

    # Invoke -clearcommand option
    if {[string length $itk_option(-clearcommand)] > 0} {
        uplevel #0 $itk_option(-clearcommand)
    }

}

itcl::body Rappture::ResultsPage::_resultTooltip {} {
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

itcl::body Rappture::ResultsPage::_fixSize {} {
    set ht [winfo height $itk_component(results)]
    if {$ht <= 1} { set ht [winfo reqheight $itk_component(results)] }
    set cntlht [$itk_component(resultset) size -controlarea]
    set frac [expr {double($cntlht)/$ht}]

    if {$frac < 0.4} {
        $itk_component(results) fraction end $frac
    }
    #_fixSimControl
}

itcl::body Rappture::ResultsPage::_autoLabel {xmlobj path title cntVar} {
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
# USAGE: _plot ?<index> <options> <index> <options>...?
#
# Used internally to update the plot shown in the current result
# viewer whenever the resultset settings have changed.  Causes the
# desired results to show up on screen.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultsPage::_plot {args} {
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
itcl::body Rappture::ResultsPage::_reorder {comps} {
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

itcl::body Rappture::ResultsPage::resultset {} {
    return $itk_component(resultset)
}

