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
option add *ResultSet.toggleBackground gray widgetDefault
option add *ResultSet.toggleForeground white widgetDefault
option add *ResultSet.textFont \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault
option add *ResultSet.boldFont \
    -*-helvetica-bold-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::ResultSet {
    inherit itk::Widget

    itk_option define -togglebackground toggleBackground Background ""
    itk_option define -toggleforeground toggleForeground Foreground ""
    itk_option define -textfont textFont Font ""
    itk_option define -boldfont boldFont Font ""
    itk_option define -missingdata missingData MissingData ""
    itk_option define -clearcommand clearCommand ClearCommand ""
    itk_option define -settingscommand settingsCommand SettingsCommand ""
    itk_option define -promptcommand promptCommand PromptCommand ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {xmlobj}
    public method clear {}
    public method contains {xmlobj}
    public method size {{what -results}}

    protected method _doClear {}
    protected method _doSettings {{cmd ""}}
    protected method _fixControls {args}
    protected method _fixSettings {args}
    protected method _doPrompt {state}
    protected method _toggleAll {path widget}

    private variable _dispatcher ""  ;# dispatchers for !events
    private variable _results ""     ;# tuple of known results
    private variable _recent ""      ;# most recent result in _results
    private variable _plotall ""     ;# column with "All" active
    private variable _col2widget     ;# maps column name => control widget
    private variable _counter 0      ;# counter for unique control names
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
    $_dispatcher register !settings
    $_dispatcher dispatch $this !settings \
        [itcl::code $this _fixSettings]

    # create a list of tuples for data
    set _results [Rappture::Tuples ::#auto]
    $_results column insert end -name xmlobj -label "top-level XML object"


    itk_component add cntls {
        frame $itk_interior.cntls
    }
    pack $itk_component(cntls) -fill x

    itk_component add clear {
        button $itk_component(cntls).clear -text "Clear" -state disabled \
            -relief flat -overrelief raised \
            -command [itcl::code $this _doClear]
    }
    pack $itk_component(clear) -side right
    Rappture::Tooltip::for $itk_component(clear) \
        "Clears all results collected so far."

    itk_component add status {
        label $itk_component(cntls).status -anchor w -text "No results"
    }
    pack $itk_component(status) -side left -expand yes -fill x

    itk_component add scroller {
        Rappture::Scroller $itk_interior.scroller \
            -xscrollmode off -yscrollmode auto -height 1i
    }
    pack $itk_component(scroller) -expand yes -fill both

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
        if {[$_results column names $vpath] == ""} {
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
    set f [$itk_component(scroller) contents frame]
    foreach w [winfo children $f] {
        destroy $w
    }
    catch {unset _col2widget}
    set _plotall ""
    set _counter 0

    # don't need to scroll adjustor controls right now
    $itk_component(scroller) configure -yscrollmode off

    # clear out all results
    $_results delete 0 end
    eval $_results column delete [lrange [$_results column names] 1 end]
    set _recent ""

    # update status and Clear button
    $itk_component(status) configure -text "No results"
    $itk_component(clear) configure -state disabled
    $_dispatcher event -idle !fixcntls
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
# USAGE: size ?-results|-controls?
#
# Returns the number of results or the number of controls in this
# result set.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::size {{what -results}} {
    switch -- $what {
        -results {
            return [$_results size]
        }
        -controls {
            return [array size _col2widget]
        }
        default {
            error "bad option \"$what\": should be -results or -controls"
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
# USAGE: _fixControls ?<eventArgs...>?
#
# Called automatically at the idle point after one or more results
# have been added to this result set.  Scans through all existing
# data and updates controls used to select the data.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_fixControls {args} {
    set f [$itk_component(scroller) contents frame]
    grid columnconfigure $f 1 -weight 1

    if {[$_results size] == 0} {
        return
    }

    #
    # Scan through all columns in the data and create any
    # controls that just appeared.
    #
    foreach col [lrange [$_results column names] 1 end] {
        set xmlobj [$_results get -format xmlobj 0]

        #
        # If this column doesn't have a control yet, then
        # create one.
        #
        if {![info exists _col2widget($col)]} {
            # add an "All" button to plot all results
            label $f.all$_counter -text "All" -padx 8 \
                -borderwidth 1 -relief raised -font $itk_option(-textfont)
            grid $f.all$_counter -row $_counter -column 0 \
                -padx 8 -pady 2 -sticky nsew
            Rappture::Tooltip::for $f.all$_counter "Plot all values for this quantity"

            bind $f.all$_counter <ButtonPress> \
                [itcl::code $this _toggleAll $col $f.all$_counter]

            # search for the first XML object with this element defined
            foreach xmlobj [$_results get -format xmlobj] {
                set str [$xmlobj get $col.about.label]
                if {"" == $str} {
                    set str [$xmlobj element -as id $col]
                }
                if {"" != $str} {
                    break
                }
            }

            if {"" != $str} {
                set w $f.label$_counter
                label $w -text $str -anchor w -font $itk_option(-boldfont)
                grid $w -row $_counter -column 1 -sticky w

                grid $f.all$_counter -rowspan 2
                Rappture::Tooltip::for $f.all$_counter "Plot all values for $str"
                incr _counter
            }

            set w $f.cntl$_counter
            Rappture::Radiodial $w
            grid $w -row $_counter -column 1 -sticky ew
            bind $w <<Value>> \
                [itcl::code $_dispatcher event -after 100 !settings column $col widget $w]
            set _col2widget($col) $w

            incr _counter
            grid rowconfigure $f $_counter -minsize 4
            incr _counter

            $itk_component(scroller) configure -yscrollmode auto

            # let clients know that a new control appeared
            # so they can fix the overall size accordingly
            event generate $itk_component(hull) <<Control>>
        }

        #
        # Determine the unique values for this column and load
        # them into the control.
        #
        catch {unset values}
        set havenums 1
        set vlist ""
        foreach rec [$_results get -format [list xmlobj $col]] {
            set xo [lindex $rec 0]
            set v [lindex $rec 1]

            if {![info exists values($v)]} {
                lappend vlist $v
                foreach {raw norm} [Rappture::LibraryObj::value $xo $col] break
                set values($v) $norm

                if {$havenums && ![string is double $norm]} {
                    set havenums 0
                }
            }
        }

        if {!$havenums} {
            # don't have normalized nums? then sort and create nums
            catch {unset values}

            set n 0
            foreach v [lsort $vlist] {
                set values($v) [incr n]
            }
        }

        # load the results into the control
        set w $_col2widget($col)
        $w clear
        foreach v [array names values] {
            $w add $v $values($v)
        }
    }

    #
    # Set all controls to the settings of the most recent
    # addition.
    #
    if {"" != $_recent} {
        foreach col [array names _col2widget] {
            set raw [lindex [Rappture::LibraryObj::value $_recent $col] 0]
            $_col2widget($col) current $raw
        }
    }

    # fix the settings after everything settles
    $_dispatcher event -after 100 !settings
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
    _doPrompt off

    switch -- [$_results size] {
        0 {
            # no data? then do nothing
            return
        }
        1 {
            # only one data set? then plot it
            _doSettings [list 0 [list -width 2]]
            return
        }
    }

    #
    # Search for tuples matching the current setting and
    # plot them.
    #
    set format ""
    set tuple ""
    foreach col [lrange [$_results column names] 1 end] {
        if {$col != $_plotall} {
            lappend format $col
            set w $_col2widget($col)
            lappend tuple [$w get current]
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
            set w $_col2widget($col)
            lappend tuple [$w get current]
        }
        set icurr [$_results find -format $format -- $tuple]

        # no data for these settings? prompt the user to simulate
        if {"" == $icurr} {
            _doPrompt on
        }

        if {[llength $ilist] == 1} {
            # single result -- always use active color
            set i [lindex $ilist 0]
            set plist [list $i [list -width 2]]
        } else {
            #
            # Get the color for all points according to
            # the color spectrum.
            #
            set plist ""
            foreach i $ilist {
                if {$i == $icurr} {
                    lappend plist $i [list -width 3 -raise 1]
                } else {
                    lappend plist $i [list -brightness 0.7 -width 1]
                }
            }
        }

        #
        # Load up the matching plots
        #
        _doSettings $plist
    } elseif {$itk_option(-missingdata) == "skip"} {
        #
        # No data for these settings.  Try leaving the next
        # column open, then the next, and so forth, until
        # we find some data.
        #
        array set eventdata $args
        if {[info exists eventdata(column)]} {
            set changed $eventdata(column)
            set allcols [lrange [$_results column names] 1 end]
            set i [lsearch -exact $allcols $changed]
            set search [concat \
                [lrange $allcols [expr {$i+1}] end] \
                [lrange $allcols 0 [expr {$i-1}]] \
            ]
            set nsearch [llength $search]

            set tweak(widget) ""
            set tweak(value) ""
            for {set i 0} {$i < $nsearch} {incr i} {
                set format $eventdata(column)
                set tuple [list [$eventdata(widget) get current]]
                for {set j [expr {$i+1}]} {$j < $nsearch} {incr j} {
                    set col [lindex $search $j]
                    set w $_col2widget($col)
                    lappend format $col
                    lappend tuple [$w get current]
                }
                set ilist [$_results find -format $format -- $tuple]
                if {[llength $ilist] > 0} {
                    set col [lindex $search $i]
                    set tweak(widget) $_col2widget($col)
                    set first [lindex $ilist 0]
                    set tweak(value) [lindex [$_results get -format $col -- $first] 0]
                    break
                }
            }

            # set the value to the next valid result
            if {$tweak(widget) != ""} {
                $tweak(widget) current $tweak(value)
            }
        }

    } elseif {$itk_option(-missingdata) == "prompt"} {
        # prompt the user to simulate these settings
        _doPrompt on
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
                set w $_col2widget($col)
                set val [$w get current]
                lappend settings $col $val
            }
            uplevel #0 $itk_option(-promptcommand) [list on $message $settings]
        } else {
            uplevel #0 $itk_option(-promptcommand) off
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _toggleAll <path> <widget>
#
# Called automatically whenever the user clicks on an "All" button.
# Toggles the button between its on/off states.  In the "on" state,
# all results associated with the <path> are sent to the result viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_toggleAll {path widget} {
    if {[$widget cget -relief] == "sunken"} {
        $widget configure -relief raised \
            -background $itk_option(-background) \
            -foreground $itk_option(-foreground)

        set _plotall ""
    } else {
        # pop out all other "All" buttons
        set f [$itk_component(scroller) contents frame]
        for {set i 0} {$i < $_counter} {incr i} {
            if {[winfo exists $f.all$i]} {
                $f.all$i configure -relief raised \
                    -background $itk_option(-background) \
                    -foreground $itk_option(-foreground)
            }
        }

        # push this one in
        $widget configure -relief sunken \
            -background $itk_option(-togglebackground) \
            -foreground $itk_option(-toggleforeground)

        # switch the "All" context to this path
        set _plotall $path
    }
    $_dispatcher event -idle !settings
}

# ----------------------------------------------------------------------
# OPTION: -missingdata
# ----------------------------------------------------------------------
itcl::configbody Rappture::ResultSet::missingdata {
    set opts {prompt skip}
    if {[lsearch -exact $opts $itk_option(-missingdata)] < 0} {
        error "bad value \"$itk_option(-missingdata)\": should be [join $opts {, }]"
    }
}
