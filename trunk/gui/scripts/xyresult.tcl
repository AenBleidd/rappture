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
option add *XyResult.hiliteColor black widgetDefault
option add *XyResult.controlBackground gray widgetDefault
option add *XyResult.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

blt::bitmap define ContourResult-reset {
#define reset_width 12
#define reset_height 12
static unsigned char reset_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02,
   0x04, 0x02, 0x04, 0x02, 0x04, 0x02, 0xfc, 0x03, 0x00, 0x00, 0x00, 0x00};
}

itcl::class Rappture::XyResult {
    inherit itk::Widget

    itk_option define -gridcolor gridColor GridColor ""
    itk_option define -hilitecolor hiliteColor HiliteColor ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {curve {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}

    protected method _rebuild {}
    protected method _fixLimits {}
    protected method _zoom {option args}
    protected method _hilite {state x y}

    private variable _clist ""     ;# list of curve objects
    private variable _curve2color  ;# maps curve => plotting color
    private variable _curve2width  ;# maps curve => line width
    private variable _curve2dashes ;# maps curve => BLT -dashes list
    private variable _curve2raise  ;# maps curve => raise flag 0/1
    private variable _elem2curve   ;# maps graph element => curve
    private variable _xmin ""      ;# autoscale min for x-axis
    private variable _xmax ""      ;# autoscale max for x-axis
    private variable _vmin ""      ;# autoscale min for y-axis
    private variable _vmax ""      ;# autoscale max for y-axis
    private variable _hilite ""    ;# info from last _hilite operation
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

    itk_component add controls {
        frame $itk_interior.cntls
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(controls) -side right -fill y

    itk_component add reset {
        button $itk_component(controls).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap ContourResult-reset \
            -command [itcl::code $this _zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"


    itk_component add plot {
        blt::graph $itk_interior.plot \
            -highlightthickness 0 -plotpadx 0 -plotpady 0 \
            -rightmargin 10
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(plot) -expand yes -fill both

    # special pen for highlighting active traces
    $itk_component(plot) element bind all <Enter> \
        [itcl::code $this _hilite on %x %y]
    $itk_component(plot) element bind all <Leave> \
        [itcl::code $this _hilite off %x %y]

    bind $itk_component(plot) <Leave> \
        [list Rappture::Tooltip::tooltip cancel]

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
# are used to configure the plot.  Allowed settings are -color,
# -brightness, -width, -linestyle and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::add {curve {settings ""}} {
    array set params {
        -color black
        -brightness 0
        -width 1
        -raise 0
        -linestyle solid
    }
    foreach {opt val} $settings {
        if {![info exists params($opt)]} {
            error "bad setting \"$opt\": should be [join [lsort [array names params]] {, }]"
        }
        set params($opt) $val
    }

    # convert -linestyle to BLT -dashes
    switch -- $params(-linestyle) {
        dashed { set params(-linestyle) {4 4} }
        dotted { set params(-linestyle) {2 4} }
        default { set params(-linestyle) {} }
    }

    # if -brightness is set, then update the color
    if {$params(-brightness) != 0} {
        set params(-color) [Rappture::color::brightness \
            $params(-color) $params(-brightness)]
    }

    set pos [lsearch -exact $curve $_clist]
    if {$pos < 0} {
        lappend _clist $curve
        set _curve2color($curve) $params(-color)
        set _curve2width($curve) $params(-width)
        set _curve2dashes($curve) $params(-linestyle)
        set _curve2raise($curve) $params(-raise)

        after cancel [itcl::code $this _rebuild]
        after idle [itcl::code $this _rebuild]
    }
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::get {} {
    # put the dataobj list in order according to -raise options
    set clist $_clist
    foreach obj $clist {
        if {[info exists _curve2raise($obj)] && $_curve2raise($obj)} {
            set i [lsearch -exact $clist $obj]
            if {$i >= 0} {
                set clist [lreplace $clist $i $i]
                lappend clist $obj
            }
        }
    }
    return $clist
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
            catch {unset _curve2dashes($curve)}
            catch {unset _curve2raise($curve)}
            foreach elem [array names _elem2curve] {
                if {$_elem2curve($elem) == $curve} {
                    unset _elem2curve($elem)
                }
            }
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
    set _vmin ""
    set _vmax ""
    foreach obj $args {
        foreach axis {x v} {
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
    $g axis configure x -min "" -max "" -logscale 0
    $g axis configure y -min "" -max "" -logscale 0

    # extract axis information from the first curve
    set clist [get]
    set xydata [lindex $clist 0]
    if {$xydata != ""} {
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

    foreach lim {xmin xmax ymin ymax} {
        set limits($lim) ""
    }

    # plot all of the curves
    set count 0
    foreach xydata $clist {
        foreach comp [$xydata components] {
            catch {unset hints}
            array set hints [$xydata hints]

            set xv [$xydata mesh $comp]
            set yv [$xydata values $comp]

            if {[info exists _curve2color($xydata)]} {
                set color $_curve2color($xydata)
            } else {
                if {[info exists hints(color)]} {
                    set color $hints(color)
                } else {
                    set color black
                }
            }

            if {[info exists _curve2width($xydata)]} {
                set lwidth $_curve2width($xydata)
            } else {
                set lwidth 2
            }

            if {[info exists _curve2dashes($xydata)]} {
                set dashes $_curve2dashes($xydata)
            } else {
                set dashes ""
            }

            if {[$xv length] <= 1} {
                set sym square
            } else {
                set sym ""
            }

            set elem "elem[incr count]"
            set _elem2curve($elem) $xydata

            if {[info exists hints(label)]} {
                set label $hints(label)
            } else {
                set label ""
            }
            $g element create $elem -x $xv -y $yv \
                -symbol $sym -pixels 6 -linewidth $lwidth -label $label \
                -color $color -dashes $dashes

            if {[info exists hints(xscale)] && $hints(xscale) == "log"} {
                $g xaxis configure -logscale 1
            }
            if {[info exists hints(yscale)] && $hints(yscale) == "log"} {
                $g yaxis configure -logscale 1
            }

            # see if there are any hints on limit
            foreach lim {xmin xmax ymin ymax} {
                if {[info exists hints($lim)] && "" != $hints($lim)} {
                    set limits($lim) $hints($lim)
                }
            }
        }
    }

    # add any limit directives from the curve objects
    foreach lim {xmin xmax ymin ymax} var {_xmin _xmax _vmin _vmax} {
        if {"" != $limits($lim)} {
            set $var $limits($lim)
        }
    }
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
    if {$_xmin != $_xmax} {
        $g axis configure x -min $_xmin -max $_xmax
    } else {
        $g axis configure x -min "" -max ""
    }

    if {"" != $_vmin && "" != $_vmax} {
        set min $_vmin
        set max $_vmax
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
        if {$min != $max} {
            $g axis configure y -min $min -max $max
        } else {
            $g axis configure y -min "" -max ""
        }
    } else {
        $g axis configure y -min "" -max ""
    }
}

# ----------------------------------------------------------------------
# USAGE: _zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::_zoom {option args} {
    switch -- $option {
        reset {
            _fixLimits
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _hilite <state> <x> <y>
#
# Called automatically when the user brushes one of the elements
# on the plot.  Causes the element to highlight and a tooltip to
# pop up with element info.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::_hilite {state x y} {
    set elem [$itk_component(plot) element get current]
    if {$state} {
        #
        # Highlight ON:
        # - fatten line
        # - change color
        # - pop up tooltip about data
        #
        set t [$itk_component(plot) element cget $elem -linewidth]
        $itk_component(plot) element configure $elem -linewidth [expr {$t+2}]

        set _hilite [$itk_component(plot) element cget $elem -color]
        $itk_component(plot) element configure $elem \
            -color $itk_option(-hilitecolor)

        set tip ""
        if {[info exists _elem2curve($elem)]} {
            set curve $_elem2curve($elem)
            set tip [$curve hints tooltip]
        }
        if {"" != $tip} {
            set x [expr {$x+4}]  ;# move the tooltip over a bit
            set y [expr {$y+4}]
            Rappture::Tooltip::text $itk_component(plot) $tip
            Rappture::Tooltip::tooltip show $itk_component(plot) +$x,$y
        }
    } else {
        #
        # Highlight OFF:
        # - put line width back to normal
        # - put color back to normal
        # - take down tooltip
        #
        set t [$itk_component(plot) element cget $elem -linewidth]
        $itk_component(plot) element configure $elem -linewidth [expr {$t-2}]

        if {"" != $_hilite} {
            $itk_component(plot) element configure $elem -color $_hilite
        }
        Rappture::Tooltip::tooltip cancel
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
