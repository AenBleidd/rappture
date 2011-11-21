
# ----------------------------------------------------------------------
#  COMPONENT: ResultSet - controls for a collection of related results
#
#  This widget stores a collection of results that all represent
#  the same quantity, but for various ranges of input values.
#  It also manages the controls to select and visualize the data.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *ResultSet.width 4i widgetDefault
option add *ResultSet.height 4i widgetDefault
option add *ResultSet.missingData skip widgetDefault
option add *ResultSet.controlbarBackground gray widgetDefault
option add *ResultSet.controlbarForeground white widgetDefault
option add *ResultSet.activeControlBackground #ffffcc widgetDefault
option add *ResultSet.activeControlForeground black widgetDefault
option add *ResultSet.controlActiveForeground blue widgetDefault
option add *ResultSet.toggleBackground gray widgetDefault
option add *ResultSet.toggleForeground white widgetDefault
option add *ResultSet.textFont \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *ResultSet.boldFont \
    -*-helvetica-bold-r-normal-*-12-* widgetDefault

itcl::class Rappture::ResultSet {
    inherit itk::Widget

    itk_option define -activecontrolbackground activeControlBackground Background ""
    itk_option define -activecontrolforeground activeControlForeground Foreground ""
    itk_option define -controlactiveforeground controlActiveForeground Foreground ""
    itk_option define -togglebackground toggleBackground Background ""
    itk_option define -toggleforeground toggleForeground Foreground ""
    itk_option define -textfont textFont Font ""
    itk_option define -boldfont boldFont Font ""
    itk_option define -foreground foreground Foreground ""
    itk_option define -missingdata missingData MissingData ""
    itk_option define -clearcommand clearCommand ClearCommand ""
    itk_option define -settingscommand settingsCommand SettingsCommand ""
    itk_option define -promptcommand promptCommand PromptCommand ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {xmlobj}
    public method clear {}
    public method activate {column}
    public method contains {xmlobj}
    public method size {{what -results}}

    protected method _doClear {}
    protected method _doSettings {{cmd ""}}
    protected method _doPrompt {state}
    protected method _control {option args}
    protected method _fixControls {args}
    protected method _fixLayout {args}
    protected method _fixSettings {args}
    protected method _fixExplore {}
    protected method _fixValue {column why}
    protected method _drawValue {column widget wmax}
    protected method _toggleAll {{column "current"}}
    protected method _getValues {column {which ""}}
    protected method _getTooltip {role column}
    protected method _getParamDesc {which {index "current"}}

    private variable _dispatcher ""  ;# dispatchers for !events
    private variable _results ""     ;# tuple of known results
    private variable _recent ""      ;# most recent result in _results
    private variable _active ""      ;# column with active control
    private variable _plotall 0      ;# non-zero => plot all active results
    private variable _layout         ;# info used in _fixLayout
    private variable _counter 0      ;# counter for unique control names
    private variable _settings 0     ;# non-zero => _fixSettings in progress
    private variable _explore 0      ;# non-zero => explore all parameters

    private common _cntlInfo         ;# maps column name => control info
}
                                                                                
itk::usual ResultSet {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    # create a dispatcher for events
    Rappture::dispatcher _dispatcher
    $_dispatcher register !fixcntls
    $_dispatcher dispatch $this !fixcntls \
        [itcl::code $this _fixControls]
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

    # create a list of tuples for data
    set _results [Rappture::Tuples ::#auto]
    $_results column insert end -name xmlobj -label "top-level XML object"


    itk_component add cntls {
        frame $itk_interior.cntls
    } {
        usual
        rename -background -controlbarbackground controlbarBackground Background
        rename -highlightbackground -controlbarbackground controlbarBackground Background
    }
    pack $itk_component(cntls) -fill x -pady {0 2}

    itk_component add clear {
        button $itk_component(cntls).clear -text "Clear" -state disabled \
            -padx 1 -pady 1 \
            -relief flat -overrelief raised \
            -command [itcl::code $this _doClear]
    } {
        usual
        rename -background -controlbarbackground controlbarBackground Background
        rename -foreground -controlbarforeground controlbarForeground Foreground
        rename -highlightbackground -controlbarbackground controlbarBackground Background
    }
    pack $itk_component(clear) -side right -padx 2 -pady 1
    Rappture::Tooltip::for $itk_component(clear) \
        "Clears all results collected so far."

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

    itk_component add parameters {
        button $itk_component(cntls).params -text "Parameters..." \
            -state disabled -padx 1 -pady 1 \
            -relief flat -overrelief raised \
            -command [list $itk_component(hull).popup activate $itk_component(cntls).params above]
    } {
        usual
        rename -background -controlbarbackground controlbarBackground Background
        rename -foreground -controlbarforeground controlbarForeground Foreground
        rename -highlightbackground -controlbarbackground controlbarBackground Background
    }
    pack $itk_component(parameters) -side left -padx 8 -pady 1
    Rappture::Tooltip::for $itk_component(parameters) \
        "Click to access all parameters."

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
    _control bind $dials.labelmore.arrow @more
    label $dials.labelmore.name -text "more parameters..." -font $fn \
        -borderwidth 0 -padx 0 -pady 1
    pack $dials.labelmore.name -side left
    label $dials.labelmore.value
    pack $dials.labelmore.value -side left
    _control bind $dials.labelmore.name @more
    Rappture::Tooltip::for $dials.labelmore \
        "@[itcl::code $this _getTooltip more more]"

    # use this pop-up for access to all controls
    Rappture::Balloon $itk_component(hull).popup \
        -title "Change Parameters" -padx 0 -pady 0
    set inner [$itk_component(hull).popup component inner]

    frame $inner.cntls
    pack $inner.cntls -side bottom -fill x
    frame $inner.cntls.sep -height 2 -borderwidth 1 -relief sunken
    pack $inner.cntls.sep -side top -fill x -padx 4 -pady 4
    checkbutton $inner.cntls.explore -font $fn \
        -text "Explore combinations with no results" \
        -variable [itcl::scope _explore] \
        -command [itcl::code $this _fixExplore]
    pack $inner.cntls.explore -side top -anchor w
    Rappture::Tooltip::for $inner.cntls.explore \
        "When this option is turned on, you can set parameters to various combinations that have not yet been simulated.  The Simulate button will light up, and you can simulate these missing combinations.\n\nWhen turned off, controls will avoid missing combinations, and automatically snap to the closest available dataset."

    itk_component add options {
        Rappture::Scroller $inner.scrl -xscrollmode auto -yscrollmode auto
    }
    pack $itk_component(options) -expand yes -fill both

    set popup [$itk_component(options) contents frame]
    frame $popup.bg

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::destructor {} {
    itcl::delete object $_results
}

# ----------------------------------------------------------------------
# USAGE: add <xmlobj>
#
# Adds a new result to this result set.  Scans through all existing
# results to look for a difference compared to previous results.
# Returns the index of this new result to the caller.  The various
# data objects for this result set should be added to their result
# viewers at the same index.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::add {xmlobj} {
    # make sure we fix up controls at some point
    $_dispatcher event -idle !fixcntls

    #
    # If this is the first result, then there are no diffs.
    # Add it right in.
    #
    set xmlobj0 [$_results get -format xmlobj end]
    if {"" == $xmlobj0} {
        # first element -- always add
        $_results insert end [list $xmlobj]
        set _recent $xmlobj
        $itk_component(status) configure -text "1 result"
        $itk_component(clear) configure -state normal
        if {[$_results size] >= 2} {
            $itk_component(parameters) configure -state normal
        } else {
            $itk_component(parameters) configure -state disabled
        }
        return 0
    }

    #
    # Compare this new object against the last XML object in the
    # results set.  If it has a difference, make sure that there
    # is a column to represent the quantity with the difference.
    #
    foreach {op vpath oldval newval} [$xmlobj0 diff $xmlobj] {
        if {[$xmlobj get $vpath.about.diffs] == "ignore"} {
            continue
        }
        if {$op == "+" || $op == "-"} {
            # ignore differences where parameters come and go
            # such differences make it hard to work controls
            continue
        }

        # make sure that these values really are different
        set oldval [lindex [Rappture::LibraryObj::value $xmlobj0 $vpath] 0]
        set newval [lindex [Rappture::LibraryObj::value $xmlobj $vpath] 0]

        if {$oldval != $newval && [$_results column names $vpath] == ""} {
            # no column for this quantity yet
            $_results column insert end -name $vpath -default $oldval
        }
    }

    # build a tuple for this new object
    set cols ""
    set tuple ""
    foreach col [lrange [$_results column names] 1 end] {
        lappend cols $col
        set raw [lindex [Rappture::LibraryObj::value $xmlobj $col] 0]
        lappend tuple $raw  ;# use the "raw" (user-readable) label
    }

    # find a matching tuple? then replace it -- only need one
    if {[llength $cols] > 0} {
        set ilist [$_results find -format $cols -- $tuple]
    } else {
        set ilist 0  ;# no diffs -- must match first entry
    }

    # add all remaining columns for this new entry
    set tuple [linsert $tuple 0 $xmlobj]

    if {[llength $ilist] > 0} {
        if {[llength $ilist] > 1} {
            error "why so many matching results?"
        }

        # overwrite the first matching entry
        set index [lindex $ilist 0]
        $_results put $index $tuple
        set _recent $xmlobj
    } else {
        set index [$_results size]
        $_results insert end $tuple
        set _recent $xmlobj
    }

    if {[$_results size] == 1} {
        $itk_component(status) configure -text "1 result"
    } else {
        $itk_component(status) configure -text "[$_results size] results"
        $itk_component(parameters) configure -state normal
    }
    $itk_component(clear) configure -state normal

    return $index
}

# ----------------------------------------------------------------------
# USAGE: clear
#
# Clears all results in this result set.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::clear {} {
    _doSettings

    # delete all adjuster controls
    set popup [$itk_component(options) contents frame]
    set shortlist $itk_component(dials)
    foreach col $_cntlInfo($this-all) {
        set id $_cntlInfo($this-$col-id)
        destroy $popup.label$id $popup.dial$id $popup.all$id
        destroy $shortlist.label$id
    }

    # clean up control info
    foreach key [array names _cntlInfo $this-*] {
        catch {unset _cntlInfo($key)}
    }
    set _cntlInfo($this-all) ""
    set _counter 0

    # clear out all results
    $_results delete 0 end
    eval $_results column delete [lrange [$_results column names] 1 end]
    set _recent ""
    set _active ""

    set _plotall 0
    $itk_component(dials).all configure -relief raised \
        -background $itk_option(-background) \
        -foreground $itk_option(-foreground)

    # update status and Clear button
    $itk_component(status) configure -text "No results"
    $itk_component(parameters) configure -state disabled
    $itk_component(clear) configure -state disabled
    $_dispatcher event -idle !fixcntls

    # let clients know that the number of controls has changed
    event generate $itk_component(hull) <<Control>>
}

# ----------------------------------------------------------------------
# USAGE: activate <column>
#
# Clients use this to activate a particular column in the set of
# controls.  When a column is active, its label is bold and its
# value has a radiodial in the "short list" area.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::activate {column} {
    if {$column == "@more"} {
        $itk_component(hull).popup activate \
            $itk_component(dials).labelmore.name above
        return
    }

    set allowed [$_results column names]
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

# ----------------------------------------------------------------------
# USAGE: contains <xmlobj>
#
# Checks to see if the given <xmlobj> is already represented by
# some result in this result set.  This comes in handy when checking
# to see if an input case is already covered.
#
# Returns 1 if the result set already contains this result, and
# 0 otherwise.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::contains {xmlobj} {
    # no results? then this must be new
    if {[$_results size] == 0} {
        return 0
    }

    #
    # Compare this new object against the last XML object in the
    # results set.  If it has a difference, make sure that there
    # is a column to represent the quantity with the difference.
    #
    set xmlobj0 [$_results get -format xmlobj end]
    foreach {op vpath oldval newval} [$xmlobj0 diff $xmlobj] {
        if {[$xmlobj get $vpath.about.diffs] == "ignore"} {
            continue
        }
        if {$op == "+" || $op == "-"} {
            # ignore differences where parameters come and go
            # such differences make it hard to work controls
            continue
        }
        if {[$_results column names $vpath] == ""} {
            # no column for this quantity yet
            return 0
        }
    }

    #
    # If we got this far, then look through existing results for
    # matching tuples, then check each one for diffs.
    #
    set format ""
    set tuple ""
    foreach col [lrange [$_results column names] 1 end] {
        lappend format $col
        set raw [lindex [Rappture::LibraryObj::value $xmlobj $col] 0]
        lappend tuple $raw  ;# use the "raw" (user-readable) label
    }
    if {[llength $format] > 0} {
        set ilist [$_results find -format $format -- $tuple]
    } else {
        set ilist 0  ;# no diffs -- must match first entry
    }

    foreach i $ilist {
        set xmlobj0 [$_results get -format xmlobj $i]
        set diffs [$xmlobj0 diff $xmlobj]
        if {[llength $diffs] == 0} {
            # no diffs -- already contained here
            return 1
        }
    }

    # must be some differences
    return 0
}


# ----------------------------------------------------------------------
# USAGE: size ?-results|-controls|-controlarea?
#
# Returns various measures for the size of this area:
#   -results ....... number of results loaded
#   -controls ...... number of distinct control parameters
#   -controlarea ... minimum size of usable control area, in pixels
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::size {{what -results}} {
    switch -- $what {
        -results {
            return [$_results size]
        }
        -controls {
            return [llength $_cntlInfo($this-all)]
        }
        -controlarea {
            set ht [winfo reqheight $itk_component(cntls)]
            incr ht 2  ;# padding below controls

            set normalLine [font metrics $itk_option(-textfont) -linespace]
            incr normalLine 2  ;# padding
            set boldLine [font metrics $itk_option(-boldfont) -linespace]
            incr boldLine 2  ;# padding

            set numcntls [llength $_cntlInfo($this-all)]
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
# USAGE: _doClear
#
# Invoked automatically when the user presses the Clear button.
# Invokes the -clearcommand to clear all data from this resultset
# and all other resultsets in an Analyzer.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_doClear {} {
    if {[string length $itk_option(-clearcommand)] > 0} {
        uplevel #0 $itk_option(-clearcommand)
    }
}

# ----------------------------------------------------------------------
# USAGE: _doSettings ?<command>?
#
# Used internally whenever the result selection changes to invoke
# the -settingscommand.  This will notify some external widget, which
# with perform the plotting action specified in the <command>.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_doSettings {{cmd ""}} {
    if {[string length $itk_option(-settingscommand)] > 0} {
        uplevel #0 $itk_option(-settingscommand) $cmd
    }
}

# ----------------------------------------------------------------------
# USAGE: _doPrompt <state>
#
# Used internally whenever the current settings represent a point
# with no data.  Invokes the -promptcommand with an explanation of
# the missing data, prompting the user to simulate it.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_doPrompt {state} {
    if {[string length $itk_option(-promptcommand)] > 0} {
        if {$state} {
            set message "No data for these settings"
            set settings ""
            foreach col [lrange [$_results column names] 1 end] {
                set val $_cntlInfo($this-$col-value)
                lappend settings $col $val
            }
            uplevel #0 $itk_option(-promptcommand) [list on $message $settings]
        } else {
            uplevel #0 $itk_option(-promptcommand) off
        }
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
itcl::body Rappture::ResultSet::_control {option args} {
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
            bind $widget <ButtonRelease> [itcl::code $this activate $col]
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

            $dial clear
            foreach {label val} [_getValues $col all] {
                $dial add $label $val
            }
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
itcl::body Rappture::ResultSet::_fixControls {args} {
    if {[$_results size] == 0} {
        return
    }

    set popup [$itk_component(options) contents frame]
    grid columnconfigure $popup 0 -minsize 16
    grid columnconfigure $popup 1 -weight 1

    set shortlist $itk_component(dials)
    grid columnconfigure $shortlist 1 -weight 1

    #
    # Scan through all columns in the data and create any
    # controls that just appeared.
    #
    $shortlist.dial configure -variable ""

    set nadded 0
    foreach col [$_results column names] {
        set xmlobj [$_results get -format xmlobj 0]

        #
        # If this column doesn't have a control yet, then
        # create one.
        #
        if {![info exists _cntlInfo($this-$col-id)]} {
            set row [lindex [grid size $popup] 1]
            set row2 [expr {$row+1}]

            set tip ""
            if {$col == "xmlobj"} {
                set quantity "Simulation"
                set tip "List of all simulations that you have performed so far."
            } else {
                # search for the first XML object with this element defined
                foreach xmlobj [$_results get -format xmlobj] {
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

            #
            # Build the main control in the pop-up panel.
            #
            set fn $itk_option(-textfont)
            set w $popup.label$_counter
            frame $w
            grid $w -row $row -column 2 -sticky ew -padx 4 -pady {4 0}
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

            set w $popup.dial$_counter
            Rappture::Radiodial $w -valuewidth 0
            grid $w -row $row2 -column 2 -sticky ew -padx 4 -pady {0 4}
            $w configure -variable ::Rappture::ResultSet::_cntlInfo($this-$col-value)
            Rappture::Tooltip::for $w \
                "@[itcl::code $this _getTooltip dial $col]"

            set w $popup.all$_counter
            label $w -text "All" -padx 8 \
                -borderwidth 1 -relief raised -font $fn
            grid $w -row $row -rowspan 2 -column 1 -sticky nsew -padx 2 -pady 4
            Rappture::Tooltip::for $w \
                "@[itcl::code $this _getTooltip all $col]"
            bind $w <ButtonRelease> [itcl::code $this _toggleAll $col]

            # Create the controls for the "short list" area.
            set w $shortlist.label$_counter
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

            # if this is the "Simulation #" control, add a separator
            if {$col == "xmlobj"} {
                grid $popup.all$_counter -column 0
                grid $popup.label$_counter -column 1 -columnspan 2
                grid $popup.dial$_counter -column 1 -columnspan 2

                if {![winfo exists $popup.sep]} {
                    frame $popup.sep -height 1 -borderwidth 0 -background black
                }
                grid $popup.sep -row [expr {$row+2}] -column 0 \
                    -columnspan 3 -sticky ew -pady 4

                if {![winfo exists $popup.paraml]} {
                    label $popup.paraml -text "Parameters:" -font $fn
                }
                grid $popup.paraml -row [expr {$row+3}] -column 0 \
                    -columnspan 3 -sticky w -padx 4 -pady {0 4}
            }

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

            # fix the shortlist layout to show as many controls as we can
            $_dispatcher event -now !layout why data

            # let clients know that a new control appeared
            # so they can fix the overall size accordingly
            event generate $itk_component(hull) <<Control>>

            incr nadded
        }

        #
        # Determine the unique values for this column and load
        # them into the control.
        #
        set id $_cntlInfo($this-$col-id)
        set popup [$itk_component(options) contents frame]
        set dial $popup.dial$id

        _control load $popup.dial$id $col

        if {$col == $_layout(active)} {
            _control load $shortlist.dial $col
            $shortlist.dial configure -variable \
                "::Rappture::ResultSet::_cntlInfo($this-$col-value)"
        }
    }

    #
    # Activate the most recent control.  If a bunch of controls
    # were just added, then activate the "Simulation" control,
    # since that's the easiest way to page through results.
    #
    if {$nadded > 0} {
        if {[$_results column names] == 2 || $nadded == 1} {
            activate [lindex $_cntlInfo($this-all) end]
        } else {
            activate xmlobj
        }
    }

    #
    # Set all controls to the settings of the most recent addition.
    # Setting the value slot will trigger the !settings event, which
    # will then fix all other controls to match the one that changed.
    #
    if {"" != $_recent} {
        set raw [lindex [$_results find -format xmlobj $_recent] 0]
        set raw "#[expr {$raw+1}]"
        set _cntlInfo($this-xmlobj-value) $raw
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixLayout ?<eventArgs...>?
#
# Called automatically at the idle point after the controls have
# changed, or the size of the window has changed.  Fixes the layout
# so that the active control is displayed, and other recent controls
# are shown above and/or below.  At the very least, we must show the
# "more options..." control, which pops up a panel of all controls.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_fixLayout {args} {
    array set eventdata $args

    set popup [$itk_component(options) contents frame]
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
        $popup.label$id configure -background $bg
        $popup.label$id.arrow configure -background $bg \
            -bitmap [Rappture::icon empty]
        $popup.label$id.name configure -font $fn -background $bg
        $popup.label$id.value configure -background $bg
        $popup.all$id configure -background $bg -foreground $fg \
            -relief raised
        $popup.dial$id configure -background $bg
        $shortlist.label$id configure -background $bg
        $shortlist.label$id.arrow configure -background $bg \
            -bitmap [Rappture::icon empty]
        $shortlist.label$id.name configure -font $fn -background $bg
        $shortlist.label$id.value configure -background $bg
    }

    # only 1 result? then we don't need any controls
    if {[$_results size] < 2} {
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
    foreach col $_cntlInfo($this-all) {
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
        if {$ncntls > 2} {
            set mostUsed [lreplace $mostUsed end end @more]
            set rest [expr {[llength $order]-($ncntls-1)}]
            if {$rest == 1} {
                $dials.labelmore.name configure -text "1 more parameter..."
            } else {
                $dials.labelmore.name configure -text "$rest more parameters..."
            }
        }
    }

    # draw the active control
    set row 0
    foreach col [concat $_cntlInfo($this-all) @more] {
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
            # put the background behind the active control in the popup
            set id $_cntlInfo($this-$_active-id)
            array set ginfo [grid info $popup.label$id]
            grid $popup.bg -row $ginfo(-row) -rowspan 2 \
                -column 0 -columnspan 3 -sticky nsew
            lower $popup.bg

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
                        "::Rappture::ResultSet::_cntlInfo($this-$col-value)"
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

        $popup.label$id configure -background $bg
        $popup.label$id.arrow configure -foreground $fg -background $bg \
            -bitmap [Rappture::icon rarrow]
        $popup.label$id.name configure -foreground $fg -background $bg \
            -font $bf
        $popup.label$id.value configure -foreground $fg -background $bg
        $popup.dial$id configure -background $bg
        $popup.bg configure -background $bg

        if {$_plotall} {
            $popup.all$id configure -relief sunken \
                -background $itk_option(-togglebackground) \
                -foreground $itk_option(-toggleforeground)
        } else {
            $popup.all$id configure -relief raised \
                -background $itk_option(-activecontrolbackground) \
                -foreground $itk_option(-activecontrolforeground)
        }

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
itcl::body Rappture::ResultSet::_fixSettings {args} {
    array set eventdata $args
    if {[info exists eventdata(column)]} {
        set changed $eventdata(column)
    } else {
        set changed ""
    }
    _doPrompt off

    if {[info exists _cntlInfo($this-$_active-label)]} {
        lappend params $_cntlInfo($this-$_active-label)
    } else {
        lappend params "???"
    }
    if { $_active == "" } {
	return;				# Nothing active. Don't do anything.
    }
    eval lappend params [_getValues $_active all]

    switch -- [$_results size] {
        0 {
            # no data? then do nothing
            return
        }
        1 {
            # only one data set? then plot it
            _doSettings [list \
                0 [list -width 2 \
                        -param [_getValues $_active current] \
                        -description [_getParamDesc all] \
                  ] \
                params $params \
            ]
            return
        }
    }

    #
    # Find the selected run.  If the run setting changed, then
    # look at its current value.  Otherwise, search the results
    # for a tuple that matches the current settings.
    #
    if {$changed == "xmlobj"} {
        # value is "#2" -- skip # and adjust range starting from 0
        set irun [string range $_cntlInfo($this-xmlobj-value) 1 end]
        if {"" != $irun} { set irun [expr {$irun-1}] }
    } else {
        set format ""
        set tuple ""
        foreach col [lrange [$_results column names] 1 end] {
            lappend format $col
            lappend tuple $_cntlInfo($this-$col-value)
        }
        set irun [lindex [$_results find -format $format -- $tuple] 0]

        if {"" == $irun && "" != $changed
             && $itk_option(-missingdata) == "skip"} {
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
            set allcols [lrange [$_results column names] 1 end]
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
                set irun [lindex [$_results find -format $format -- $tuple] 0]
                if {"" != $irun} {
                    break
                }
            }
        }
    }

    #
    # If we found a particular run, then load its values into all
    # controls.
    #
    if {"" != $irun} {
        # stop reacting to value changes
        set _settings 1

        set format [lrange [$_results column names] 1 end]
        if {[llength $format] == 1} {
            set data [$_results get -format $format $irun]
        } else {
            set data [lindex [$_results get -format $format $irun] 0]
        }

        foreach col $format val $data {
            set _cntlInfo($this-$col-value) $val
        }
        set _cntlInfo($this-xmlobj-value) "#[expr {$irun+1}]"

        # okay, react to value changes again
        set _settings 0
    }

    #
    # Search for tuples matching the current setting and
    # plot them.
    #
    if {$_plotall && $_active == "xmlobj"} {
        set format ""
    } else {
        set format ""
        set tuple ""
        foreach col [lrange [$_results column names] 1 end] {
            if {!$_plotall || $col != $_active} {
                lappend format $col
                lappend tuple $_cntlInfo($this-$col-value)
            }
        }
    }

    if {"" != $format} {
        set ilist [$_results find -format $format -- $tuple]
    } else {
        set ilist [$_results find]
    }

    if {[llength $ilist] > 0} {
        # search for the result for these settings
        set format ""
        set tuple ""
        foreach col [lrange [$_results column names] 1 end] {
            lappend format $col
            lappend tuple $_cntlInfo($this-$col-value)
        }
        set icurr [$_results find -format $format -- $tuple]

        # no data for these settings? prompt the user to simulate
        if {"" == $icurr} {
            _doPrompt on
        }

        if {[llength $ilist] == 1} {
            # single result -- always use active color
            set i [lindex $ilist 0]
            set plist [list \
                $i [list -width 2 \
                         -param [_getValues $_active $i] \
                         -description [_getParamDesc all $i] \
                   ] \
                params $params \
            ]
        } else {
            #
            # Get the color for all points according to
            # the color spectrum.
            #
            set plist [list params $params]
            foreach i $ilist {
                if {$i == $icurr} {
                    lappend plist $i [list -width 3 -raise 1 \
                        -param [_getValues $_active $i] \
                        -description [_getParamDesc all $i]]
                } else {
                    lappend plist $i [list -brightness 0.7 -width 1 \
                        -param [_getValues $_active $i] \
                        -description [_getParamDesc all $i]]
                }
            }
        }

        #
        # Load up the matching plots
        #
        _doSettings $plist

    } elseif {$itk_option(-missingdata) == "prompt"} {
        # prompt the user to simulate these settings
        _doPrompt on
        _doSettings  ;# clear plotting area

        # clear the current run selection -- there is no run for this
        set _settings 1
        set _cntlInfo($this-xmlobj-value) ""
        set _settings 0
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixExplore
#
# Called automatically whenever the user toggles the "Explore" button
# on the parameter popup.  Changes the -missingdata option back and
# forth, to allow for missing data or skip it.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_fixExplore {} {
    if {$_explore} {
        configure -missingdata prompt
    } else {
        configure -missingdata skip
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
itcl::body Rappture::ResultSet::_fixValue {col why} {
    if {[info exists _cntlInfo($this-$col-id)]} {
        set id $_cntlInfo($this-$col-id)

        set popup [$itk_component(options) contents frame]
        set widget $popup.label$id
        set wmax [winfo width $popup.dial$id]
        _drawValue $col $widget $wmax

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
itcl::body Rappture::ResultSet::_drawValue {col widget wmax} {
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
itcl::body Rappture::ResultSet::_toggleAll {{col "current"}} {
    if {$col == "current"} {
        set col $_active
    }
    if {![info exists _cntlInfo($this-$col-id)]} {
        return
    }
    set id $_cntlInfo($this-$col-id)
    set popup [$itk_component(options) contents frame]
    set pbutton $popup.all$id
    set current [$pbutton cget -relief]
    set sbutton $itk_component(dials).all

    foreach c $_cntlInfo($this-all) {
        set id $_cntlInfo($this-$c-id)
        $popup.all$id configure -relief raised \
            -background $itk_option(-background) \
            -foreground $itk_option(-foreground)
    }

    if {$current == "sunken"} {
        $pbutton configure -relief raised \
            -background $itk_option(-activecontrolbackground) \
            -foreground $itk_option(-activecontrolforeground)
        $sbutton configure -relief raised \
            -background $itk_option(-activecontrolbackground) \
            -foreground $itk_option(-activecontrolforeground)
        set _plotall 0
    } else {
        $pbutton configure -relief sunken \
            -background $itk_option(-togglebackground) \
            -foreground $itk_option(-toggleforeground)
        $sbutton configure -relief sunken \
            -background $itk_option(-togglebackground) \
            -foreground $itk_option(-toggleforeground)
        set _plotall 1

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
# Called automatically whenever the user hovers a control within
# this widget.  Returns the tooltip associated with the control.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_getValues {col {which ""}} {
    if {$col == "xmlobj"} {
        # load the Simulation # control
        set nruns [$_results size]
        for {set n 0} {$n < $nruns} {incr n} {
            set v "#[expr {$n+1}]"
            set label2val($v) $n
        }
    } else {
        set havenums 1
        set vlist ""
        foreach rec [$_results get -format [list xmlobj $col]] {
            set xo [lindex $rec 0]
            set v [lindex $rec 1]

            if {![info exists label2val($v)]} {
                lappend vlist $v
                foreach {raw norm} [Rappture::LibraryObj::value $xo $col] break
                set label2val($v) $norm

                if {$havenums && ![string is double $norm]} {
                    set havenums 0
                }
            }
        }

        if {!$havenums} {
            # don't have normalized nums? then sort and create nums
            catch {unset label2val}

            set n 0
            foreach v [lsort $vlist] {
                incr n
                set label2val($v) $n
            }
        }
    }

    switch -- $which {
        current {
            set curr $_cntlInfo($this-$col-value)
            if {[info exists label2val($curr)]} {
                return [list $curr $label2val($curr)]
            }
            return ""
        }
        all {
            return [array get label2val]
        }
        default {
            if {[string is integer $which]} {
                if {$col == "xmlobj"} {
                    set val "#[expr {$which+1}]"
                } else { 
		    set val [lindex [$_results get -format [list $col] $which] 0]
                }
                if {[info exists label2val($val)]} {
                    return [list $val $label2val($val)]
                }
                return ""
            }
            error "bad option \"$which\": should be all, current, or an integer index"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _getTooltip <role> <column>
#
# Called automatically whenever the user hovers a control within
# this widget.  Returns the tooltip associated with the control.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_getTooltip {role column} {
    set label ""
    set tip ""
    if {$column == "active"} {
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
            if {$column != $_active} {
                append tip "\n\nClick to activate this control."
            }
        }
        dial {
            append tip "\n\nClick to change the value of this parameter."
        }
        all {
            if {$label == ""} {
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
        more {
            set tip "Click to access all parameters."
        }
    }
    return [string trim $tip]
}

# ----------------------------------------------------------------------
# USAGE: _getParamDesc <which> ?<index>?
#
# Used internally to build a descripton of parameters for the data
# tuple at the specified <index>.  This is passed on to the underlying
# results viewer, so it will know what data is being viewed.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_getParamDesc {which {index "current"}} {
    if {$index == "current"} {
        # search for the result for these settings
        set format ""
        set tuple ""
        foreach col [lrange [$_results column names] 1 end] {
            lappend format $col
            lappend tuple $_cntlInfo($this-$col-value)
        }
        set index [$_results find -format $format -- $tuple]
        if {"" == $index} {
            return ""  ;# somethings wrong -- bail out!
        }
    }

    switch -- $which {
        active {
            if {"" == $_active} {
                return ""
            }
        }
        all {
            set desc ""
            foreach col $_cntlInfo($this-all) {
                set quantity $_cntlInfo($this-$col-label)
                set val [lindex [$_results get -format [list $col] $index] 0]
                if {$col == "xmlobj"} {
                    set num [lindex [$_results find -format xmlobj $val] 0]
                    set val "#[expr {$num+1}]"
                }
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
# OPTION: -missingdata
# ----------------------------------------------------------------------
itcl::configbody Rappture::ResultSet::missingdata {
    set opts {prompt skip}
    if {[lsearch -exact $opts $itk_option(-missingdata)] < 0} {
        error "bad value \"$itk_option(-missingdata)\": should be [join $opts {, }]"
    }
    set _explore [expr {$itk_option(-missingdata) != "skip"}]
}

# ----------------------------------------------------------------------
# OPTION: -activecontrolbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::ResultSet::activecontrolbackground {
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# OPTION: -activecontrolforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::ResultSet::activecontrolforeground {
    $_dispatcher event -idle !layout
}
