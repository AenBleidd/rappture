# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: ProbDistEditor - probability distribution editor
#
#  This widget displays a probability distribution and lets the user
#  edit it in various flavors.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2010  Purdue University
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *ProbDistEditor.fillColor blue widgetDefault
option add *ProbDistEditor*Entry.background white widgetDefault
option add *ProbDistEditor*Entry.width 10 widgetDefault

itcl::class Rappture::ProbDistEditor {
    inherit itk::Widget

    itk_option define -fillcolor fillColor FillColor ""

    constructor {args} {
        # defined below
    }
    public method value {{newval ""}}
    public method mode {{newval ""}}
    public method reset {}

    protected method _redraw {}
    protected method _axisLabels {which graph val}
    protected method _apply {op {widget ""}}

    protected variable _dispatcher "";  # dispatcher for !events
    protected variable _mode "";        # type of distribution function
    protected variable _value;          # data values for mode-detail
    protected variable _uvalue;         # data values for mode-detail with units
    protected variable _umin;           # minimum allowed value for tool with units
    protected variable _umax;           # maximum allowed value for tool with units
    protected variable _units;          # units for min and max
}

itk::usual ProbDistEditor {
    keep -cursor -font -foreground -background
    keep -fillcolor
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ProbDistEditor::constructor {min max units default} {
    # create a dispatcher for events
    #puts "PDE::constructor $min $max $default"

    # These are the tool min and max values.  May be empty.
    set _umin $min
    set _umax $max
    set _units $units
    set _uvalue(central) $default

    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw "[itcl::code $this _redraw]; list"

    itk_component add vals {
        frame $itk_interior.vals
    }
    pack $itk_component(vals) -side left -fill both -padx {0 10}
    #
    # EXACT value options
    #
    itk_component add exactvals {
        frame $itk_component(vals).exact
    }
    itk_component add evl {
        label $itk_component(exactvals).l -text "Value:"
    }
    entry $itk_component(exactvals).val
    _apply bindings $itk_component(exactvals).val
    grid $itk_component(evl) -row 0 -column 0 -sticky e
    grid $itk_component(exactvals).val -row 0 -column 1 -sticky ew -pady 2
    grid rowconfigure $itk_component(exactvals) 1 -weight 1
    #
    # UNIFORM value options
    #
    itk_component add uniformvals {
        frame $itk_component(vals).uniform
    }
    itk_component add uvl0 {
        label $itk_component(uniformvals).l0 -text "Minimum:"
    }
    entry $itk_component(uniformvals).min
    _apply bindings $itk_component(uniformvals).min
    grid $itk_component(uvl0) -row 0 -column 0 -sticky e
    grid $itk_component(uniformvals).min -row 0 -column 1 -sticky ew -pady 2

    itk_component add uvl1 {
        label $itk_component(uniformvals).l1 -text "Maximum:"
    }
    entry $itk_component(uniformvals).max
    _apply bindings $itk_component(uniformvals).max
    grid $itk_component(uvl1) -row 1 -column 0 -sticky e
    grid $itk_component(uniformvals).max -row 1 -column 1 -sticky ew -pady 2
    grid rowconfigure $itk_component(uniformvals) 2 -weight 1

    #
    # GAUSSIAN value options
    #
    itk_component add gaussianvals {
        frame $itk_component(vals).gaussian
    }
    itk_component add gvlm {
        label $itk_component(gaussianvals).lm -text "Mean:"
    }
    entry $itk_component(gaussianvals).mean
    _apply bindings $itk_component(gaussianvals).mean
    grid $itk_component(gvlm) -row 0 -column 0 -sticky e
    grid $itk_component(gaussianvals).mean -row 0 -column 1 -sticky ew -pady 2

    itk_component add gvls {
        label $itk_component(gaussianvals).ls -text "Std Deviation:"
    }
    entry $itk_component(gaussianvals).stddev
    _apply bindings $itk_component(gaussianvals).stddev
    grid $itk_component(gvls) -row 1 -column 0 -sticky e
    grid $itk_component(gaussianvals).stddev -row 1 -column 1 -sticky ew -pady 2

    itk_component add gvl0 {
        label $itk_component(gaussianvals).l0 -text "Minimum:" -state disabled
    }
    entry $itk_component(gaussianvals).min -state readonly
    _apply bindings $itk_component(gaussianvals).min
    grid $itk_component(gvl0) -row 2 -column 0 -sticky e
    grid $itk_component(gaussianvals).min -row 2 -column 1 -sticky ew -pady 2

    itk_component add gvl1 {
        label $itk_component(gaussianvals).l1 -text "Maximum:" -state disabled
    }
    entry $itk_component(gaussianvals).max -state readonly
    _apply bindings $itk_component(gaussianvals).max
    grid $itk_component(gvl1) -row 3 -column 0 -sticky e
    grid $itk_component(gaussianvals).max -row 3 -column 1 -sticky ew -pady 2
    grid rowconfigure $itk_component(gaussianvals) 4 -weight 1

    #
    # Preview graph to display probability density function
    #
    itk_component add graph {
        blt::graph $itk_interior.graph -width 2i -height 1i \
            -plotborderwidth 1 -plotrelief solid -plotpadx 0 -plotpady 0 \
            -highlightthickness 0
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(graph) -expand yes -fill both

    $itk_component(graph) xaxis configure -ticklength 4 \
        -command [itcl::code $this _axisLabels xaxis]
    $itk_component(graph) yaxis configure -ticklength 4 \
        -title "PDF" -command [itcl::code $this _axisLabels yaxis]
    $itk_component(graph) grid off
    $itk_component(graph) legend configure -hide yes

    # start in exact mode with default values
    mode "exact"

    eval itk_initialize

    #
    # Figure out a good size for all panels of options and for the
    # overall widget.
    #
    update idletasks
    set maxw 0
    set maxh 0
    foreach panel {exact uniform gaussian} {
        set w [winfo reqwidth $itk_component(${panel}vals)]
        if {$w > $maxw} { set maxw $w }
        set h [winfo reqheight $itk_component(${panel}vals)]
        if {$h > $maxh} { set maxh $h }
    }

    pack propagate $itk_component(vals) off
    $itk_component(vals) configure -width $maxw -height $maxh
}

# ----------------------------------------------------------------------
# USAGE: value ?<setValue>?
#
# With no arguments, this returns the current value within the widget
# as a list of key/value pairs:
#   mode gaussian mean xxx stddev xxx min xxx max xxx
#
# Each value always has a "mode" that indicates its type, and other
# values to characterize the type.
#
# If the value is specified, then it is interpreted as the sort of
# value that is passed back, but used to set the value.
# ----------------------------------------------------------------------
itcl::body Rappture::ProbDistEditor::value {{newval ""}} {
    # puts "PDE::value $newval"

    #FIXME: _value has numerical values _uvalue has value with units
    if {"" == $newval} {
        catch {_apply finalize}  ;# apply any outstanding changes

        switch -- $_mode {
            exact {
                return $_uvalue(central)
            }
            uniform {
                return [list uniform $_uvalue(min) $_uvalue(max)]
            }
            gaussian {
                return [list gaussian $_uvalue(central) $_uvalue(stddev)]
            }
            default {
                error "don't know how to format value for $_mode"
            }
        }
    }

    set newmode [lindex $newval 0]
    switch -- $newmode {
        uniform {
            foreach {utmp(min) utmp(max)} [lrange $newval 1 2] break
            if {[catch {Rappture::Units::convert $utmp(min) -units off} tmp(min)]} {
                set tmp(min) $utmp(min)
            }
            if {[catch {Rappture::Units::convert $utmp(max) -units off} tmp(max)]} {
                set tmp(max) $utmp(max)
            }
        }
        gaussian {
            foreach {utmp(central) utmp(stddev)} [lrange $newval 1 2] break
            if {[catch {Rappture::Units::convert $utmp(central) -units off} tmp(central)]} {
                set tmp(central) $utmp(central)
            }
            if {[catch {Rappture::Units::convert $utmp(stddev) -units off} tmp(stddev)]} {
                set tmp(stddev) $utmp(stddev)
            }
            set units [Rappture::Units::Search::for $utmp(central)]
            if {"" != $_umin} {
                if {"" != $units} {
                    set tmp(min) [Rappture::Units::convert $_umin -to $units -context $units -units off]
                } else {
                    set tmp(min) $_umin
                }
            }
            if {"" != $_umax} {
                if {"" != $units} {
                    set tmp(max) [Rappture::Units::convert $_umax -to $units -context $units -units off]
                } else {
                    set tmp(max) $_umax
                }
            }
        }
        exact {
            set utmp(central) [lindex $newval 1]
            if {[catch {Rappture::Units::convert $utmp(central) -units off} tmp(central)]} {
                set tmp(central) $utmp(central)
            }
        }
        default {
            set newmode exact
            set utmp(central) [lindex $newval 0]
            if {[catch {Rappture::Units::convert $utmp(central) -units off} tmp(central)]} {
                set tmp(central) $utmp(central)
            }
        }
    }
    array set _value [array get tmp]
    array set _uvalue [array get utmp]
    mode $newmode  ;# assign this mode and copy in values for editing
}

# ----------------------------------------------------------------------
# USAGE: mode ?<setMode>?
#
# With no arguments, this returns a list of allowed distribution
# modes.  Otherwise, the value sets the mode of the distribution and
# loads default or previous values for the distribution.  Note that
# the mode is also reported as part of the "value" returned from
# this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::ProbDistEditor::mode {{newval ""}} {
    # puts "PDE::mode mode=$newval min=$_umin max=$_umax units=$_units"

    set modes {exact uniform gaussian custom}
    if {"" == $newval} {
        return $modes
    }

    set check 0
    if {$newval == "check"} {
        set newval $_mode
        set check 1
    }

    if {[lsearch $modes $newval] < 0} {
        error "bad value \"$newval\": should be [join $modes {, }]"
    }

    # discard units from tool min/max
    if {$_units == ""} {
        set min $_umin
        set max $_umax
    } else {
        if {$_umin == ""} {
            set min ""
        } else {
            set min [Rappture::Units::convert $_umin -context $_units -units off]
        }
        if {$_umax == ""} {
            set max ""
        } else {
            set max [Rappture::Units::convert $_umax -context $_units -units off]
        }
    }

    # The mode was changed by the user.  Pick reasonable values.
    switch -- $newval {
        exact {
            if {![info exists _value(central)]} {
                if {$min != "" && $max != ""} {
                    set _value(central) [expr {0.5*($max + $min)}]
                } else {
                    set _value(central) 0.5
                }
                set _uvalue(central) $_value(central)$_units
            }
        }
        uniform {
            if {![info exists _value(min)] || ![info exists _value(max)]} {
                set _value(max) [expr $_value(central) * 1.1]
                set _value(min) [expr $_value(central) * 0.9]
                if {$_value(central) == 0.0} {
                    set _value(max) 1.0
                }
                if {$min != ""} {
                    if {$_value(min) < $min} {
                        set $_value(min) $min
                    }
                }
                if {$max != ""} {
                    if {$_value(max) > $max} {
                        set $_value(max) $max
                    }
                }
                set _uvalue(min) $_value(min)$_units
                set _uvalue(max) $_value(max)$_units
            }
        }
        gaussian {
            #puts "GAUSSIAN min=$min max=$max"
            if {![info exists _value(central)]} {
                if {[info exists _value(min)] && [info exists _value(max)]} {
                    set _value(central) [expr {0.5*($_value(max) + $_value(min))}]
                } else {
                    set _value(central) [expr {0.5*($max + $min)}]
                }
            }
            #puts "central=$_value(central)"
            if {![info exists _value(stddev)]} {
                set _value(stddev)  [expr $_value(central) * 0.10]
            }
            if {$_value(stddev) <= 0} {
                set _value(stddev) 1.0
            }
            #puts "stddev=$_value(stddev)"
            # lower bound is -3 deviations or tool min
            set trunc [expr {$_value(central) - 3*$_value(stddev)}]
            if {$min == "" || $trunc > $min} {
                set _value(min) $trunc
            } else {
                set _value(min) $min
            }

            # upper bound is +3 deviations or tool max
            set trunc [expr {$_value(central) + 3*$_value(stddev)}]
            if {$max == "" || $trunc < $max} {
                set _value(max) $trunc
            } else {
                set _value(max) $max
            }

            set _uvalue(min) $_value(min)$_units
            set _uvalue(max) $_value(max)$_units
            set _uvalue(central) $_value(central)$_units
            set _uvalue(stddev) $_value(stddev)$_units
        }
    }

    if {$check == 1} {
        return
    }
    # pop up the panel of editing options for this mode
    foreach w [pack slaves $itk_component(vals)] {
        pack forget $w
    }
    pack $itk_component(${newval}vals) -expand yes -fill both

    switch -- $newval {
        exact {
            $itk_component(exactvals).val delete 0 end
            $itk_component(exactvals).val insert end $_uvalue(central)
            focus $itk_component(exactvals).val
        }
        uniform {
            $itk_component(uniformvals).min delete 0 end
            $itk_component(uniformvals).min insert end $_uvalue(min)
            $itk_component(uniformvals).max delete 0 end
            $itk_component(uniformvals).max insert end $_uvalue(max)
            focus $itk_component(uniformvals).min
        }
        gaussian {
            $itk_component(gaussianvals).mean delete 0 end
            $itk_component(gaussianvals).mean insert end $_uvalue(central)
            $itk_component(gaussianvals).stddev delete 0 end
            $itk_component(gaussianvals).stddev insert end $_uvalue(stddev)

            $itk_component(gaussianvals).min configure -state normal
            $itk_component(gaussianvals).min delete 0 end
            $itk_component(gaussianvals).min insert end $_uvalue(min)
            $itk_component(gaussianvals).min configure -state readonly

            $itk_component(gaussianvals).max configure -state normal
            $itk_component(gaussianvals).max delete 0 end
            $itk_component(gaussianvals).max insert end $_uvalue(max)
            $itk_component(gaussianvals).max configure -state readonly

            focus $itk_component(gaussianvals).mean
        }
    }

    set _mode $newval
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# USAGE: reset
#
# Clears all value information so that the next value application
# starts from scratch.  Without a reset, the widget carries information
# like the "central" value, in case the user changes modes and then
# changes back.  Reset discards that info and starts fresh.
# ----------------------------------------------------------------------
itcl::body Rappture::ProbDistEditor::reset {} {
    set _mode ""
    catch {unset _value}
    catch {unset _uvalue}
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to reconfigure the graph to display the current
# distribution plot.
# ----------------------------------------------------------------------
itcl::body Rappture::ProbDistEditor::_redraw {} {
    #puts "PDE::_redraw"

    set g $itk_component(graph)
    eval $g element delete [$g element names]
    eval $g mark delete [$g mark names]
    switch -- $_mode {
        exact {
            $g xaxis configure -majorticks {} -min 0 -max 1 -stepsize 0.1
            $g yaxis configure -min 0 -max 1
            $g element create delta \
                -xdata {0 0.48 0.49 0.50 0.51 0.52 1} \
                -ydata {0    0 0.05 0.93 0.05    0 0} \
                -color $itk_option(-foreground) -symbol "" \
                -areaforeground $itk_option(-fillcolor) -areapattern solid
            # $g marker create image -anchor nw -coords {0.5 0.95} \
                -xoffset -6 -yoffset -3 -image [Rappture::icon delta]
        }
        uniform {
            $g xaxis configure -majorticks {} -min 0 -max 1 -stepsize 0.1
            $g yaxis configure -min 0 -max 1

            # uniform distribution throughout
            $g element create uniform \
                -xdata {-0.1 -0.1 1.1 1.1} -ydata {-0.1 0.5 0.5 -0.1} \
                -color $itk_option(-foreground) -symbol "" \
                -areaforeground $itk_option(-fillcolor) -areapattern solid

            # left/right limits
            $g marker create polygon \
                -coords {-0.1 -0.1 0.1 -0.1 0.1 1.1 -0.1 1.1} \
                -outline gray -linewidth 1 -fill gray -stipple gray25
            $g marker create polygon \
                -coords {1.1 -0.1 0.9 -0.1 0.9 1.1 1.1 1.1} \
                -outline gray -linewidth 1 -fill gray -stipple gray25
        }
        gaussian {
            #puts "min=$_value(min) max=$_value(max)"
            #puts "mean=$_value(central) dev=$_value(stddev)"

            set min $_value(min)
            set max $_value(max)
            set dx [expr {$max - $min}]
            set realmin [expr {$min - 0.1 * $dx}]
            set realmax [expr {$max + 0.1 * $dx}]
            set mean $_value(central)
            set sig2 [expr {$_value(stddev) * $_value(stddev)}]
            $g xaxis configure -min $realmin -max $realmax \
                -majorticks [list $min $mean $max]
            $g yaxis configure -min 0 -max 1.1
            set xdata ""
            set ydata ""
            for {set n 0} {$n <= 50} {incr n} {
                set x [expr {($realmax - $realmin) / 50.0 * $n + $realmin}]
                lappend xdata $x
                set expon [expr {-0.5 * ($x - $mean) * ($x - $mean) / $sig2}]
                if {$expon < -200} {
                    lappend ydata 0.0
                } else {
                    lappend ydata [expr {exp($expon)}]
                }
            }
            $g element create bellcurve \
                -xdata $xdata -ydata $ydata \
                -color $itk_option(-foreground) -symbol "" \
                -areaforeground $itk_option(-fillcolor) -areapattern solid

            # left/right limits
            set x0m [expr {$min - 0.2 * $dx}]
            set x0 $min
            set x1 $max
            set x1p [expr {$max + 0.2 * $dx}]
            $g marker create polygon \
                -coords [list $x0m -0.1 $x0 -0.1 $x0 1.2 $x0m 1.2] \
                -outline gray -linewidth 1 -fill gray -stipple gray25
            $g marker create polygon \
                -coords [list $x1p -0.1 $x1 -0.1 $x1 1.2 $x1p 1.2] \
                -outline gray -linewidth 1 -fill gray -stipple gray25

            # lines for standard deviation
            set x0 [expr {$_value(central)-$_value(stddev)}]
            set x1 [expr {$_value(central)+$_value(stddev)}]
            $g marker create line \
                -coords [list $x0 -0.1 $x0 1.2] -linewidth 1 \
                -outline gray -dashes 2
            $g marker create line \
                -coords [list $x1 -0.1 $x1 1.2] -linewidth 1 \
                -outline gray -dashes 2
        }
        custom {
        }
        default {
            error "don't know how to redraw for mode \"$_mode\""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _axisLabels <which> <graph> <val>
#
# Used internally to generate axis labels for the x- and y-axes of
# the graph.  The y-axis doesn't usually have labels since the values
# aren't meaningful.  The x-axis should show values according to the
# current mode of input.
# ----------------------------------------------------------------------
itcl::body Rappture::ProbDistEditor::_axisLabels {which graph val} {
    switch -- $which {
        xaxis {
            switch -- $_mode {
                exact {
                    if {$val == 0.5} {
                        return $_value(central)
                    }
                    return ""
                }
                uniform {
                    if {$val == 0.1} {
                        return [expr {$_value(min) < $_value(max)
                            ? $_value(min) : $_value(max)}]
                    } elseif {$val == 0.9} {
                        return [expr {$_value(min) > $_value(max)
                            ? $_value(min) : $_value(max)}]
                    }
                    return ""
                }
                gaussian {
                    return $val
                }
            }
        }
        yaxis {
            return ""  ;# no labels for y-axis values
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _apply bindings <widget>
# USAGE: _apply value <widget>
# USAGE: _apply finalize
#
# Called automatically whenever a value is edited to check/apply the
# value and redraw the PDF plot.  If the value is bad, then it is
# reloaded for editing.  Otherwise, it is applied.
# ----------------------------------------------------------------------
itcl::body Rappture::ProbDistEditor::_apply {op {widget ""}} {
    #puts "PDE::_apply $op $widget"
    if {$op != "bindings"} {
        # need this for value/finalize
        array set w2uvalue [list \
            $itk_component(exactvals).val _uvalue(central) \
            $itk_component(uniformvals).min _uvalue(min) \
            $itk_component(uniformvals).max _uvalue(max) \
            $itk_component(gaussianvals).mean _uvalue(central) \
            $itk_component(gaussianvals).stddev _uvalue(stddev) \
            $itk_component(gaussianvals).min _uvalue(min) \
            $itk_component(gaussianvals).max _uvalue(max) \
        ]
        array set w2value [list \
            $itk_component(exactvals).val _value(central) \
            $itk_component(uniformvals).min _value(min) \
            $itk_component(uniformvals).max _value(max) \
            $itk_component(gaussianvals).mean _value(central) \
            $itk_component(gaussianvals).stddev _value(stddev) \
            $itk_component(gaussianvals).min _value(min) \
            $itk_component(gaussianvals).max _value(max) \
        ]
    }

    if {$op == "finalize"} {
        # one of our widgets has focus?
        set widget [focus]
        if {![info exists w2value($widget)]} {
            # nope -- nevermind, nothing to do
            return
        }
        # yes -- continue onward trying to finalize the value in that widget
        set op "value"
    }

    switch -- $op {
        bindings {
            bind $widget <KeyPress-Tab> [itcl::code $this _apply value %W]
            bind $widget <KeyPress-Return> [itcl::code $this _apply value %W]
            bind $widget <FocusOut> [itcl::code $this _apply value %W]
        }
        value {
            set var $w2value($widget)
            set uvar $w2uvalue($widget)
            if {$_units == ""} {
                set newval [$widget get]
            } else {
                set newval [Rappture::Units::convert [$widget get] -context $_units]
            }
            set itk $itk_component(gaussianvals).stddev
            if {$widget != $itk && [catch {Rappture::Units::mcheck_range $newval $_umin $_umax $_units} err]} {
                # oops! value is bad -- edit again
                bell
                Rappture::Tooltip::cue $widget $err
                $widget delete 0 end
                $widget insert end [set $var]
                $widget select from 0
                $widget select to end
                focus $widget
                return -code break  ;# stop other bindings like tab-traversal
            }
            $widget delete 0 end
            $widget insert end $newval
            set $uvar $newval

            #if {$widget == $itk_component(gaussianvals).mean} {
                # need new min and max
                #set $_uvalue(min)
                #set $_uvalue(max) ...
            #}

            if {[catch {Rappture::Units::convert $newval -units off} nvar]} {
                set nvar $newval
            }
            set $var $nvar

            mode check
            $_dispatcher event -idle !redraw
        }
        default {
            error "bad option \"$opt\": should be bindings, value"
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -fillcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::ProbDistEditor::fillcolor {
    $_dispatcher event -idle !redraw
}
