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
option add *XyResult.activeColor blue widgetDefault
option add *XyResult.controlBackground gray widgetDefault
option add *XyResult.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

option add *XyResult*Balloon*Entry.background white widgetDefault

blt::bitmap define XyResult-reset {
#define reset_width 12
#define reset_height 12
static unsigned char reset_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02,
   0x04, 0x02, 0x04, 0x02, 0x04, 0x02, 0xfc, 0x03, 0x00, 0x00, 0x00, 0x00};
}

blt::bitmap define XyResult-dismiss {
#define dismiss_width 10
#define dismiss_height 8
static unsigned char dismiss_bits[] = {
   0x87, 0x03, 0xce, 0x01, 0xfc, 0x00, 0x78, 0x00, 0x78, 0x00, 0xfc, 0x00,
   0xce, 0x01, 0x87, 0x03};
}


itcl::class Rappture::XyResult {
    inherit itk::Widget

    itk_option define -gridcolor gridColor GridColor ""
    itk_option define -activecolor activeColor ActiveColor ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {curve {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method download {}

    protected method _rebuild {}
    protected method _fixLimits {}
    protected method _zoom {option args}
    protected method _hilite {state x y}
    protected method _axis {option args}

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
    private variable _hilite       ;# info from last _hilite operation
    private variable _axis         ;# info for axis being edited
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
            -bitmap XyResult-reset \
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
    $itk_component(plot) pen configure activeLine -symbol square -pixels 5
    $itk_component(plot) element bind all <Enter> \
        {%W element activate [%W element get current]}
    $itk_component(plot) element bind all <Leave> \
        {%W element deactivate [%W element get current]}

    #
    # Add bindings so you can mouse over points to see values:
    #
    array set _hilite {
        elem ""
        color ""
    }
    bind $itk_component(plot) <Motion> \
        [itcl::code $this _hilite at %x %y]
    bind $itk_component(plot) <Leave> \
        [itcl::code $this _hilite off %x %y]

    #
    # Add support for editing axes:
    #
    Rappture::Balloon $itk_component(hull).axes
    set inner [$itk_component(hull).axes component inner]
    set inner [frame $inner.bd -borderwidth 4 -relief flat]
    pack $inner -expand yes -fill both

    button $inner.dismiss -bitmap XyResult-dismiss \
        -relief flat -overrelief raised -command "
          Rappture::Tooltip::cue hide
          [list $itk_component(hull).axes deactivate]
        "
    grid $inner.dismiss -row 0 -column 1 -sticky e

    label $inner.labell -text "Label:"
    entry $inner.label -width 15 -highlightbackground $itk_option(-background)
    grid $inner.labell -row 1 -column 0 -sticky e
    grid $inner.label -row 1 -column 1 -sticky ew -pady 4

    label $inner.minl -text "Minimum:"
    entry $inner.min -width 15 -highlightbackground $itk_option(-background)
    grid $inner.minl -row 2 -column 0 -sticky e
    grid $inner.min -row 2 -column 1 -sticky ew -pady 4

    label $inner.maxl -text "Maximum:"
    entry $inner.max -width 15 -highlightbackground $itk_option(-background)
    grid $inner.maxl -row 3 -column 0 -sticky e
    grid $inner.max -row 3 -column 1 -sticky ew -pady 4

    label $inner.formatl -text "Format:"
    Rappture::Combobox $inner.format -width 15 -editable no
    $inner.format choices insert end \
        "%.3g"  "Auto"         \
        "%.1f"  "X.X"          \
        "%.2f"  "X.XX"         \
        "%.3f"  "X.XXX"        \
        "%.6f"  "X.XXXXXX"     \
        "%.1e"  "X.Xe+XX"      \
        "%.2e"  "X.XXe+XX"     \
        "%.3e"  "X.XXXe+XX"    \
        "%.6e"  "X.XXXXXXe+XX"
    grid $inner.formatl -row 4 -column 0 -sticky e
    grid $inner.format -row 4 -column 1 -sticky ew -pady 4

    label $inner.scalel -text "Scale:"
    frame $inner.scales
    radiobutton $inner.scales.linear -text "Linear" \
        -variable [itcl::scope _axis(scale)] -value "linear"
    pack $inner.scales.linear -side left
    radiobutton $inner.scales.log -text "Logarithmic" \
        -variable [itcl::scope _axis(scale)] -value "log"
    pack $inner.scales.log -side left
    grid $inner.scalel -row 5 -column 0 -sticky e
    grid $inner.scales -row 5 -column 1 -sticky ew -pady 4

    foreach axis {x y} {
        $itk_component(plot) axis bind $axis <Enter> \
            [itcl::code $this _axis hilite $axis on]
        $itk_component(plot) axis bind $axis <Leave> \
            [itcl::code $this _axis hilite $axis off]
        $itk_component(plot) axis bind $axis <ButtonPress> \
            [itcl::code $this _axis edit $axis]
    }

    set _axis(format-x) "%.3g"
    set _axis(format-y) "%.3g"
    _axis scale x linear
    _axis scale y linear

    # quick-and-dirty zoom functionality, for now...
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
# USAGE: download
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::download {} {
    set psdata [$itk_component(plot) postscript output -maxpect 1]

    set cmds {
        set fout "xy[pid].pdf"
        exec ps2pdf - $fout << $psdata

        set fid [open $fout r]
        fconfigure $fid -translation binary -encoding binary
        set pdfdata [read $fid]
        close $fid

        file delete -force $fout
    }
    if {[catch $cmds result] == 0} {
        return [list .pdf $pdfdata]
    }
    return [list .ps $psdata]
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

    $g axis configure x -min "" -max ""
    _axis scale x linear

    $g axis configure y -min "" -max ""
    _axis scale y linear

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
                _axis scale x log
            }
            if {[info exists hints(yscale)] && $hints(yscale) == "log"} {
                _axis scale y log
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
    set g $itk_component(plot)
    if {$state == "at"} {
        if {[$g element closest $x $y info]} {
            set elem $info(name)
            set x [$g axis transform x $info(x)]
            set y [$g axis transform y $info(y)]
            set state 1
        } else {
            set state 0
        }
    }

    if {$state} {
        $g crosshairs configure -hide no -position @$x,$y
        #
        # Highlight ON:
        # - fatten line
        # - change color
        # - pop up tooltip about data
        #
        if {"" == $_hilite(elem)} {
            set t [$g element cget $elem -linewidth]
            $g element configure $elem -linewidth [expr {$t+2}]
            set _hilite(elem) $elem
        }

        set tip ""
        if {[info exists _elem2curve($elem)]} {
            set curve $_elem2curve($elem)
            set tip [$curve hints tooltip]
            if {[info exists info(y)]} {
                set val [_axis format y dummy $info(y)]
                set units [$curve hints yunits]
                append tip "\n$val$units"

                if {[info exists info(x)]} {
                    set val [_axis format x dummy $info(x)]
                    set units [$curve hints xunits]
                    append tip " @ $val$units"
                }
            }
            set tip [string trim $tip]
        }
        if {"" != $tip} {
            if {$x > 0.5*[winfo width $g]} {
                set x "-[expr {$x-4}]"  ;# move tooltip to the left
            } else {
                set x "+[expr {$x+4}]"  ;# move tooltip to the right
            }
            if {$y > 0.5*[winfo height $g]} {
                set y "-[expr {$y-4}]"  ;# move tooltip to the top
            } else {
                set y "+[expr {$y+4}]"  ;# move tooltip to the bottom
            }
            Rappture::Tooltip::text $g $tip
            Rappture::Tooltip::tooltip show $g $x,$y
        }
    } else {
        #
        # Highlight OFF:
        # - put line width back to normal
        # - put color back to normal
        # - take down tooltip
        #
        $g crosshairs configure -hide yes

        if {"" != $_hilite(elem)} {
            set t [$g element cget $_hilite(elem) -linewidth]
            $g element configure $_hilite(elem) -linewidth [expr {$t-2}]
            set _hilite(elem) ""
        }
        Rappture::Tooltip::tooltip cancel
    }
}

# ----------------------------------------------------------------------
# USAGE: _axis hilite <axis> <state>
# USAGE: _axis edit <axis>
# USAGE: _axis changed <axis> <what>
# USAGE: _axis format <axis> <widget> <value>
# USAGE: _axis scale <axis> linear|log
#
# Used internally to handle editing of the x/y axes.  The hilite
# operation causes the axis to light up.  The edit operation pops
# up a panel with editing options.  The changed operation applies
# changes from the panel.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::_axis {option args} {
    set inner [$itk_component(hull).axes component inner].bd

    switch -- $option {
        hilite {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_axis hilite axis state\""
            }
            set axis [lindex $args 0]
            set state [lindex $args 1]

            if {$state} {
                $itk_component(plot) axis configure $axis \
                    -color $itk_option(-activecolor) \
                    -titlecolor $itk_option(-activecolor)
            } else {
                $itk_component(plot) axis configure $axis \
                    -color $itk_option(-foreground) \
                    -titlecolor $itk_option(-foreground)
            }
        }
        edit {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"_axis edit axis\""
            }
            set axis [lindex $args 0]
            set _axis(current) $axis

            # apply last value when deactivating
            $itk_component(hull).axes configure -deactivatecommand \
                [itcl::code $this _axis changed $axis focus]

            # fix axis label controls...
            set label [$itk_component(plot) axis cget $axis -title]
            $inner.label delete 0 end
            $inner.label insert end $label
            bind $inner.label <KeyPress-Return> \
                [itcl::code $this _axis changed $axis label]
            bind $inner.label <FocusOut> \
                [itcl::code $this _axis changed $axis label]

            # fix min/max controls...
            foreach {min max} [$itk_component(plot) axis limits $axis] break
            $inner.min delete 0 end
            $inner.min insert end $min
            bind $inner.min <KeyPress-Return> \
                [itcl::code $this _axis changed $axis min]
            bind $inner.min <FocusOut> \
                [itcl::code $this _axis changed $axis min]

            $inner.max delete 0 end
            $inner.max insert end $max
            bind $inner.max <KeyPress-Return> \
                [itcl::code $this _axis changed $axis max]
            bind $inner.max <FocusOut> \
                [itcl::code $this _axis changed $axis max]

            # fix format control...
            set fmts [$inner.format choices get -value]
            set i [lsearch -exact $fmts $_axis(format-$axis)]
            if {$i < 0} { set i 0 }  ;# use Auto choice
            $inner.format value [$inner.format choices get -label $i]

            bind $inner.format <<Value>> \
                [itcl::code $this _axis changed $axis format]

            # fix scale control...
            if {[$itk_component(plot) axis cget $axis -logscale]} {
                set _axis(scale) "log"
                $inner.format configure -state disabled
            } else {
                set _axis(scale) "linear"
                $inner.format configure -state normal
            }
            $inner.scales.linear configure \
                -command [itcl::code $this _axis changed $axis scale]
            $inner.scales.log configure \
                -command [itcl::code $this _axis changed $axis scale]

            #
            # Figure out where the window should pop up.
            #
            set x [winfo rootx $itk_component(plot)]
            set y [winfo rooty $itk_component(plot)]
            set w [winfo width $itk_component(plot)]
            set h [winfo height $itk_component(plot)]
            foreach {x0 y0 pw ph} [$itk_component(plot) extents plotarea] break
            switch -- $axis {
                x {
                    set x [expr {round($x + $x0+0.5*$pw)}]
                    set y [expr {round($y + $y0+$ph + 0.5*($h-$y0-$ph))}]
                    set dir "above"
                }
                y {
                    set x [expr {round($x + 0.5*$x0)}]
                    set y [expr {round($y + $y0+0.5*$ph)}]
                    set dir "right"
                }
            }
            $itk_component(hull).axes activate @$x,$y $dir
        }
        changed {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_axis changed axis what\""
            }
            set axis [lindex $args 0]
            set what [lindex $args 1]
            if {$what == "focus"} {
                set what [focus]
                if {[winfo exists $what]} {
                    set what [winfo name $what]
                }
            }

            switch -- $what {
                label {
                    set val [$inner.label get]
                    $itk_component(plot) axis configure $axis -title $val
                }
                min {
                    set val [$inner.min get]
                    if {![string is double -strict $val]} {
                        Rappture::Tooltip::cue $inner.min "Must be a number"
                        bell
                        return
                    }

                    set max [lindex [$itk_component(plot) axis limits $axis] 1]
                    if {$val >= $max} {
                        Rappture::Tooltip::cue $inner.min "Must be <= max ($max)"
                        bell
                        return
                    }
                    catch {
                        # can fail in log mode
                        $itk_component(plot) axis configure $axis -min $val
                    }
                    foreach {min max} [$itk_component(plot) axis limits $axis] break
                    $inner.min delete 0 end
                    $inner.min insert end $min
                }
                max {
                    set val [$inner.max get]
                    if {![string is double -strict $val]} {
                        Rappture::Tooltip::cue $inner.max "Should be a number"
                        bell
                        return
                    }

                    set min [lindex [$itk_component(plot) axis limits $axis] 0]
                    if {$val <= $min} {
                        Rappture::Tooltip::cue $inner.max "Must be >= min ($min)"
                        bell
                        return
                    }
                    catch {
                        # can fail in log mode
                        $itk_component(plot) axis configure $axis -max $val
                    }
                    foreach {min max} [$itk_component(plot) axis limits $axis] break
                    $inner.max delete 0 end
                    $inner.max insert end $max
                }
                format {
                    set fmt [$inner.format translate [$inner.format value]]
                    set _axis(format-$axis) $fmt

                    # force a refresh
                    $itk_component(plot) axis configure $axis -min \
                        [$itk_component(plot) axis cget $axis -min]
                }
                scale {
                    _axis scale $axis $_axis(scale)

                    if {$_axis(scale) == "log"} {
                        $inner.format configure -state disabled
                    } else {
                        $inner.format configure -state normal
                    }

                    foreach {min max} [$itk_component(plot) axis limits $axis] break
                    $inner.min delete 0 end
                    $inner.min insert end $min
                    $inner.max delete 0 end
                    $inner.max insert end $max
                }
                default {
                    # be lenient so we can handle the "focus" case
                }
            }
        }
        format {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"_axis format axis widget value\""
            }
            set axis [lindex $args 0]
            set value [lindex $args 2]

            if {[$itk_component(plot) axis cget $axis -logscale]} {
                set fmt "%.3g"
            } else {
                set fmt $_axis(format-$axis)
            }
            return [format $fmt $value]
        }
        scale {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_axis scale axis type\""
            }
            set axis [lindex $args 0]
            set type [lindex $args 1]

            if {$type == "log"} {
                catch {$itk_component(plot) axis configure $axis -logscale 1}
                # leave format alone in log mode
                $itk_component(plot) axis configure $axis -command ""
            } else {
                catch {$itk_component(plot) axis configure $axis -logscale 0}
                # use special formatting for linear mode
                $itk_component(plot) axis configure $axis -command \
                    [itcl::code $this _axis format $axis]
            }
        }
        default {
            error "bad option \"$option\": should be changed, edit, hilite, or format"
        }
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
