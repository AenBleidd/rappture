# ----------------------------------------------------------------------
#  COMPONENT: xyresult - X/Y plot in a ResultSet
#
#  This widget is an X/Y plot, meant to view line graphs produced
#  as output from the run of a Rappture tool.  Use the "add" and
#  "delete" methods to control the curves showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *XyResult.width 4i widgetDefault
option add *XyResult.height 4i widgetDefault
option add *XyResult.gridColor #d9d9d9 widgetDefault
option add *XyResult.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::XyResult {
    inherit itk::Widget

    itk_option define -gridcolor gridColor GridColor ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {curve {settings ""}}
    public method delete {args}
    public method scale {args}

    protected method _rebuild {}
    protected method _fixLimits {}

    private variable _clist ""     ;# list of curve objects
    private variable _curve2color  ;# maps curve => plotting color
    private variable _curve2width  ;# maps curve => line width
    private variable _curve2raise  ;# maps curve => raise flag 0/1
    private variable _curve2elems  ;# maps curve => elements on graph
    private variable _xmin ""      ;# autoscale min for x-axis
    private variable _xmax ""      ;# autoscale max for x-axis
    private variable _ymin ""      ;# autoscale min for y-axis
    private variable _ymax ""      ;# autoscale max for y-axis
}
                                                                                
itk::usual XyResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add plot {
        blt::graph $itk_interior.plot \
            -highlightthickness 0 -plotpadx 0 -plotpady 0 \
            -rightmargin 10
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(plot) -expand yes -fill both

    Blt_ZoomStack $itk_component(plot)
    $itk_component(plot) legend configure -hide yes

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::destructor {} {
}

# ----------------------------------------------------------------------
# USAGE: add <curve> ?<settings>?
#
# Clients use this to add a curve to the plot.  The optional <settings>
# are used to configure the plot.  Allowed settings are -color, -width,
# and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::add {curve {settings ""}} {
    array set params {
        -color black
        -width 1
        -raise 0
    }
    foreach {opt val} $settings {
        if {![info exists params($opt)]} {
            error "bad setting \"$opt\": should be [join [lsort [array names params]] {, }]"
        }
        set params($opt) $val
    }

    set pos [lsearch -exact $curve $_clist]
    if {$pos < 0} {
        lappend _clist $curve
        set _curve2color($curve) $params(-color)
        set _curve2width($curve) $params(-width)
        set _curve2raise($curve) $params(-raise)

        after cancel [itcl::code $this _rebuild]
        after idle [itcl::code $this _rebuild]
    }
}

# ----------------------------------------------------------------------
# USAGE: delete ?<curve1> <curve2> ...?
#
# Clients use this to delete a curve from the plot.  If no curves
# are specified, then all curves are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::delete {args} {
    if {[llength $args] == 0} {
        set args $_clist
    }

    # delete all specified curves
    set changed 0
    foreach curve $args {
        set pos [lsearch -exact $_clist $curve]
        if {$pos >= 0} {
            set _clist [lreplace $_clist $pos $pos]
            catch {unset _curve2color($curve)}
            catch {unset _curve2width($curve)}
            catch {unset _curve2raise($curve)}
            catch {unset _curve2elems($curve)}
            set changed 1
        }
    }

    # if anything changed, then rebuild the plot
    if {$changed} {
        after cancel [itcl::code $this _rebuild]
        after idle [itcl::code $this _rebuild]
    }
}

# ----------------------------------------------------------------------
# USAGE: scale ?<curve1> <curve2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <curve> objects.  This
# accounts for all curves--even those not showing on the screen.
# Because of this, the limits are appropriate for all curves as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::scale {args} {
    set _xmin ""
    set _xmax ""
    set _ymin ""
    set _ymax ""
    foreach obj $args {
        foreach axis {x y} {
            foreach {min max} [$obj limits $axis] break
            if {"" != $min && "" != $max} {
                if {"" == [set _${axis}min]} {
                    set _${axis}min $min
                    set _${axis}max $max
                } else {
                    if {$min < [set _${axis}min]} {
                        set _${axis}min $min
                    }
                    if {$max > [set _${axis}max]} {
                        set _${axis}max $max
                    }
                }
            }
        }
    }
    _fixLimits
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::_rebuild {} {
    set g $itk_component(plot)

    # first clear out the widget
    eval $g element delete [$g element names]
    $g axis configure y -min "" -max ""

    # extract axis information from the first curve
    set xydata [lindex $_clist 0]
    if {$xydata != ""} {
        set title [$xydata hints label]
        if {"" != $title} {
            $g configure -title $title
        }

        set legend [$xydata hints legend]
        if {"" != $legend} {
            if {$legend == "off"} {
                $g legend configure -hide yes
            } else {
                $g legend configure -hide no \
                    -position plotarea -anchor $legend -borderwidth 0
            }
        }

        set xlabel [$xydata hints xlabel]
        if {"" != $xlabel} {
            $g xaxis configure -title $xlabel
        }

        set ylabel [$xydata hints ylabel]
        if {"" != $ylabel} {
            $g yaxis configure -title $ylabel
        }
    }

    # plot all of the curves
    set count 0
    foreach xydata $_clist {
        set _curve2elems($xydata) ""

        foreach comp [$xydata components] {
            set xv [$xydata mesh $comp]
            set yv [$xydata values $comp]

            if {[info exists _curve2color($xydata)]} {
                set color $_curve2color($xydata)
            } else {
                set color [$xydata hints color]
                if {"" == $color} {
                    set color black
                }
            }

            if {[info exists _curve2width($xydata)]} {
                set lwidth $_curve2width($xydata)
            } else {
                set lwidth 2
            }

            set elem "elem[incr count]"
            lappend _curve2elems($xydata) $elem

            set label [$xydata hints label]
            $g element create $elem -x $xv -y $yv \
                -symbol "" -linewidth $lwidth -label $label -color $color

            set style [$xydata hints style]
            if {$style != ""} {
                eval $g element configure $elem $style
            }
        }
    }

    # raise those tagged to be on top
    set dlist [$g element show]
    foreach xydata $_clist {
        if {[info exists _curve2raise($xydata)] && $_curve2raise($xydata)} {
            foreach elem $_curve2elems($xydata) {
                set i [lsearch -exact $dlist $elem]
                if {$i >= 0} {
                    # move element to end of display list
                    set dlist [lreplace $dlist $i $i]
                    lappend dlist $elem
                }
            }
        }
    }
    $g element show $dlist

    _fixLimits
}

# ----------------------------------------------------------------------
# USAGE: _fixLimits
#
# Used internally to apply automatic limits to the axes for the
# current plot.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::_fixLimits {} {
    set g $itk_component(plot)

    #
    # HACK ALERT!
    # Use this code to fix up the y-axis limits for the BLT graph.
    # The auto-limits don't always work well.  We want them to be
    # set to a "nice" number slightly above or below the min/max
    # limits.
    #
    $g axis configure x -min $_xmin -max $_xmax

    if {"" != $_ymin && "" != $_ymax} {
        set min $_ymin
        set max $_ymax
        set log [$g axis cget y -logscale]
        if {$log} {
            if {$min == $max} {
                set min [expr {0.9*$min}]
                set max [expr {1.1*$max}]
            }
            set min [expr {pow(10.0,floor(log10($min)))}]
            set max [expr {pow(10.0,ceil(log10($max)))}]
        } else {
            if {$min > 0} {
                set min [expr {0.95*$min}]
            } else {
                set min [expr {1.05*$min}]
            }
            if {$max > 0} {
                set max [expr {1.05*$max}]
            } else {
                set max [expr {0.95*$max}]
            }
        }
        $g axis configure y -min $min -max $max
    } else {
        $g axis configure y -min "" -max ""
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -gridcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::XyResult::gridcolor {
    if {"" == $itk_option(-gridcolor)} {
        $itk_component(plot) grid off
    } else {
        $itk_component(plot) grid configure -color $itk_option(-gridcolor)
        $itk_component(plot) grid on
    }
}
