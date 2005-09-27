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

option add *XyResult.width 3i widgetDefault
option add *XyResult.height 3i widgetDefault
option add *XyResult.gridColor #d9d9d9 widgetDefault
option add *XyResult.activeColor blue widgetDefault
option add *XyResult.dimColor gray widgetDefault
option add *XyResult.controlBackground gray widgetDefault
option add *XyResult.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

option add *XyResult.autoColors {
    #0000ff #ff0000 #00cc00
    #cc00cc #ff9900 #cccc00
    #000080 #800000 #006600
    #660066 #996600 #666600
} widgetDefault

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
    itk_option define -dimcolor dimColor DimColor ""
    itk_option define -autocolors autoColors AutoColors ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {curve {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method download {}

    protected method _rebuild {}
    protected method _resetLimits {}
    protected method _zoom {option args}
    protected method _hilite {state x y}
    protected method _axis {option args}
    protected method _getAxes {xydata}

    private variable _dispatcher "" ;# dispatcher for !events

    private variable _clist ""     ;# list of curve objects
    private variable _curve2color  ;# maps curve => plotting color
    private variable _curve2width  ;# maps curve => line width
    private variable _curve2dashes ;# maps curve => BLT -dashes list
    private variable _curve2raise  ;# maps curve => raise flag 0/1
    private variable _elem2curve   ;# maps graph element => curve
    private variable _label2axis   ;# maps axis label => axis ID
    private variable _limits       ;# axis limits:  x-min, x-max, etc.
    private variable _autoColorI 0 ;# index for next "-color auto"

    private variable _hilite       ;# info for element currently highlighted
    private variable _axis         ;# info for axis manipulations
    private variable _axisPopup    ;# info for axis being edited in popup
}
                                                                                
itk::usual XyResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"

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
    $itk_component(plot) pen configure activeLine \
        -symbol square -pixels 3 -linewidth 2 -color black

    #
    # Add bindings so you can mouse over points to see values:
    #
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
        -variable [itcl::scope _axisPopup(scale)] -value "linear"
    pack $inner.scales.linear -side left
    radiobutton $inner.scales.log -text "Logarithmic" \
        -variable [itcl::scope _axisPopup(scale)] -value "log"
    pack $inner.scales.log -side left
    grid $inner.scalel -row 5 -column 0 -sticky e
    grid $inner.scales -row 5 -column 1 -sticky ew -pady 4

    foreach axis {x y} {
        set _axisPopup(format-$axis) "%.3g"
    }
    _axis scale x linear
    _axis scale y linear

    # quick-and-dirty zoom functionality, for now...
    Blt_ZoomStack $itk_component(plot)
    $itk_component(plot) legend configure -hide yes

    eval itk_initialize $args

    set _hilite(elem) ""
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
        -color auto
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

    # if the color is "auto", then select a color from -autocolors
    if {$params(-color) == "auto" || $params(-color) == "autoreset"} {
        if {$params(-color) == "autoreset"} {
            set _autoColorI 0
        }
        set color [lindex $itk_option(-autocolors) $_autoColorI]
        if {"" == $color} { set color black }
        set params(-color) $color

        # set up for next auto color
        if {[incr _autoColorI] >= [llength $itk_option(-autocolors)]} {
            set _autoColorI 0
        }
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

        set bg [$itk_component(plot) cget -plotbackground]
        foreach {h s v} [Rappture::color::RGBtoHSV $bg] break
        if {$v > 0.5} {
            set params(-color) [Rappture::color::brightness_max \
                $params(-color) 0.8]
        } else {
            set params(-color) [Rappture::color::brightness_min \
                $params(-color) 0.2]
        }
    }

    set pos [lsearch -exact $curve $_clist]
    if {$pos < 0} {
        lappend _clist $curve
        set _curve2color($curve) $params(-color)
        set _curve2width($curve) $params(-width)
        set _curve2dashes($curve) $params(-linestyle)
        set _curve2raise($curve) $params(-raise)

        $_dispatcher event -idle !rebuild
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
        $_dispatcher event -idle !rebuild
    }

    # nothing left? then start over with auto colors
    if {[llength $_clist] == 0} {
        set _autoColorI 0
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
    set allx [$itk_component(plot) x2axis use]
    lappend allx x  ;# fix main x-axis too
    foreach axis $allx {
        _axis scale $axis linear
    }

    set ally [$itk_component(plot) y2axis use]
    lappend ally y  ;# fix main y-axis too
    foreach axis $ally {
        _axis scale $axis linear
    }

    catch {unset _limits}
    foreach xydata $args {
        # find the axes for this curve (e.g., {x y2})
        foreach {map(x) map(y)} [_getAxes $xydata] break

        foreach axis {x y} {
            # get defaults for both linear and log scales
            foreach type {lin log} {
                # store results -- ex: _limits(x2log-min)
                set id $map($axis)$type
                foreach {min max} [$xydata limits $axis$type] break
                if {"" != $min && "" != $max} {
                    if {![info exists _limits($id-min)]} {
                        set _limits($id-min) $min
                        set _limits($id-max) $max
                    } else {
                        if {$min < $_limits($id-min)} {
                            set _limits($id-min) $min
                        }
                        if {$max > $_limits($id-max)} {
                            set _limits($id-max) $max
                        }
                    }
                }
            }

            if {[$xydata hints ${axis}scale] == "log"} {
                _axis scale $map($axis) log
            }
        }
    }
    _resetLimits
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
    foreach axis [$g axis names] {
        $g axis configure $axis -hide yes
    }
    catch {unset _label2axis}

    #
    # Scan through all objects and create a list of all axes.
    # The first x-axis gets mapped to "x".  The second, to "x2".
    # Beyond that, we must create new axes "x3", "x4", etc.
    # We do the same for y.
    #
    set anum(x) 0
    set anum(y) 0
    foreach xydata [get] {
        foreach ax {x y} {
            set label [$xydata hints ${ax}label]
            if {"" != $label} {
                if {![info exists _label2axis($ax-$label)]} {
                    switch [incr anum($ax)] {
                        1 { set axis $ax }
                        2 { set axis ${ax}2 }
                        default {
                            set axis $ax$anum($ax)
                            catch {$g axis create $axis}
                        }
                    }
                    $g axis configure $axis -title $label -hide no
                    set _label2axis($ax-$label) $axis
                }
            }
        }
    }

    #
    # All of the extra axes get mapped to the x2/y2 (top/right)
    # position.
    #
    set all ""
    foreach ax {x y} {
        lappend all $ax

        set extra ""
        for {set i 2} {$i <= $anum($ax)} {incr i} {
            lappend extra ${ax}$i
        }
        eval lappend all $extra
        $g ${ax}2axis use $extra
        if {$ax == "y"} {
            $g configure -rightmargin [expr {($extra == "") ? 10 : 0}]
        }
    }

    foreach axis $all {
        set _axisPopup(format-$axis) "%.3g"

        $g axis bind $axis <Enter> \
            [itcl::code $this _axis hilite $axis on]
        $g axis bind $axis <Leave> \
            [itcl::code $this _axis hilite $axis off]
        $g axis bind $axis <ButtonPress> \
            [itcl::code $this _axis click $axis %x %y]
        $g axis bind $axis <B1-Motion> \
            [itcl::code $this _axis drag $axis %x %y]
        $g axis bind $axis <ButtonRelease> \
            [itcl::code $this _axis release $axis %x %y]
    }

    #
    # Plot all of the curves.
    #
    set count 0
    foreach xydata $_clist {
        set label [$xydata hints label]
        foreach {mapx mapy} [_getAxes $xydata] break

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

            $g element create $elem -x $xv -y $yv \
                -symbol $sym -pixels 6 -linewidth $lwidth -label $label \
                -color $color -dashes $dashes \
                -mapx $mapx -mapy $mapy
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _resetLimits
#
# Used internally to apply automatic limits to the axes for the
# current plot.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::_resetLimits {} {
    set g $itk_component(plot)

    #
    # HACK ALERT!
    # Use this code to fix up the y-axis limits for the BLT graph.
    # The auto-limits don't always work well.  We want them to be
    # set to a "nice" number slightly above or below the min/max
    # limits.
    #
    foreach axis [$g axis names] {
        if {[info exists _limits(${axis}lin-min)]} {
            set log [$g axis cget $axis -logscale]
            if {$log} {
                set min $_limits(${axis}log-min)
                set max $_limits(${axis}log-max)
                if {$min == $max} {
                    set logmin [expr {floor(log10(abs(0.9*$min)))}]
                    set logmax [expr {ceil(log10(abs(1.1*$max)))}]
                } else {
                    set logmin [expr {floor(log10(abs($min)))}]
                    set logmax [expr {ceil(log10(abs($max)))}]
                    if {[string match y* $axis]} {
                        # add a little padding
                        set delta [expr {$logmax-$logmin}]
                        set logmin [expr {$logmin-0.05*$delta}]
                        set logmax [expr {$logmax+0.05*$delta}]
                    }
                }
                if {$logmin < -300} {
                    set min 1e-300
                } elseif {$logmin > 300} {
                    set min 1e+300
                } else {
                    set min [expr {pow(10.0,$logmin)}]
                }

                if {$logmax < -300} {
                    set max 1e-300
                } elseif {$logmax > 300} {
                    set max 1e+300
                } else {
                    set max [expr {pow(10.0,$logmax)}]
                }
            } else {
                set min $_limits(${axis}lin-min)
                set max $_limits(${axis}lin-max)

                if {[string match y* $axis]} {
                    # add a little padding
                    set delta [expr {$max-$min}]
                    set min [expr {$min-0.05*$delta}]
                    set max [expr {$max+0.05*$delta}]
                }
            }
            if {$min != $max} {
                $g axis configure $axis -min $min -max $max
            } else {
                $g axis configure $axis -min "" -max ""
            }
        } else {
            $g axis configure $axis -min "" -max ""
        }
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
            _resetLimits
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
    set elem ""
    if {$state == "at"} {
        if {[$g element closest $x $y info -interpolate yes]} {
            set elem $info(name)
            foreach {mapx mapy} [_getAxes $_elem2curve($elem)] break

            # search again for an exact point -- this time don't interpolate
            set tip ""
            if {[$g element closest $x $y info -interpolate no]
                  && $info(name) == $elem} {
                set x [$g axis transform $mapx $info(x)]
                set y [$g axis transform $mapy $info(y)]

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
            }
            set state 1
        } else {
            set state 0
        }
    }

    if {$state} {
        #
        # Highlight ON:
        # - activate trace
        # - multiple axes? dim other axes
        # - pop up tooltip about data
        #
        if {$_hilite(elem) != "" && $_hilite(elem) != $elem} {
            $g element deactivate $_hilite(elem)
            $g crosshairs configure -hide yes
            Rappture::Tooltip::tooltip cancel
        }
        $g element activate $elem
        set _hilite(elem) $elem

        set dlist [$g element show]
        set i [lsearch -exact $dlist $elem]
        if {$i >= 0} {
            set dlist [lreplace $dlist $i $i]
            lappend dlist $elem
            $g element show $dlist
        }

        foreach {mapx mapy} [_getAxes $_elem2curve($elem)] break

        set allx [$g x2axis use]
        if {[llength $allx] > 0} {
            lappend allx x  ;# fix main x-axis too
            foreach axis $allx {
                if {$axis == $mapx} {
                    $g axis configure $axis -color $itk_option(-foreground) \
                        -titlecolor $itk_option(-foreground)
                } else {
                    $g axis configure $axis -color $itk_option(-dimcolor) \
                        -titlecolor $itk_option(-dimcolor)
                }
            }
        }
        set ally [$g y2axis use]
        if {[llength $ally] > 0} {
            lappend ally y  ;# fix main y-axis too
            foreach axis $ally {
                if {$axis == $mapy} {
                    $g axis configure $axis -color $itk_option(-foreground) \
                        -titlecolor $itk_option(-foreground)
                } else {
                    $g axis configure $axis -color $itk_option(-dimcolor) \
                        -titlecolor $itk_option(-dimcolor)
                }
            }
        }

        if {"" != $tip} {
            $g crosshairs configure -hide no -position @$x,$y

            if {$x > 0.5*[winfo width $g]} {
                if {$x < 4} {
                    set x "-0"
                } else {
                    set x "-[expr {$x-4}]"  ;# move tooltip to the left
                }
            } else {
                if {$x < -4} {
                    set x "+0"
                } else {
                    set x "+[expr {$x+4}]"  ;# move tooltip to the right
                }
            }
            if {$y > 0.5*[winfo height $g]} {
                if {$y < 4} {
                    set y "-0"
                } else {
                    set y "-[expr {$y-4}]"  ;# move tooltip to the top
                }
            } else {
                if {$y < -4} {
                    set y "+0"
                } else {
                    set y "+[expr {$y+4}]"  ;# move tooltip to the bottom
                }
            }
            Rappture::Tooltip::text $g $tip
            Rappture::Tooltip::tooltip show $g $x,$y
        }
    } else {
        #
        # Highlight OFF:
        # - deactivate (color back to normal)
        # - put all axes back to normal color
        # - take down tooltip
        #
        if {"" != $_hilite(elem)} {
            $g element deactivate $_hilite(elem)

            set allx [$g x2axis use]
            if {[llength $allx] > 0} {
                lappend allx x  ;# fix main x-axis too
                foreach axis $allx {
                    $g axis configure $axis -color $itk_option(-foreground) \
                        -titlecolor $itk_option(-foreground)
                }
            }

            set ally [$g y2axis use]
            if {[llength $ally] > 0} {
                lappend ally y  ;# fix main y-axis too
                foreach axis $ally {
                    $g axis configure $axis -color $itk_option(-foreground) \
                        -titlecolor $itk_option(-foreground)
                }
            }
        }

        $g crosshairs configure -hide yes
        Rappture::Tooltip::tooltip cancel

        # there is no currently highlighted element
        set _hilite(elem) ""
    }
}

# ----------------------------------------------------------------------
# USAGE: _axis hilite <axis> <state>
#
# USAGE: _axis click <axis> <x> <y>
# USAGE: _axis drag <axis> <x> <y>
# USAGE: _axis release <axis> <x> <y>
#
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
        click {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"_axis click axis x y\""
            }
            set axis [lindex $args 0]
            set x [lindex $args 1]
            set y [lindex $args 2]
            set g $itk_component(plot)

            set _axis(moved) 0
            set _axis(click-x) $x
            set _axis(click-y) $y
            foreach {min max} [$g axis limits $axis] break
            set _axis(min0) $min
            set _axis(max0) $max
        }
        drag {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"_axis drag axis x y\""
            }
            set axis [lindex $args 0]
            set x [lindex $args 1]
            set y [lindex $args 2]
            set g $itk_component(plot)

            if {[info exists _axis(click-x)] && [info exists _axis(click-y)]} {
                foreach {x0 y0 pw ph} [$g extents plotarea] break
                switch -glob $axis {
                  x* {
                    set pix $x
                    set pix0 $_axis(click-x)
                    set pixmin $x0
                    set pixmax [expr {$x0+$pw}]
                  }
                  y* {
                    set pix $y
                    set pix0 $_axis(click-y)
                    set pixmin [expr {$y0+$ph}]
                    set pixmax $y0
                  }
                }
                set log [$g axis cget $axis -logscale]
                set min $_axis(min0)
                set max $_axis(max0)
                set dpix [expr {abs($pix-$pix0)}]
                set v0 [$g axis invtransform $axis $pixmin]
                set v1 [$g axis invtransform $axis [expr {$pixmin+$dpix}]]
                if {$log} {
                    set v0 [expr {log10($v0)}]
                    set v1 [expr {log10($v1)}]
                    set min [expr {log10($min)}]
                    set max [expr {log10($max)}]
                }

                if {$pix > $pix0} {
                    set delta [expr {$v1-$v0}]
                } else {
                    set delta [expr {$v0-$v1}]
                }
                set min [expr {$min-$delta}]
                set max [expr {$max-$delta}]
                if {$log} {
                    set min [expr {pow(10.0,$min)}]
                    set max [expr {pow(10.0,$max)}]
                }
                $g axis configure $axis -min $min -max $max

                # move axis, don't edit on release
                set _axis(move) 1
            }
        }
        release {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"_axis release axis x y\""
            }
            set axis [lindex $args 0]
            set x [lindex $args 1]
            set y [lindex $args 2]

            if {![info exists _axis(moved)] || !$_axis(moved)} {
                # small movement? then treat as click -- pop up axis editor
                set dx [expr {abs($x-$_axis(click-x))}]
                set dy [expr {abs($y-$_axis(click-y))}]
                if {$dx < 2 && $dy < 2} {
                    _axis edit $axis
                }
            } else {
                # one last movement
                _axis drag $axis $x $y
            }
            catch {unset _axis}
        }
        edit {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"_axis edit axis\""
            }
            set axis [lindex $args 0]
            set _axisPopup(current) $axis

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
            set i [lsearch -exact $fmts $_axisPopup(format-$axis)]
            if {$i < 0} { set i 0 }  ;# use Auto choice
            $inner.format value [$inner.format choices get -label $i]

            bind $inner.format <<Value>> \
                [itcl::code $this _axis changed $axis format]

            # fix scale control...
            if {[$itk_component(plot) axis cget $axis -logscale]} {
                set _axisPopup(scale) "log"
                $inner.format configure -state disabled
            } else {
                set _axisPopup(scale) "linear"
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
            switch -glob -- $axis {
                x {
                    set x [expr {round($x + $x0+0.5*$pw)}]
                    set y [expr {round($y + $y0+$ph + 0.5*($h-$y0-$ph))}]
                    set dir "above"
                }
                x* {
                    set x [expr {round($x + $x0+0.5*$pw)}]
                    set dir "below"
                    set allx [$itk_component(plot) x2axis use]
                    set max [llength $allx]
                    set i [lsearch -exact $allx $axis]
                    set y [expr {round($y + ($i+0.5)*$y0/double($max))}]
                }
                y {
                    set x [expr {round($x + 0.5*$x0)}]
                    set y [expr {round($y + $y0+0.5*$ph)}]
                    set dir "right"
                }
                y* {
                    set y [expr {round($y + $y0+0.5*$ph)}]
                    set dir "left"
                    set ally [$itk_component(plot) y2axis use]
                    set max [llength $ally]
                    set i [lsearch -exact $ally $axis]
                    set y [expr {round($y + ($i+0.5)*$y0/double($max))}]
                    set x [expr {round($x+$x0+$pw + ($i+0.5)*($w-$x0-$pw)/double($max))}]
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
                    set _axisPopup(format-$axis) $fmt

                    # force a refresh
                    $itk_component(plot) axis configure $axis -min \
                        [$itk_component(plot) axis cget $axis -min]
                }
                scale {
                    _axis scale $axis $_axisPopup(scale)

                    if {$_axisPopup(scale) == "log"} {
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
                set fmt $_axisPopup(format-$axis)
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
# USAGE: _getAxes <curveObj>
#
# Used internally to figure out the axes used to plot the given
# <curveObj>.  Returns a list of the form {x y}, where x is the
# x-axis name (x, x2, x3, etc.), and y is the y-axis name.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::_getAxes {xydata} {
    # rebuild if needed, so we know about the axes
    if {[$_dispatcher ispending !rebuild]} {
        $_dispatcher cancel !rebuild
        $_dispatcher event -now !rebuild
    }

    # what is the x axis?  x? x2? x3? ...
    set xlabel [$xydata hints xlabel]
    if {[info exists _label2axis(x-$xlabel)]} {
        set mapx $_label2axis(x-$xlabel)
    } else {
        set mapx "x"
    }

    # what is the y axis?  y? y2? y3? ...
    set ylabel [$xydata hints ylabel]
    if {[info exists _label2axis(y-$ylabel)]} {
        set mapy $_label2axis(y-$ylabel)
    } else {
        set mapy "y"
    }

    return [list $mapx $mapy]
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

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -autocolors
# ----------------------------------------------------------------------
itcl::configbody Rappture::XyResult::autocolors {
    foreach c $itk_option(-autocolors) {
        if {[catch {winfo rgb $itk_component(hull) $c}]} {
            error "bad color \"$c\""
        }
    }
    if {$_autoColorI >= [llength $itk_option(-autocolors)]} {
        set _autoColorI 0
    }
}
