# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: ResultSelector - controls for a ResultSet
#
#  This widget displays a collection of results stored in a ResultSet
#  object.  It manages the controls to select and visualize the data.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *ResultSelector.width 4i widgetDefault
option add *ResultSelector.height 4i widgetDefault
option add *ResultSelector.missingData skip widgetDefault
option add *ResultSelector.controlbarBackground gray widgetDefault
option add *ResultSelector.controlbarForeground white widgetDefault
option add *ResultSelector.activeControlBackground #ffffcc widgetDefault
option add *ResultSelector.activeControlForeground black widgetDefault
option add *ResultSelector.controlActiveForeground blue widgetDefault
option add *ResultSelector.toggleBackground gray widgetDefault
option add *ResultSelector.toggleForeground white widgetDefault
option add *ResultSelector.textFont \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *ResultSelector.boldFont \
    -*-helvetica-bold-r-normal-*-12-* widgetDefault

itcl::class Rappture::ResultSelector {
    inherit itk::Widget

    itk_option define -resultset resultSet ResultSet ""
    itk_option define -activecontrolbackground activeControlBackground Background ""
    itk_option define -activecontrolforeground activeControlForeground Foreground ""
    itk_option define -controlactiveforeground controlActiveForeground Foreground ""
    itk_option define -togglebackground toggleBackground Background ""
    itk_option define -toggleforeground toggleForeground Foreground ""
    itk_option define -textfont textFont Font ""
    itk_option define -boldfont boldFont Font ""
    itk_option define -foreground foreground Foreground ""
    itk_option define -settingscommand settingsCommand SettingsCommand ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method activate {column}
    public method size {{what -results}}

    protected method _doClear {what}
    protected method _doSettings {{cmd ""}}
    protected method _control {option args}
    protected method _fixControls {args}
    protected method _fixLayout {args}
    protected method _fixNumResults {}
    protected method _fixSettings {args}
    protected method _fixValue {column why}
    protected method _drawValue {column widget wmax}
    protected method _toggleAll {{column "current"}}
    protected method _getValues {column {which "all"}}
    protected method _getTooltip {role column}
    protected method _getParamDesc {which {index "current"}}
    protected method _log {col}

    private variable _dispatcher ""  ;# dispatchers for !events
    private variable _resultset ""   ;# object containing results
    private variable _active ""      ;# name of active control
    private variable _plotall 0      ;# non-zero => plot all active results
    private variable _layout         ;# info used in _fixLayout
    private variable _counter 0      ;# counter for unique control names
    private variable _settings 0     ;# non-zero => _fixSettings in progress

    private common _cntlInfo         ;# maps column name => control info
}

itk::usual ResultSelector {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    # create a dispatcher for events
    Rappture::dispatcher _dispatcher

    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout \
        [itcl::code $this _fixLayout]

    $_dispatcher register !settings
    $_dispatcher dispatch $this !settings \
        [itcl::code $this _fixSettings]

    # initialize controls info
    set _cntlInfo($this-all) ""

    # initialize layout info
    set _layout(mode) "usual"
    set _layout(active) ""
    set _layout(numcntls) 0

    itk_component add cntls {
        frame $itk_interior.cntls
    } {
        usual
        rename -background -controlbarbackground controlbarBackground Background
        rename -highlightbackground -controlbarbackground controlbarBackground Background
    }
    pack $itk_component(cntls) -fill x -pady {0 2}

    itk_component add clearall {
        button $itk_component(cntls).clearall -text "Clear" -state disabled \
            -padx 1 -pady 1 \
            -relief flat -overrelief raised \
            -command [itcl::code $this _doClear all]
    } {
        usual
        rename -background -controlbarbackground controlbarBackground Background
        rename -foreground -controlbarforeground controlbarForeground Foreground
        rename -highlightbackground -controlbarbackground controlbarBackground Background
    }
    pack $itk_component(clearall) -side right -padx 2 -pady 1
    Rappture::Tooltip::for $itk_component(clearall) \
        "Clears all results collected so far."

    itk_component add clear {
        button $itk_component(cntls).clear -text "Clear One" -state disabled \
            -padx 1 -pady 1 \
            -relief flat -overrelief raised \
            -command [itcl::code $this _doClear current]
    } {
        usual
        rename -background -controlbarbackground controlbarBackground Background
        rename -foreground -controlbarforeground controlbarForeground Foreground
        rename -highlightbackground -controlbarbackground controlbarBackground Background
    }
    pack $itk_component(clear) -side right -padx 2 -pady 1
    Rappture::Tooltip::for $itk_component(clear) \
        "Clears the result that is currently selected."

    itk_component add status {
        label $itk_component(cntls).status -anchor w \
            -text "No results" -padx 0 -pady 0
    } {
        usual
        rename -background -controlbarbackground controlbarBackground Background
        rename -foreground -controlbarforeground controlbarForeground Foreground
        rename -highlightbackground -controlbarbackground controlbarBackground Background
    }
    pack $itk_component(status) -side left -padx 2 -pady {2 0}

    itk_component add dials {
        frame $itk_interior.dials
    }
    pack $itk_component(dials) -expand yes -fill both
    bind $itk_component(dials) <Configure> \
        [list $_dispatcher event -after 10 !layout why resize]

    # create the permanent controls in the "short list" area
    set dials $itk_component(dials)
    frame $dials.bg
    Rappture::Radiodial $dials.dial -valuewidth 0
    Rappture::Tooltip::for $dials.dial \
        "@[itcl::code $this _getTooltip dial active]"

    set fn [option get $itk_component(hull) textFont Font]
    label $dials.all -text "All" -padx 8 \
        -borderwidth 1 -relief raised -font $fn
    Rappture::Tooltip::for $dials.all \
        "@[itcl::code $this _getTooltip all active]"
    bind $dials.all <ButtonRelease> [itcl::code $this _toggleAll]

    frame $dials.labelmore
    label $dials.labelmore.arrow -bitmap [Rappture::icon empty] -borderwidth 0
    pack $dials.labelmore.arrow -side left -fill y
    label $dials.labelmore.name -text "more parameters..." -font $fn \
        -borderwidth 0 -padx 0 -pady 1
    pack $dials.labelmore.name -side left
    label $dials.labelmore.value
    pack $dials.labelmore.value -side left

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::destructor {} {
    # disconnect any existing resultset to stop notifications
    configure -resultset ""
}

# ----------------------------------------------------------------------
# USAGE: activate <column>
#
# Clients use this to activate a particular column in the set of
# controls.  When a column is active, its label is bold and its
# value has a radiodial in the "short list" area.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::activate {column} {
    if {$_resultset ne ""} {
        set allowed [$_resultset diff names]
        if {[lsearch $allowed $column] < 0} {
            error "bad value \"$column\": should be one of [join $allowed {, }]"
        }

        # column is now active
        set _active $column

        # keep track of usage, so we know which controls are popular
        incr _cntlInfo($this-$column-usage)

        # fix controls at next idle point
        $_dispatcher event -idle !layout why data
        $_dispatcher event -idle !settings column $_active
    }
}

# ----------------------------------------------------------------------
# USAGE: size ?-results|-controls|-controlarea?
#
# Returns various measures for the size of this area:
#   -results ....... number of results loaded
#   -controls ...... number of distinct control parameters
#   -controlarea ... minimum size of usable control area, in pixels
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::size {{what -results}} {
    switch -- $what {
        -results {
            if {$_resultset eq ""} {
                return 0
            }
            return [$_resultset size]
        }
        -controls {
            return [expr {[llength [$_resultset diff names]]-1}]
        }
        -controlarea {
            set ht [winfo reqheight $itk_component(cntls)]
            incr ht 2  ;# padding below controls

            set normalLine [font metrics $itk_option(-textfont) -linespace]
            incr normalLine 2  ;# padding
            set boldLine [font metrics $itk_option(-boldfont) -linespace]
            incr boldLine 2  ;# padding

            set numcntls [expr {[llength [$_resultset diff names]]-1}]
            switch -- $numcntls {
                0 - 1 {
                    # 0 = no controls (no data at all)
                    # 1 = run control, but only 1 run so far
                    # add nothing
                }
                default {
                    # non-active controls
                    incr ht [expr {($numcntls-1)*$normalLine}]
                    # active control
                    incr ht $boldLine
                    # dial for active control
                    incr ht [winfo reqheight $itk_component(dials).dial]
                    # padding around active control
                    incr ht 4
                }
            }
            return $ht
        }
        default {
            error "bad option \"$what\": should be -results, -controls, or -controlarea"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _doClear all|current
#
# Invoked automatically when the user presses the "Clear One" or
# "Clear All" buttons.  Clears results from the ResultSet and then
# waits for an event to react to the change.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_doClear {what} {
    if {$_resultset ne ""} {
        switch -- $what {
            current {
                set simnum $_cntlInfo($this-simnum-value)
                foreach xmlobj [$_resultset find simnum $simnum] {
                    $_resultset clear $xmlobj
                }
                Rappture::Logger::log result -clear one
            }
            all {
                $_resultset clear
                Rappture::Logger::log result -clear all
            }
            default { error "bad option \"$what\": should be current or all" }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _doSettings ?<command>?
#
# Used internally whenever the result selection changes to invoke
# the -settingscommand.  This will notify some external widget, which
# with perform the plotting action specified in the <command>.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_doSettings {{cmd ""}} {
    if {[string length $itk_option(-settingscommand)] > 0} {
        uplevel #0 $itk_option(-settingscommand) $cmd
    }
}

# ----------------------------------------------------------------------
# USAGE: _control bind <widget> <column>
# USAGE: _control hilite <state> <column> <panel>
# USAGE: _control load <widget> <column>
#
# Used internally to manage the interactivity of controls.  The "bind"
# operation sets up bindings on the label/value for each control, so
# you can mouse over and click on a control to activate it.  The
# "hilite" operation controls highlighting of the control.  The "load"
# operation loads data into the specified radiodial <widget>.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_control {option args} {
    switch -- $option {
        bind {
            if {[llength $args] != 2} {
                error "wrong # args: should be _control bind widget column"
            }
            set widget [lindex $args 0]
            set col [lindex $args 1]

            set panel [winfo parent $widget]
            if {[string match label* [winfo name $panel]]} {
                set panel [winfo parent $panel]
            }

            bind $widget <Enter> \
                [itcl::code $this _control hilite on $col $panel]
            bind $widget <Leave> \
                [itcl::code $this _control hilite off $col $panel]
            bind $widget <ButtonRelease> "
                [itcl::code $this activate $col]
                [list Rappture::Logger::log result -active $col]
            "
        }
        hilite {
            if {[llength $args] != 3} {
                error "wrong # args: should be _control hilite state column panel"
            }
            if {$_layout(mode) != "usual"} {
                # abbreviated controls? then skip highlighting
                return
            }
            set state [lindex $args 0]
            set col [lindex $args 1]
            set panel [lindex $args 2]

            if {[string index $col 0] == "@"} {
                # handle artificial names like "@more"
                set id [string range $col 1 end]
            } else {
                # get id for ordinary columns
                set id $_cntlInfo($this-$col-id)
            }

            # highlight any non-active entries
            if {$col != $_active} {
                if {$state} {
                    set fg $itk_option(-controlactiveforeground)
                    $panel.label$id.name configure -fg $fg
                    $panel.label$id.value configure -fg $fg
                    $panel.label$id.arrow configure -fg $fg \
                        -bitmap [Rappture::icon rarrow2]
                } else {
                    set fg $itk_option(-foreground)
                    $panel.label$id.name configure -fg $fg
                    $panel.label$id.value configure -fg $fg
                    $panel.label$id.arrow configure -fg $fg \
                        -bitmap [Rappture::icon empty]
                }
            }
        }
        load {
            if {[llength $args] != 2} {
                error "wrong # args: should be _control load widget column"
            }
            set dial [lindex $args 0]
            set col [lindex $args 1]

            set shortlist $itk_component(dials)
            $shortlist.dial configure -variable ""
            $dial clear
            foreach {label val} [_getValues $col all] {
                $dial add $label $val
            }
            $shortlist.dial configure -variable \
                "::Rappture::ResultSelector::_cntlInfo($this-$col-value)" \
                -interactcommand [itcl::code $this _log $col]
        }
        default {
            error "bad option \"$option\": should be bind, hilite, or load"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixControls ?<eventArgs...>?
#
# Called automatically at the idle point after one or more results
# have been added to this result set.  Scans through all existing
# data and updates controls used to select the data.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_fixControls {args} {
    array set eventData $args
    if {![info exists itk_component(dials)]} {
        # no controls? then nothing to fix
        # this happens when the widget is being destroyed
        return
    }
    set shortlist $itk_component(dials)

    # added or cleared something -- update number of results
    _fixNumResults

    if {$_resultset eq ""} {
        return
    }

    # cleared something?
    if {$eventData(op) eq "clear"} {
        if {[$_resultset size] == 0} {
            # cleared everything? then reset controls to initial state
            array unset _cntlInfo $this-*
            set _cntlInfo($this-all) ""
            set _active ""
            set _plotall 0
            $itk_component(dials).all configure -relief raised \
                -background $itk_option(-background) \
                -foreground $itk_option(-foreground)

            # clean up all of the label/value widgets
            foreach w [winfo children $itk_component(dials)] {
                if {[string match {label[0-9]*} [winfo name $w]]} {
                    destroy $w
                }
            }
            set _counter 0

            $_dispatcher event -idle !layout why data

            return
        }

        # cleared a single value? then move to another one
        array set params $eventData(what)

        # clear any currently highlighted result
        _doSettings

        # did the active control go away?  then switch to simnum
        if {[lsearch -exact [$_resultset diff names] $_active] < 0} {
            set _cntlInfo($this-$_active-value) ""
            activate simnum
        }

        # figure out where we were in the active control, and
        # what value we should display now that this was deleted
        if {[info exists params($_active)]} {
            set current $params($_active)
            if {$current eq [$shortlist.dial get -format label current]} {
                # result deleted is the current result
                set vlist [$shortlist.dial get]
                set i [lsearch -exact $vlist $current]
                if {$i >= 0} {
                    if {$i+1 < [llength $vlist]} {
                        set newControlValue [lindex $vlist [expr {$i+1}]]
                    } elseif {$i-1 >= 0} {
                        set newControlValue [lindex $vlist [expr {$i-1}]]
                    }
                }
            }
        }

        if {[info exists newControlValue]} {
            # Set the control to a value we were able to find.
            # Disconnect the current variable, then plug in the
            # new (legal) value, then reload the controls.
            # This will trigger !settings and other adjustments.
            $shortlist.dial configure -variable ""
            set _cntlInfo($this-$_active-value) $newControlValue
            _control load $shortlist.dial $_active
        } else {
            # if all else fails, show solution #1
            set xmlobj0 [lindex [$_resultset find * *] 0]
            set simnum0 [$_resultset get simnum $xmlobj0]
            set _cntlInfo($this-simnum-value) $simnum0
            activate simnum
        }

        # if clearing this dataset changed the controls, then
        # fix the layout
        set numcntls [expr {[llength [$_resultset diff names]]-1}]
        if {$numcntls != $_layout(numcntls)} {
            $_dispatcher event -idle !layout why data
        }
        return
    }

    # must have added something...
    set shortlist $itk_component(dials)
    grid columnconfigure $shortlist 1 -weight 1

    #
    # Scan through all columns in the data and create any
    # controls that just appeared.
    #
    set nadded 0
    foreach col [$_resultset diff names] {
        if {$col eq "xmlobj"} {
            continue  ;# never create a control for this column
        }

        #
        # If this column doesn't have a control yet, then
        # create one.
        #
        if {![info exists _cntlInfo($this-$col-id)]} {
            set tip ""
            if {$col eq "simnum"} {
                set quantity "Simulation"
                set tip "List of all simulations that you have performed so far."
            } else {
                # search for the first XML object with this element defined
                foreach xmlobj [$_resultset find * *] {
                    set quantity [$xmlobj get $col.about.label]
                    set tip [$xmlobj get $col.about.description]
                    if {"" != $quantity} {
                        break
                    }
                }
                if {"" == $quantity && "" != $xmlobj} {
                    set quantity [$xmlobj element -as id $col]
                }
            }

            # Create the controls for the "short list" area.
            set fn $itk_option(-textfont)
            set w $shortlist.label$_counter
            set row [lindex [grid size $shortlist] 1]
            frame $w
            grid $w -row $row -column 1 -sticky ew
            label $w.arrow -bitmap [Rappture::icon empty] -borderwidth 0
            pack $w.arrow -side left -fill y
            _control bind $w.arrow $col

            label $w.name -text $quantity -anchor w \
                -borderwidth 0 -padx 0 -pady 1 -font $fn
            pack $w.name -side left
            bind $w.name <Configure> [itcl::code $this _fixValue $col resize]
            _control bind $w.name $col

            label $w.value -anchor w \
                -borderwidth 0 -padx 0 -pady 1 -font $fn
            pack $w.value -side left
            bind $w.value <Configure> [itcl::code $this _fixValue $col resize]
            _control bind $w.value $col

            Rappture::Tooltip::for $w \
                "@[itcl::code $this _getTooltip label $col]"

            # create a record for this control
            lappend _cntlInfo($this-all) $col
            set _cntlInfo($this-$col-id) $_counter
            set _cntlInfo($this-$col-label) $quantity
            set _cntlInfo($this-$col-tip) $tip
            set _cntlInfo($this-$col-value) ""
            set _cntlInfo($this-$col-usage) 0
            set _cntlInfo($this-$col) ""

            trace add variable _cntlInfo($this-$col-value) write \
                "[itcl::code $this _fixValue $col value]; list"

            incr _counter

            incr nadded
        }

        #
        # Determine the unique values for this column and load
        # them into the control.
        #
        set id $_cntlInfo($this-$col-id)

        if {$col == $_layout(active)} {
            _control load $shortlist.dial $col
        }
    }

    #
    # Activate the most recent control.  If a bunch of controls
    # were just added, then activate the "Simulation" control,
    # since that's the easiest way to page through results.
    #
    set numcntls [expr {[llength [$_resultset diff names]]-1}]
    if {$nadded > 0 || $numcntls != $_layout(numcntls)} {
        if {$numcntls == 2 || $nadded == 1} {
            activate [lindex [$_resultset diff names] end]
        } else {
            activate simnum
        }

        # fix the shortlist layout to show as many controls as we can
        $_dispatcher event -idle !layout why data
    }

    #
    # Set all controls to the settings of the most recent addition.
    # Setting the value slot will trigger the !settings event, which
    # will then fix all other controls to match the one that changed.
    #
    if {[info exists eventData(what)]} {
        set xmlobj $eventData(what)
        set simnum [$_resultset get simnum $xmlobj]
        set _cntlInfo($this-simnum-value) $simnum
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixLayout ?<eventArgs...>?
#
# Called automatically at the idle point after the controls have
# changed, or the size of the window has changed.  Fixes the layout
# so that the active control is displayed, and other recent controls
# are shown above and/or below.  At the very least, we must show the
# "more options..." control.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_fixLayout {args} {
    array set eventdata $args

    set shortlist $itk_component(dials)

    # clear out the short list area
    foreach w [grid slaves $shortlist] {
        grid forget $w
    }

    # reset all labels back to an ordinary font/background
    set fn $itk_option(-textfont)
    set bg $itk_option(-background)
    set fg $itk_option(-foreground)
    foreach col $_cntlInfo($this-all) {
        set id $_cntlInfo($this-$col-id)
        $shortlist.label$id configure -background $bg
        $shortlist.label$id.arrow configure -background $bg \
            -bitmap [Rappture::icon empty]
        $shortlist.label$id.name configure -font $fn -background $bg
        $shortlist.label$id.value configure -background $bg
    }

    # only 1 result? then we don't need any controls
    if {$_resultset eq "" || [$_resultset size] < 2} {
        set _layout(active) $_active

        # let clients know that the layout has changed
        # so they can fix the overall size accordingly
        if {![info exists eventdata(why)] || $eventdata(why) ne "resize"} {
            event generate $itk_component(hull) <<Layout>>
        }

        return
    }

    # compute the number of controls that will fit in the shortlist area
    set dials $itk_component(dials)
    set h [winfo height $dials]
    set normalLine [font metrics $itk_option(-textfont) -linespace]
    set boldLine [font metrics $itk_option(-boldfont) -linespace]
    set active [expr {$boldLine+[winfo reqheight $dials.dial]+4}]

    if {$h < $active+$normalLine} {
        # active control kinda big? then show parameter values only
        set _layout(mode) abbreviated
        set ncntls [expr {int(floor(double($h)/$normalLine))}]
    } else {
        set _layout(mode) usual
        set ncntls [expr {int(floor(double($h-$active)/$normalLine))+1}]
    }

    # find the controls with the most usage
    set order ""
    foreach col [lrange [$_resultset diff names] 1 end] {
        lappend order [list $col $_cntlInfo($this-$col-usage)]
    }
    set order [lsort -integer -decreasing -index 1 $order]

    set mostUsed ""
    if {[llength $order] <= $ncntls} {
        # plenty of space? then show all controls
        foreach item $order {
            lappend mostUsed [lindex $item 0]
        }
    } else {
        # otherwise, limit to the most-used controls
        foreach item [lrange $order 0 [expr {$ncntls-1}]] {
            lappend mostUsed [lindex $item 0]
        }

        # make sure the active control is included
        if {"" != $_active && [lsearch -exact $mostUsed $_active] < 0} {
            set mostUsed [lreplace [linsert $mostUsed 0 $_active] end end]
        }

        # if there are more controls, add the "more parameters..." entry
        if {$ncntls >= 2} {
            set mostUsed [lreplace $mostUsed end end @more]
            set rest [expr {[llength $order]-($ncntls-1)}]
            if {$rest == 1} {
                $dials.labelmore.name configure -text "1 more parameter..."
            } else {
                $dials.labelmore.name configure -text "$rest more parameters..."
            }
        }
    }

    # show controls associated with diffs and put up the radiodial
    # for the "active" column
    set row 0
    foreach col [concat [lrange [$_resultset diff names] 1 end] @more] {
        # this control not on the short list? then ignore it
        if {[lsearch $mostUsed $col] < 0} {
            continue
        }

        if {[string index $col 0] == "@"} {
            set id [string range $col 1 end]
        } else {
            set id $_cntlInfo($this-$col-id)
        }
        grid $shortlist.label$id -row $row -column 1 -sticky ew -padx 4

        if {$col == $_active} {
            if {$_layout(mode) == "usual"} {
                # put the background behind the active control in the shortlist
                grid $shortlist.bg -row $row -rowspan 2 \
                    -column 0 -columnspan 2 -sticky nsew
                lower $shortlist.bg

                # place the All and dial in the shortlist area
                grid $shortlist.all -row $row -rowspan 2 -column 0 \
                    -sticky nsew -padx 2 -pady 2
                grid $shortlist.dial -row [expr {$row+1}] -column 1 \
                    -sticky ew -padx 4
                incr row

                if {$_layout(active) != $_active} {
                    $shortlist.dial configure -variable ""
                    _control load $shortlist.dial $col
                    $shortlist.dial configure -variable \
                      "::Rappture::ResultSelector::_cntlInfo($this-$col-value)"
                    set _layout(active) $_active
                }
            }
        }
        incr row
    }

    # highlight the active control
    if {[info exists _cntlInfo($this-$_active-id)]} {
        set id $_cntlInfo($this-$_active-id)
        set bf $itk_option(-boldfont)
        set fg $itk_option(-activecontrolforeground)
        set bg $itk_option(-activecontrolbackground)

        if {$_layout(mode) == "usual"} {
            $shortlist.label$id configure -background $bg
            $shortlist.label$id.arrow configure -foreground $fg \
                -background $bg -bitmap [Rappture::icon rarrow]
            $shortlist.label$id.name configure -foreground $fg \
                -background $bg -font $bf
            $shortlist.label$id.value configure -foreground $fg \
                -background $bg
            $shortlist.dial configure -background $bg
            $shortlist.bg configure -background $bg

            if {[$shortlist.all cget -relief] == "raised"} {
                $shortlist.all configure -foreground $fg -background $bg
            }
        }
    }

    # let clients know that the layout has changed
    # so they can fix the overall size accordingly
    if {![info exists eventdata(why)] || $eventdata(why) ne "resize"} {
        event generate $itk_component(hull) <<Layout>>
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixNumResults
#
# Used internally to update the number of results displayed near the
# top of this widget.  If there is only 1 result, then there is also
# a single "Clear" button.  If there are no results, the clear button
# is diabled.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_fixNumResults {} {
    set size 0
    if {$_resultset ne ""} {
        set size [$_resultset size]
    }

    switch $size {
        0 {
            $itk_component(status) configure -text "No results"
            $itk_component(clearall) configure -state disabled -text "Clear"
            pack forget $itk_component(clear)
        }
        1 {
            $itk_component(status) configure -text "1 result"
            $itk_component(clearall) configure -state normal -text "Clear"
            pack forget $itk_component(clear)
        }
        default {
            $itk_component(status) configure -text "$size results"
            $itk_component(clearall) configure -state normal -text "Clear All"
            $itk_component(clear) configure -state normal
            pack $itk_component(clear) -side right \
                -after $itk_component(clearall) -padx {0 6}
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixSettings ?<eventArgs...>?
#
# Called automatically at the idle point after a control has changed
# to load new data into the plotting area at the top of this result
# set.  Extracts the current tuple of control values from the control
# area, then finds the corresponding data values.  Loads the data
# by invoking a -settingscommand callback with parameters that
# describe what data should be plotted.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_fixSettings {args} {
    array set eventdata $args
    if {[info exists eventdata(column)]} {
        set changed $eventdata(column)
    } else {
        set changed ""
    }

    if {[info exists _cntlInfo($this-$_active-label)]} {
        lappend params $_cntlInfo($this-$_active-label)
    } else {
        lappend params "???"
    }
    if {$_active == ""} {
        return   ;# nothing active -- don't do anything
    }
    eval lappend params [_getValues $_active all]

    if {$_resultset eq "" || [$_resultset size] == 0} {
        # no data? then do nothing
        return
    } elseif {[$_resultset size] == 1} {
        # only one data set? then plot it
        set xmlobj [$_resultset find * *]
        set simnum [lindex [$_resultset get simnum $xmlobj] 0]

        _doSettings [list \
            $simnum [list -width 2 \
                    -param [_getValues $_active current] \
                    -description [_getParamDesc all] \
              ] \
            params $params \
        ]
        return
    }

    #
    # Find the selected run.  If the run setting changed, then
    # look at its current value.  Otherwise, search the results
    # for a tuple that matches the current settings.
    #
    if {$changed == "xmlobj" || $changed == "simnum"} {
        set xmlobj [$_resultset find simnum $_cntlInfo($this-simnum-value)]
    } else {
        set format ""
        set tuple ""
        foreach col [lrange [$_resultset diff names] 2 end] {
            lappend format $col
            lappend tuple $_cntlInfo($this-$col-value)
        }
        set xmlobj [lindex [$_resultset find $format $tuple] 0]

        if {$xmlobj eq "" && $changed ne ""} {
            #
            # No data for these settings.  Try leaving the next
            # column open, then the next, and so forth, until
            # we find some data.
            #
            # allcols:  foo bar baz qux
            #               ^^^changed
            #
            # search:   baz qux foo
            #
            set val $_cntlInfo($this-$changed-value)
            set allcols [lrange [$_resultset diff names] 2 end]
            set i [lsearch -exact $allcols $changed]
            set search [concat \
                [lrange $allcols [expr {$i+1}] end] \
                [lrange $allcols 0 [expr {$i-1}]] \
            ]
            set nsearch [llength $search]

            for {set i 0} {$i < $nsearch} {incr i} {
                set format $changed
                set tuple [list $val]
                for {set j [expr {$i+1}]} {$j < $nsearch} {incr j} {
                    set col [lindex $search $j]
                    lappend format $col
                    lappend tuple $_cntlInfo($this-$col-value)
                }
                set xmlobj [lindex [$_resultset find $format $tuple] 0]
                if {$xmlobj ne ""} {
                    break
                }
            }
        }
    }

    #
    # If we found a particular run, then load its values into all
    # controls.
    #
    if {$xmlobj ne ""} {
        # stop reacting to value changes
        set _settings 1

        set format [lrange [$_resultset diff names] 2 end]
        if {[llength $format] == 1} {
            set data [list [$_resultset get $format $xmlobj]]
        } else {
            set data [$_resultset get $format $xmlobj]
        }

        foreach col $format val $data {
            set _cntlInfo($this-$col-value) $val
        }

        set simnum [$_resultset get simnum $xmlobj]
        set _cntlInfo($this-simnum-value) $simnum

        # okay, react to value changes again
        set _settings 0
    }

    #
    # Search for tuples matching the current setting and
    # plot them.
    #
    if {$_plotall && $_active eq "simnum"} {
        set format ""
    } else {
        set format ""
        set tuple ""
        foreach col [lrange [$_resultset diff names] 2 end] {
            if {!$_plotall || $col ne $_active} {
                lappend format $col
                lappend tuple $_cntlInfo($this-$col-value)
            }
        }
    }

    if {$format ne ""} {
        set xolist [$_resultset find $format $tuple]
    } else {
        set xolist [$_resultset find * *]
    }

    if {[llength $xolist] > 0} {
        # search for the result for these settings
        set format ""
        set tuple ""
        foreach col [lrange [$_resultset diff names] 2 end] {
            lappend format $col
            lappend tuple $_cntlInfo($this-$col-value)
        }
        set curr [$_resultset find $format $tuple]

        if {[llength $xolist] == 1} {
            # single result -- always use active color
            set xmlobj [lindex $xolist 0]
            set simnum [$_resultset get simnum $xmlobj]
            set plist [list \
                $simnum [list -width 2 \
                         -param [_getValues $_active $xmlobj] \
                         -description [_getParamDesc all $xmlobj] \
                   ] \
                params $params \
            ]
        } else {
            #
            # Get the color for all points according to
            # the color spectrum.
            #
            set plist [list params $params]
            foreach xmlobj $xolist {
                set simnum [$_resultset get simnum $xmlobj]
                if {$xmlobj eq $curr} {
                    lappend plist $simnum [list -width 3 -raise 1 \
                        -param [_getValues $_active $xmlobj] \
                        -description [_getParamDesc all $xmlobj]]
                } else {
                    lappend plist $simnum [list -brightness 0.7 -width 1 \
                        -param [_getValues $_active $xmlobj] \
                        -description [_getParamDesc all $xmlobj]]
                }
            }
        }

        #
        # Load up the matching plots
        #
        _doSettings $plist
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixValue <columnName> <why>
#
# Called automatically whenver a value for a parameter dial changes.
# Updates the interface to display the new value.  The <why> is a
# reason for the change, which may be "resize" (draw old value in
# new size) or "value" (value changed).
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_fixValue {col why} {
    if {[info exists _cntlInfo($this-$col-id)]} {
        set id $_cntlInfo($this-$col-id)

        set widget $itk_component(dials).label$id
        set wmax [winfo width $itk_component(dials).dial]
        if {$wmax <= 1} {
            set wmax [expr {round(0.9*[winfo width $itk_component(cntls)])}]
        }
        _drawValue $col $widget $wmax

        if {$why == "value" && !$_settings} {
            # keep track of usage, so we know which controls are popular
            incr _cntlInfo($this-$col-usage)

            # adjust the settings according to the value in the column
            $_dispatcher event -idle !settings column $col
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _drawValue <columnName> <widget> <widthMax>
#
# Used internally to fix the rendering of a "quantity = value" display.
# If the name/value in <widget> are smaller than <widthMax>, then the
# full "quantity = value" string is displayed.  Otherwise, an
# abbreviated form is displayed.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_drawValue {col widget wmax} {
    set quantity $_cntlInfo($this-$col-label)
    regsub -all {\n} $quantity " " quantity  ;# take out newlines

    set newval $_cntlInfo($this-$col-value)
    regsub -all {\n} $newval " " newval  ;# take out newlines

    set lfont [$widget.name cget -font]
    set vfont [$widget.value cget -font]

    set wn [font measure $lfont $quantity]
    set wv [font measure $lfont " = $newval"]
    set w [expr {$wn + $wv}]

    if {$w <= $wmax} {
        # if the text fits, then shown "quantity = value"
        $widget.name configure -text $quantity
        $widget.value configure -text " = $newval"
    } else {
        # Otherwise, we'll have to appreviate.
        # If the value is really long, then just show a little bit
        # of it.  Otherwise, show as much of the value as we can.
        if {[string length $newval] > 30} {
            set frac 0.8
        } else {
            set frac 0.2
        }
        set wNameSpace [expr {round($frac*$wmax)}]
        set wValueSpace [expr {$wmax-$wNameSpace}]

        # fit as much of the "quantity" label in the space available
        if {$wn < $wNameSpace} {
            $widget.name configure -text $quantity
            set wValueSpace [expr {$wmax-$wn}]
        } else {
            set wDots [font measure $lfont "..."]
            set wchar [expr {double($wn)/[string length $quantity]}]
            while {1} {
                # figure out a good size for the abbreviated string
                set cmax [expr {round(($wNameSpace-$wDots)/$wchar)}]
                if {$cmax < 0} {set cmax 0}
                set str "[string range $quantity 0 $cmax]..."
                if {[font measure $lfont $str] <= $wNameSpace
                      || $wDots >= $wNameSpace} {
                    break
                }
                # we're measuring with average chars, so we may have
                # to shave a little off and do this again
                set wDots [expr {$wDots+2*$wchar}]
            }
            $widget.name configure -text $str
            set wValueSpace [expr {$wmax-[font measure $lfont $str]}]
        }

        if {$wv < $wValueSpace} {
            $widget.value configure -text " = $newval"
        } else {
            set wDots [font measure $vfont "..."]
            set wEq [font measure $vfont " = "]
            set wchar [expr {double($wv)/[string length " = $newval"]}]
            while {1} {
                # figure out a good size for the abbreviated string
                set cmax [expr {round(($wValueSpace-$wDots-$wEq)/$wchar)}]
                if {$cmax < 0} {set cmax 0}
                set str " = [string range $newval 0 $cmax]..."
                if {[font measure $vfont $str] <= $wValueSpace
                      || $wDots >= $wValueSpace} {
                    break
                }
                # we're measuring with average chars, so we may have
                # to shave a little off and do this again
                set wDots [expr {$wDots+2*$wchar}]
            }
            $widget.value configure -text $str
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _toggleAll ?<columnName>?
#
# Called automatically whenever the user clicks on an "All" button.
# Toggles the button between its on/off states.  In the "on" state,
# all results associated with the current control are sent to the
# result viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_toggleAll {{col "current"}} {
    if {$col == "current"} {
        set col $_active
    }
    if {![info exists _cntlInfo($this-$col-id)]} {
        return
    }
    set id $_cntlInfo($this-$col-id)
    set sbutton $itk_component(dials).all
    set current [$sbutton cget -relief]

    if {$current == "sunken"} {
        $sbutton configure -relief raised \
            -background $itk_option(-activecontrolbackground) \
            -foreground $itk_option(-activecontrolforeground)
        set _plotall 0
        Rappture::Logger::log result -all off
    } else {
        $sbutton configure -relief sunken \
            -background $itk_option(-togglebackground) \
            -foreground $itk_option(-toggleforeground)
        set _plotall 1
        Rappture::Logger::log result -all on

        if {$col != $_active} {
            # clicked on an inactive "All" button? then activate that column
            activate $col
        }
    }
    $_dispatcher event -idle !settings
}

# ----------------------------------------------------------------------
# USAGE: _getValues <column> ?<which>?
#
# Returns one or more value names associated with the given <column>.
# If the <which> parameter is "all", then it returns values for all
# results in the ResultSet.  Each value appears as two values in the
# flattened list: the normalized value and the value label.  If the
# <which> parameter is "current"
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_getValues {col {which "all"}} {
    switch -- $which {
        current {
            set simnum $_cntlInfo($this-simnum-value)
            set xmlobj [$_resultset find simnum $simnum]
            if {$xmlobj ne ""} {
                return [$_resultset diff values $col $xmlobj]
            }
            return ""
        }
        all {
            return [$_resultset diff values $col all]
        }
        default {
            return [$_resultset diff values $col $which]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _getTooltip <role> <column>
#
# Called automatically whenever the user hovers on a control within
# this widget.  Returns the tooltip associated with the control.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_getTooltip {role column} {
    set label ""
    set tip ""
    if {$column eq "active"} {
        set column $_active
    }
    if {[info exists _cntlInfo($this-$column-label)]} {
        set label $_cntlInfo($this-$column-label)
    }
    if {[info exists _cntlInfo($this-$column-tip)]} {
        set tip $_cntlInfo($this-$column-tip)
    }

    switch -- $role {
        label {
            if {$column ne $_active} {
                append tip "\n\nClick to activate this control."
            }
        }
        dial {
            append tip "\n\nClick to change the value of this parameter."
        }
        all {
            if {$label eq ""} {
                set tip "Plot all values for this quantity."
            } else {
                set tip "Plot all values for $label."
            }
            if {$_plotall} {
                set what "all values"
            } else {
                set what "one value"
            }
            append tip "\n\nCurrently, plotting $what.  Click to toggle."
        }
    }
    return [string trim $tip]
}

# ----------------------------------------------------------------------
# USAGE: _getParamDesc <which> ?<xmlobj>?
#
# Used internally to build a descripton of parameters for the data
# tuple for the specified <xmlobj>.  This is passed on to the underlying
# results viewer, so it will know what data is being viewed.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_getParamDesc {which {xmlobj "current"}} {
    if {$_resultset eq ""} {
        return ""
    }

    if {$xmlobj eq "current"} {
        # search for the result for these settings
        set format ""
        set tuple ""
        foreach col [lrange [$_resultset diff names] 2 end] {
            lappend format $col
            lappend tuple $_cntlInfo($this-$col-value)
        }
        set xmlobj [$_resultset find $format $tuple]
        if {$xmlobj eq ""} {
            return ""  ;# somethings wrong -- bail out!
        }
    }

    switch -- $which {
        active {
            if {$_active eq ""} {
                return ""
            }
        }
        all {
            set desc ""
            foreach col [lrange [$_resultset diff names] 1 end] {
                set quantity $_cntlInfo($this-$col-label)
                set val [$_resultset get $col $xmlobj]
                append desc "$quantity = $val\n"
            }
            return [string trim $desc]
        }
        default {
            error "bad value \"$which\": should be active or all"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _log <col>
#
# Used internally to log the event when a user switches to a different
# result in the result selector.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSelector::_log {col} {
    Rappture::Logger::log result -select $col $_cntlInfo($this-$col-value)
}

# ----------------------------------------------------------------------
# OPTION: -resultset
# ----------------------------------------------------------------------
itcl::configbody Rappture::ResultSelector::resultset {
    set obj $itk_option(-resultset)
    if {$obj ne ""} {
        if {[catch {$obj isa ::Rappture::ResultSet} valid] || !$valid} {
            error "bad value \"$obj\": should be Rappture::ResultSet object"
        }
    }

    # disconnect the existing ResultSet and install the new one
    if {$_resultset ne ""} {
        $_resultset notify remove $this
        _fixControls op clear
    }
    set _resultset $obj

    if {$_resultset ne ""} {
        $_resultset notify add $this !change \
            [itcl::code $this _fixControls]
    }

    _fixControls op add
    activate simnum
}

# ----------------------------------------------------------------------
# OPTION: -activecontrolbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::ResultSelector::activecontrolbackground {
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# OPTION: -activecontrolforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::ResultSelector::activecontrolforeground {
    $_dispatcher event -idle !layout
}

