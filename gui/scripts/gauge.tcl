# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: gauge - compact readout for real values
#
#  This widget is a readout for a real value.  It has a little glyph
#  filled with color according to the value, followed by a numeric
#  representation of the value itself.  The value can be edited, and
#  a list of predefined values can be associated with a menu that
#  drops down from the value.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *Gauge.sampleWidth 30 widgetDefault
option add *Gauge.sampleHeight 20 widgetDefault
option add *Gauge.borderWidth 2 widgetDefault
option add *Gauge.relief sunken widgetDefault
option add *Gauge.valuePosition "right" widgetDefault
option add *Gauge.textBackground #cccccc widgetDefault
option add *Gauge.editable yes widgetDefault

itcl::class Rappture::Gauge {
    inherit itk::Widget

    itk_option define -editable editable Editable ""
    itk_option define -state state State "normal"
    itk_option define -spectrum spectrum Spectrum ""
    itk_option define -type type Type "real"
    itk_option define -units units Units ""
    itk_option define -minvalue minValue MinValue ""
    itk_option define -maxvalue maxValue MaxValue ""
    itk_option define -presets presets Presets ""
    itk_option define -valueposition valuePosition ValuePosition ""
    itk_option define -image image Image ""
    itk_option define -samplewidth sampleWidth SampleWidth 0
    itk_option define -sampleheight sampleHeight SampleHeight 0
    itk_option define -log log Log ""
    itk_option define -validatecommand validateCommand ValidateCommand ""

    constructor {args} { # defined below }

    public method value {args}
    public method edit {option}
    public method bump {delta}

    protected method _redraw {}
    protected method _resize {}
    protected method _hilite {comp state}
    protected method _editor {option args}
    protected method _presets {option}
    protected method _layout {}
    protected method _log {event args}

    private variable _value 0  ;# value for this widget

    blt::bitmap define GaugeArrow {
        #define arrow_width 9
        #define arrow_height 4
        static unsigned char arrow_bits[] = {
           0x7f, 0x00, 0x3e, 0x00, 0x1c, 0x00, 0x08, 0x00};
    }
}

itk::usual Gauge {
    keep -cursor -font -foreground -background
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::constructor {args} {
    itk_option remove hull.borderwidth hull.relief
    component hull configure -borderwidth 0

    itk_component add icon {
        canvas $itk_interior.icon -width 1 -height 1 \
            -borderwidth 0 -highlightthickness 0
    } {
        usual
        ignore -highlightthickness -highlightbackground -highlightcolor
        ignore -borderwidth -relief
    }
    bind $itk_component(icon) <Configure> [itcl::code $this _redraw]

    itk_component add -protected vframe {
        frame $itk_interior.vframe
    } {
        keep -borderwidth -relief
    }

    itk_component add value {
        label $itk_component(vframe).value -width 7 \
            -borderwidth 1 -relief flat -textvariable [itcl::scope _value]
    } {
        keep -font
        rename -background -textbackground textBackground Background
    }
    pack $itk_component(value) -side left -expand yes -fill both

    bind $itk_component(value) <Enter> [itcl::code $this _hilite value on]
    bind $itk_component(value) <Leave> [itcl::code $this _hilite value off]

    bind $itk_component(value) <<Cut>> [itcl::code $this edit cut]
    bind $itk_component(value) <<Copy>> [itcl::code $this edit copy]
    bind $itk_component(value) <<Paste>> [itcl::code $this edit paste]

    itk_component add emenu {
        menu $itk_component(value).menu -tearoff 0
    } {
        usual
        ignore -tearoff
    }
    $itk_component(emenu) add command -label "Cut" -accelerator "^X" \
        -command [list event generate $itk_component(value) <<Cut>>]
    $itk_component(emenu) add command -label "Copy" -accelerator "^C" \
        -command [list event generate $itk_component(value) <<Copy>>]
    $itk_component(emenu) add command -label "Paste" -accelerator "^V" \
        -command [list event generate $itk_component(value) <<Paste>>]
    bind $itk_component(value) <<PopupMenu>> \
        [itcl::code $this _editor menu %X %Y]

    itk_component add editor {
        Rappture::Editor $itk_interior.editor \
            -activatecommand [itcl::code $this _editor activate] \
            -validatecommand [itcl::code $this _editor validate] \
            -applycommand [itcl::code $this _editor apply]
    }
    bind $itk_component(value) <ButtonPress> \
        [itcl::code $this _editor popup]


    itk_component add spinner {
        frame $itk_component(vframe).spinner
    }

    itk_component add spinup {
        button $itk_component(spinner).up -image [Rappture::icon intplus] \
            -borderwidth 1 -relief raised -highlightthickness 0 \
            -command [itcl::code $this bump 1]
    } {
        usual
        ignore -borderwidth -highlightthickness
        rename -background -buttonbackground buttonBackground Background
    }
    pack $itk_component(spinup) -side left -expand yes -fill both

    itk_component add spindn {
        button $itk_component(spinner).down -image [Rappture::icon intminus] \
            -borderwidth 1 -relief raised -highlightthickness 0 \
            -command [itcl::code $this bump -1]
    } {
        usual
        ignore -borderwidth -highlightthickness
        rename -background -buttonbackground buttonBackground Background
    }
    pack $itk_component(spindn) -side right -expand yes -fill both

    itk_component add presets {
        button $itk_component(vframe).psbtn -bitmap GaugeArrow \
            -borderwidth 1 -highlightthickness 0 -relief raised
    } {
        usual
        ignore -borderwidth -relief -highlightthickness
    }

    bind $itk_component(presets) <Enter> [itcl::code $this _hilite presets on]
    bind $itk_component(presets) <Leave> [itcl::code $this _hilite presets off]

    itk_component add presetlist {
        Rappture::Dropdownlist $itk_component(presets).plist \
            -postcommand [itcl::code $this _presets post] \
            -unpostcommand [itcl::code $this _presets unpost] \
    }

    bind $itk_component(presetlist) <<DropdownlistSelect>> \
        [itcl::code $this _presets select]

    $itk_component(presets) configure -command \
        [list $itk_component(presetlist) post $itk_component(vframe) left]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: value ?-check? ?<newval>?
#
# Clients use this to query/set the value for this widget.  With
# no args, it returns the current value for the widget.  If the
# <newval> is specified, it sets the value of the widget and
# sends a <<Value>> event.  If the -check flag is included, the
# new value is not actually applied, but just checked for correctness.
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::value {args} {
    set onlycheck 0
    set i [lsearch -exact $args -check]
    if {$i >= 0} {
        set onlycheck 1
        set args [lreplace $args $i $i]
    }

    if {[llength $args] == 1} {
        #
        # If this gauge has -units, try to convert the incoming
        # value to that system of units.  Also, make sure that
        # the value is bound by any min/max value constraints.
        #
        # Keep track of the inputted units so we can give a
        # response about min and max values in familiar units.
        #
        set newval [set nv [string trim [lindex $args 0]]]
        set units $itk_option(-units)
        if {"" != $units} {
            set newval [Rappture::Units::convert $newval -context $units]
            set nvUnits [Rappture::Units::Search::for $newval]
            if { "" == $nvUnits} {
                set msg [Rappture::Units::description $units]
                error "unrecognized units in value \"$newval\": should be value with units of $msg"
            }
            set nv [Rappture::Units::convert $nv \
                -context $units -to $units -units off]

            # Normalize the units name
            set newval [Rappture::Units::convert $newval -units off]$nvUnits
        }

        switch -- $itk_option(-type) {
            integer {
                if { [scan $nv "%g" value] != 1 || int($nv) != $value } {
                    error "bad value \"$nv\": should be an integer value"
                }
            }
            real {
                # "scan" will reject the number if the string is "NaN" or
                # "Inf" or the empty string.  It also is accepts large numbers
                # (e.g. 111111111111111111111) that "string is double"
                # rejects.  The problem with "scan" is that it doesn't care if
                # there are extra characters trailing the number (eg. "123a").
                # The extra %s substitution is used to detect this case.
                if { [scan $nv "%g%s" dummy1 dummy2] != 1 } {
                    error "bad value \"$nv\": should be a real number"
                }
            }
        }

        if {"" != $itk_option(-minvalue)} {
            set convMinVal [set minv $itk_option(-minvalue)]
            if {"" != $units} {
                set minv [Rappture::Units::convert $minv \
                    -context $units -to $units -units off]
                set convMinVal [Rappture::Units::convert \
                    $itk_option(-minvalue) -context $units -to $nvUnits]
            } else {
                set newval [format "%g" $newval]
            }

            # fix for the case when the user tries to
            # compare values like minv=-500 nv=-0600
            set nv [format "%g" $nv]
            set minv [format "%g" $minv]

            if {$nv < $minv} {
                error "minimum value allowed here is $convMinVal"
            }
        }

        if {"" != $itk_option(-maxvalue)} {
            set convMaxVal [set maxv $itk_option(-maxvalue)]
            if {"" != $units} {
                set maxv [Rappture::Units::convert $maxv \
                    -context $units -to $units -units off]
                set convMaxVal [Rappture::Units::convert \
                    $itk_option(-maxvalue) -context $units -to $nvUnits]
            } else {
                set newval [format "%g" $newval]
            }

            # fix for the case when the user tries to
            # compare values like maxv=500 nv=0600
            set nv [format "%g" $nv]
            set maxv [format "%g" $maxv]

            if {$nv > $maxv} {
                error "maximum value allowed here is $convMaxVal"
            }
        }

        #
        # If there's a -validatecommand option, then invoke the code
        # now to check the new value.
        #
        if {[string length $itk_option(-validatecommand)] > 0} {
            set cmd "uplevel #0 [list $itk_option(-validatecommand) [list $newval]]"
            set result [eval $cmd]
        }

        if {$onlycheck} {
            return
        }

        set _value $newval

        _redraw
        event generate $itk_component(hull) <<Value>>

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }
    return $_value
}

# ----------------------------------------------------------------------
# USAGE: edit cut
# USAGE: edit copy
# USAGE: edit paste
#
# Used internally to handle cut/copy/paste operations for the current
# value.  Usually invoked by <<Cut>>, <<Copy>>, <<Paste>> events, but
# can also be called directly through this method.
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::edit {option} {
    if {$itk_option(-state) == "disabled"} {
        return  ;# disabled? then bail out here!
    }
    switch -- $option {
        cut {
            edit copy
            _editor popup
            $itk_component(editor) value ""
            $itk_component(editor) deactivate
        }
        copy {
            clipboard clear
            clipboard append $_value
        }
        paste {
            _editor popup
            $itk_component(editor) value [clipboard get]
            $itk_component(editor) deactivate
        }
        default {
            error "bad option \"$option\": should be cut, copy, paste"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: bump <delta>
#
# Changes the current value up/down by the <delta> value.  Used
# internally by the up/down spinner buttons when the value is
# -type integer.
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::bump {delta} {
    set val $_value
    if {$val == ""} {
        set val 0
    }
    if {[catch {value [expr {$val+$delta}]} result]} {
        if {[regexp {allowed here is (.+)} $result match newval]} {
            set _value $newval
            $itk_component(value) configure -text $newval
        }
        if {[regexp {^bad.*: +(.)(.+)} $result match first tail]
              || [regexp {(.)(.+)} $result match first tail]} {
            set result "[string toupper $first]$tail"
        }
        bell
        Rappture::Tooltip::cue $itk_component(value) $result
        _log warning $result
        return 0
    }
    _log input [value]
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to redraw the gauge on the internal canvas based
# on the current value and the size of the widget.  In this simple
# base class, the gauge is drawn as a colored block, with an optional
# image in the middle of it.
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::_redraw {} {
    set c $itk_component(icon)
    set w [winfo width $c]
    set h [winfo height $c]

    if {"" == [$c find all]} {
        # first time around, create the items
        $c create rectangle 0 0 1 1 -outline black -tags block
        $c create image 0 0 -anchor center -image "" -tags bimage
        $c create rectangle 0 0 1 1 -outline "" -fill "" -stipple gray50 -tags screen
    }

    if {"" != $itk_option(-spectrum)} {
        set color [$itk_option(-spectrum) get $_value]
    } else {
        set color ""
    }

    # update the items based on current values
    $c coords block 0 0 [expr {$w-1}] [expr {$h-1}]
    $c coords screen 0 0 $w $h
    $c itemconfigure block -fill $color

    $c coords bimage [expr {0.5*$w}] [expr {0.5*$h}]

    if {$itk_option(-state) == "disabled"} {
        $c itemconfigure screen -fill white
    } else {
        $c itemconfigure screen -fill ""
    }
}

# ----------------------------------------------------------------------
# USAGE: _resize
#
# Used internally to resize the internal canvas based on the -image
# option or the size of the text.
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::_resize {} {
    set w 0
    set h 0

    if {"" != $itk_option(-image) || "" != $itk_option(-spectrum)} {
        if {$itk_option(-samplewidth) > 0} {
            set w $itk_option(-samplewidth)
        } else {
            if {$itk_option(-image) != ""} {
                set w [expr {[image width $itk_option(-image)]+4}]
            } else {
                set w [winfo reqheight $itk_component(value)]
            }
        }

        if {$itk_option(-sampleheight) > 0} {
            set h $itk_option(-sampleheight)
        } else {
            if {$itk_option(-image) != ""} {
                set h [expr {[image height $itk_option(-image)]+4}]
            } else {
                set h [winfo reqheight $itk_component(value)]
            }
        }
    }

    if {$w > 0 && $h > 0} {
        $itk_component(icon) configure -width $w -height $h
    }
}

# ----------------------------------------------------------------------
# USAGE: _hilite <component> <state>
#
# Used internally to resize the internal canvas based on the -image
# option or the size of the text.
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::_hilite {comp state} {
    if {$itk_option(-state) == "disabled"} {
        set state 0  ;# disabled? then don't hilite
    }
    if {$comp == "value" && !$itk_option(-editable)} {
        $itk_component(value) configure -relief flat
        return
    }

    if {$state} {
        $itk_component($comp) configure -relief flat
    } else {
        if {$comp eq "presets"} {
            $itk_component($comp) configure -relief raised
        } else {
            $itk_component($comp) configure -relief flat
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _editor popup
# USAGE: _editor activate
# USAGE: _editor validate <value>
# USAGE: _editor apply <value>
# USAGE: _editor menu <rootx> <rooty>
#
# Used internally to handle the various functions of the pop-up
# editor for the value of this gauge.
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::_editor {option args} {
    if {$itk_option(-state) == "disabled"} {
        return  ;# disabled? then bail out here!
    }
    switch -- $option {
        popup {
            if {$itk_option(-editable)} {
                $itk_component(editor) activate
            }
        }
        activate {
            return [list text $_value \
                x [winfo rootx $itk_component(value)] \
                y [winfo rooty $itk_component(value)] \
                w [winfo width $itk_component(value)] \
                h [winfo height $itk_component(value)]]
        }
        validate {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"_editor validate val\""
            }
            set val [lindex $args 0]

            if {[catch {value -check $val} result]} {
                if {[regexp {allowed here is (.+)} $result match newval]} {
                    $itk_component(editor) value $newval
                }
                if {[regexp {^bad.*: +(.)(.+)} $result match first tail]
                      || [regexp {(.)(.+)} $result match first tail]} {
                    set result "[string toupper $first]$tail"
                }
                bell
                Rappture::Tooltip::cue $itk_component(editor) $result
                _log warning $result
                return 0
            }
        }
        apply {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"_editor apply val\""
            }
            set newval [lindex $args 0]
            value $newval
            _log input $newval
        }
        menu {
            eval tk_popup $itk_component(emenu) $args
        }
        default {
            error "bad option \"$option\": should be popup, activate, validate, apply, and menu"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _presets post
# USAGE: _presets unpost
# USAGE: _presets select
#
# Used internally to handle the list of presets for this gauge.  The
# post/unpost options are invoked when the list is posted or unposted
# to manage the relief of the controlling button.  The select option
# is invoked whenever there is a selection from the list, to assign
# the value back to the gauge.
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::_presets {option} {
    switch -- $option {
        post {
            set i [$itk_component(presetlist) index $_value]
            if {$i >= 0} {
                $itk_component(presetlist) select clear 0 end
                $itk_component(presetlist) select set $i
            }
            after 10 [list $itk_component(presets) configure -relief sunken]
        }
        unpost {
            $itk_component(presets) configure -relief raised
        }
        select {
            set val [$itk_component(presetlist) current]
            if {"" != $val} {
                value $val
                _log input $val
            }
        }
        default {
            error "bad option \"$option\": should be post, unpost, select"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _layout
#
# Used internally to fix the layout of widgets whenever there is a
# change in the options that affect layout.  Puts the value in the
# proper position according to the -valueposition option.  Also,
# adds or removes the icon if it needs to be shown.
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::_layout {} {
    foreach w [pack slaves $itk_component(hull)] {
        pack forget $w
    }

    array set side2anchor {
        left   e
        right  w
        top    s
        bottom n
    }
    set pos $itk_option(-valueposition)
    pack $itk_component(vframe) -side $pos \
        -expand yes -fill both -ipadx 2
    $itk_component(value) configure -anchor $side2anchor($pos)

    if {"" != $itk_option(-image) || "" != $itk_option(-spectrum)} {
        pack $itk_component(icon) -side $pos -padx 2
    }
}

# ----------------------------------------------------------------------
# USAGE: _log event ?arg arg...?
#
# Used internally to send info to the logging mechanism.  If the -log
# argument is set, then this calls the Rappture::Logger mechanism to
# log the rest of the arguments as an action.  Otherwise, it does
# nothing.
# ----------------------------------------------------------------------
itcl::body Rappture::Gauge::_log {event args} {
    if {$itk_option(-log) ne ""} {
        eval Rappture::Logger::log $event [list $itk_option(-log)] $args
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -editable
# ----------------------------------------------------------------------
itcl::configbody Rappture::Gauge::editable {
    if {![string is boolean -strict $itk_option(-editable)]} {
        error "bad value \"$itk_option(-editable)\": should be boolean"
    }
    if {!$itk_option(-editable) && [winfo ismapped $itk_component(editor)]} {
        $itk_component(editor) deactivate -abort
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::Gauge::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(value) configure -state $itk_option(-state)
    $itk_component(spinup) configure -state $itk_option(-state)
    $itk_component(spindn) configure -state $itk_option(-state)
    $itk_component(presets) configure -state $itk_option(-state)
    _redraw  ;# fix gauge
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -spectrum
# ----------------------------------------------------------------------
itcl::configbody Rappture::Gauge::spectrum {
    if {$itk_option(-spectrum) != ""
          && ([catch {$itk_option(-spectrum) isa ::Rappture::Spectrum} valid]
               || !$valid)} {
        error "bad option \"$itk_option(-spectrum)\": should be Rappture::Spectrum object"
    }
    _resize
    _layout
    _redraw
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -image
# ----------------------------------------------------------------------
itcl::configbody Rappture::Gauge::image {
    if {$itk_option(-image) != ""
          && [catch {image width $itk_option(-image)}]} {
        error "bad value \"$itk_option(-image)\": should be Tk image"
    }
    _resize
    _layout
    $itk_component(icon) itemconfigure bimage -image $itk_option(-image)
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -units
# ----------------------------------------------------------------------
itcl::configbody Rappture::Gauge::units {
    if {$itk_option(-units) != ""
          && [::Rappture::Units::System::for $itk_option(-units)] == ""} {
        error "unrecognized system of units \"$itk_option(-units)\""
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -valueposition
# ----------------------------------------------------------------------
itcl::configbody Rappture::Gauge::valueposition {
    set pos $itk_option(-valueposition)
    set opts {left right top bottom}
    if {[lsearch -exact $opts $pos] < 0} {
        error "bad value \"$pos\": should be [join $opts {, }]"
    }
    _layout
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -presets
# ----------------------------------------------------------------------
itcl::configbody Rappture::Gauge::presets {
    if {"" == $itk_option(-presets)} {
        pack forget $itk_component(presets)
    } else {
        if {$itk_option(-valueposition) == "left"} {
            set s "left"
        } else {
            set s "right"
        }
        set first [lindex [pack slaves $itk_component(vframe)] 0]
        pack $itk_component(presets) -before $first -side $s -fill y

        $itk_component(presetlist) delete 0 end
        $itk_component(presetlist) insert end $itk_option(-presets)
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -type
# ----------------------------------------------------------------------
itcl::configbody Rappture::Gauge::type {
    switch -- $itk_option(-type) {
        integer {
            set first [lindex [pack slaves $itk_component(vframe)] 0]
            if {$first == $itk_component(presets)} {
                pack $itk_component(spinner) -after $first -side left -fill y
            } else {
                pack $itk_component(spinner) -before $first -side right -fill y
            }
        }
        real {
            pack forget $itk_component(spinner)
        }
        default {
            error "bad number type \"$itk_option(-type)\": should be integer or real"
        }
    }
}
