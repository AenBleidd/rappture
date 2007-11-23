
# ----------------------------------------------------------------------
#  COMPONENT: historesult - X/Y plot in a ResultSet
#
#  This widget is an X/Y plot, meant to view histograms produced
#  as output from the run of a Rappture tool.  Use the "add" and
#  "delete" methods to control the histograms showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

package require Itk
package require BLT

option add *Historesult.width 3i widgetDefault
option add *Historesult.height 3i widgetDefault
option add *Historesult.gridColor #d9d9d9 widgetDefault
option add *Historesult.activeColor blue widgetDefault
option add *Historesult.dimColor gray widgetDefault
option add *Historesult.controlBackground gray widgetDefault
option add *Historesult.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

option add *Historesult.autoColors {
    #0000ff #ff0000 #00cc00
    #cc00cc #ff9900 #cccc00
    #000080 #800000 #006600
    #660066 #996600 #666600
} widgetDefault

option add *Historesult*Balloon*Entry.background white widgetDefault

itcl::class Rappture::Historesult {
    inherit itk::Widget

    itk_option define -gridcolor gridColor GridColor ""
    itk_option define -activecolor activeColor ActiveColor ""
    itk_option define -dimcolor dimColor DimColor ""
    itk_option define -autocolors autoColors AutoColors ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {bar {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method parameters {title args} { # do nothing }
    public method download {option args}

    protected method _rebuild {}
    protected method _resetLimits {}
    protected method _zoom {option args}
    protected method _hilite {state x y}
    protected method _axis {option args}
    protected method _getAxes {xydata}

    private variable _dispatcher "" ;# dispatcher for !events

    private variable _hlist ""     ;# list of histogram objects
    private variable _histo2color  ;# maps histogram => plotting color
    private variable _histo2width  ;# maps histogram => line width
    private variable _histo2dashes ;# maps histogram => BLT -dashes list
    private variable _histo2raise  ;# maps histogram => raise flag 0/1
    private variable _histo2desc   ;# maps histogram => description of data
    private variable _elem2histo   ;# maps graph element => histogram
    private variable _label2axis   ;# maps axis label => axis ID
    private variable _limits       ;# axis limits:  x-min, x-max, etc.
    private variable _autoColorI 0 ;# index for next "-color auto"

    private variable _hilite       ;# info for element currently highlighted
    private variable _axis         ;# info for axis manipulations
    private variable _axisPopup    ;# info for axis being edited in popup
    common _downloadPopup          ;# download options from popup
}
                                                                                
itk::usual Historesult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Historesult::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"

    array set _downloadPopup {
        format csv
    }

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
            -bitmap [Rappture::icon reset] \
            -command [itcl::code $this _zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"


    itk_component add plot {
        blt::barchart $itk_interior.plot \
            -highlightthickness 0 -plotpadx 0 -plotpady 0 \
            -rightmargin 10
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(plot) -expand yes -fill both
    $itk_component(plot) pen configure activeBar \
        -linewidth 2 -color black

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
    Rappture::Balloon $itk_component(hull).axes -title "Axis Options"
    set inner [$itk_component(hull).axes component inner]

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
        "%.0f"  "X"          \
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
itcl::body Rappture::Historesult::destructor {} {
}

# ----------------------------------------------------------------------
# USAGE: add <histogram> ?<settings>?
#
# Clients use this to add a histogram bar to the plot.  The optional <settings>
# are used to configure the plot.  Allowed settings are -color,
# -brightness, -width, -linestyle and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::Historesult::add {histogram {settings ""}} {
    array set params {
        -color auto
        -brightness 0
        -width 1
        -type "histogram"
        -raise 0
        -linestyle solid
        -description ""
        -param ""
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

    set pos [lsearch -exact $histogram $_hlist]
    if {$pos < 0} {
        lappend _hlist $histogram
        set _histo2color($histogram) $params(-color)
        set _histo2width($histogram) $params(-width)
        set _histo2dashes($histogram) $params(-linestyle)
        set _histo2raise($histogram) $params(-raise)
        set _histo2desc($histogram) $params(-description)

        $_dispatcher event -idle !rebuild
    }
}


# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::Historesult::get {} {
    # put the dataobj list in order according to -raise options
    set clist $_hlist
    foreach obj $clist {
        if {[info exists _histo2raise($obj)] && $_histo2raise($obj)} {
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
# USAGE: delete ?<histo1> <histo2> ...?
#
# Clients use this to delete a histogram from the plot.  If no histograms
# are specified, then all histograms are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::Historesult::delete {args} {
    if {[llength $args] == 0} {
        set args $_hlist
    }

    # delete all specified histograms
    set changed 0
    foreach h $args {
        set pos [lsearch -exact $_hlist $h]
        if {$pos >= 0} {
            set _hlist [lreplace $_hlist $pos $pos]
            catch {unset _histo2color($h)}
            catch {unset _histo2width($h)}
            catch {unset _histo2dashes($h)}
            catch {unset _histo2raise($h)}
            foreach elem [array names _elem2histo] {
                if {$_elem2histo($elem) == $h} {
                    unset _elem2histo($elem)
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
    if {[llength $_hlist] == 0} {
        set _autoColorI 0
    }
}

# ----------------------------------------------------------------------
# USAGE: scale ?<histo1> <histo2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <histogram> objects.  This
# accounts for all histograms--even those not showing on the screen.
# Because of this, the limits are appropriate for all histograms as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::Historesult::scale {args} {
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
        # find the axes for this histogram (e.g., {x y2})
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
# USAGE: download coming
# USAGE: download controls <downloadCommand>
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::Historesult::download {option args} {
    switch $option {
        coming {
            # nothing to do
        }
        controls {
            set popup .historesultdownload
            if {![winfo exists .historesultdownload]} {
                # if we haven't created the popup yet, do it now
                Rappture::Balloon $popup -title "Download as..."
                set inner [$popup component inner]
                label $inner.summary -text "" -anchor w
                pack $inner.summary -side top
                radiobutton $inner.csv -text "Data as Comma-Separated Values" \
                    -variable Rappture::Historesult::_downloadPopup(format) \
                    -value csv
                pack $inner.csv -anchor w
                radiobutton $inner.pdf -text "Image as PDF/PostScript" \
                    -variable Rappture::Historesult::_downloadPopup(format) \
                    -value pdf
                pack $inner.pdf -anchor w
                button $inner.go -text "Download Now" \
                    -command [lindex $args 0]
                pack $inner.go -pady 4
            } else {
                set inner [$popup component inner]
            }
            set num [llength [get]]
            set num [expr {($num == 1) ? "1 result" : "$num results"}]
            $inner.summary configure -text "Download $num in the following format:"
            update idletasks ;# fix initial sizes
            return $popup
        }
        now {
            set popup .historesultdownload
            if {[winfo exists .historesultdownload]} {
                $popup deactivate
            }
            switch -- $_downloadPopup(format) {
              csv {
                # reverse the objects so the selected data appears on top
                set dlist ""
                foreach dataobj [get] {
                    set dlist [linsert $dlist 0 $dataobj]
                }

                # generate the comma-separated value data for these objects
                set csvdata ""
                foreach dataobj $dlist {
                    append csvdata "[string repeat - 60]\n"
                    append csvdata " [$dataobj hints label]\n"
                    if {[info exists _histo2desc($dataobj)]
                          && [llength [split $_histo2desc($dataobj) \n]] > 1} {
                        set indent "for:"
                        foreach line [split $_histo2desc($dataobj) \n] {
                            append csvdata " $indent $line\n"
                            set indent "    "
                        }
                    }
                    append csvdata "[string repeat - 60]\n"

                    append csvdata "[$dataobj hints xlabel], [$dataobj hints ylabel]\n"
                    set first 1
                    foreach comp [$dataobj components] {
                        if {!$first} {
                            # blank line between components
                            append csvdata "\n"
                        }
                        set xv [$dataobj locations]
                        set hv [$dataobj heights]
                        set wv [$dataobj widths]
			if { $wv == "" } {
			    foreach x [$xv range 0 end] h [$hv range 0 end] {
				append csvdata \
				    [format "%20.15g, %20.15g\n" $x $h]
			    }
			} else {
			    foreach x [$xv range 0 end] \
				    h [$hv range 0 end] \
				    w [$wv range 0 end] {
				append csvdata [format \
				    "%20.15g, %20.15g, %20.15g\n" $x $h $w]
			    }
			}
			set first 0
			append csvdata "\n"
		    }
                }
                return [list .txt $csvdata]
              }
              pdf {
                set psdata [$itk_component(plot) postscript output -decorations no -maxpect 1]

                set cmds {
                    set fout "histogram[pid].pdf"
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
            }
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::Historesult::_rebuild {} {
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

                    # if this axis has a description, add it as a tooltip
                    set desc [string trim [$xydata hints ${ax}desc]]
                    Rappture::Tooltip::text $g-$axis $desc
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
        $g axis bind $axis <KeyPress> \
            [list ::Rappture::Tooltip::tooltip cancel]
    }

    #
    # Plot all of the histograms.
    #
    set count 0
    foreach xydata $_hlist {
        set label [$xydata hints label]
        foreach {mapx mapy} [_getAxes $xydata] break

        foreach comp [$xydata components] {
            set xv [$xydata locations]
            set yv [$xydata heights]
            set zv [$xydata widths]
		
            if {[info exists _histo2color($xydata)]} {
                set color $_histo2color($xydata)
            } else {
                set color [$xydata hints color]
                if {"" == $color} {
                    set color black
                }
            }

            if {[info exists _histo2width($xydata)]} {
                set lwidth $_histo2width($xydata)
            } else {
                set lwidth 2
            }

            if {[info exists _histo2dashes($xydata)]} {
                set dashes $_histo2dashes($xydata)
            } else {
                set dashes ""
            }
            set elem "elem[incr count]"
            set _elem2histo($elem) $xydata
            $g element line $elem -x $xv -y $yv \
		-symbol $sym -pixels $pixels -linewidth $lwidth -label $label \
                -color $color -dashes $dashes -smooth natural \
                -mapx $mapx -mapy $mapy

	    # Compute default bar width for histogram elements.
	    set defwidth { [expr ($zv(max) - $zv(min)) / ([$xv length] - 1)] }
	    foreach x [$xv range 0 end] y [$yv range 0 end] z [$zv range 0 end] {
		if { $z == "" } {
		    set z $defwidth
		}
		set elem "elem[incr count]"
		set _elem2histo($elem) $xydata
		$g element create $elem -x $x -y $y -barwidth $z \
			-label $label -foreground $color \
			-mapx $mapx -mapy $mapy
	    }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _resetLimits
#
# Used internally to apply automatic limits to the axes for the
# current plot.
# ----------------------------------------------------------------------
itcl::body Rappture::Historesult::_resetLimits {} {
    set g $itk_component(plot)

    #
    # HACK ALERT!
    # Use this code to fix up the y-axis limits for the BLT barchart.
    # The auto-limits don't always work well.  We want them to be
    # set to a "nice" number slightly above or below the min/max
    # limits.
    #
    foreach axis [$g axis names] {
        if {[info exists _limits(${axis}lin-min)]} {
            set log [$g axis cget $axis -logscale]
            if {$log} {
                set min $_limits(${axis}log-min)
                if {$min == 0} { set min 1 }
                set max $_limits(${axis}log-max)
                if {$max == 0} { set max 1 }

                if {$min == $max} {
                    set logmin [expr {floor(log10(abs(0.9*$min)))}]
                    set logmax [expr {ceil(log10(abs(1.1*$max)))}]
                } else {
                    set logmin [expr {floor(log10(abs($min)))}]
                    set logmax [expr {ceil(log10(abs($max)))}]
                    if {[string match y* $axis]} {
                        # add a little padding
                        set delta [expr {$logmax-$logmin}]
                        if {$delta == 0} { set delta 1 }
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
            if {$min < $max} {
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
itcl::body Rappture::Historesult::_zoom {option args} {
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
itcl::body Rappture::Historesult::_hilite {state x y} {
    set g $itk_component(plot)
    set elem ""
    if {$state == "at"} {
        if {[$g element closest $x $y info -interpolate yes]} {
            # for dealing with xy line plots
            set elem $info(name)
            foreach {mapx mapy} [_getAxes $_elem2histo($elem)] break

            # search again for an exact point -- this time don't interpolate
            set tip ""
            if {[$g element closest $x $y info -interpolate no]
                  && $info(name) == $elem} {
                set x [$g axis transform $mapx $info(x)]
                set y [$g axis transform $mapy $info(y)]

                if {[info exists _elem2histo($elem)]} {
                    set histogram $_elem2histo($elem)
                    set tip [$histogram hints tooltip]
                    if {[info exists info(y)]} {
                        set val [_axis format y dummy $info(y)]
                        set units [$histogram hints yunits]
                        append tip "\n$val$units"

                        if {[info exists info(x)]} {
                            set val [_axis format x dummy $info(x)]
                            set units [$histogram hints xunits]
                            append tip " @ $val$units"
                        }
                    }
                    set tip [string trim $tip]
                }
            }
            set state 1
        } elseif {[$g element closest $x $y info -interpolate no]} {
            # for dealing with xy scatter plot
            set elem $info(name)
            foreach {mapx mapy} [_getAxes $_elem2histo($elem)] break

            # search again for an exact point -- this time don't interpolate
            set tip ""
            if {$info(name) == $elem} {
                set x [$g axis transform $mapx $info(x)]
                set y [$g axis transform $mapy $info(y)]

                if {[info exists _elem2histo($elem)]} {
                    set histogram $_elem2histo($elem)
                    set tip [$histogram hints tooltip]
                    if {[info exists info(y)]} {
                        set val [_axis format y dummy $info(y)]
                        set units [$histogram hints yunits]
                        append tip "\n$val$units"

                        if {[info exists info(x)]} {
                            set val [_axis format x dummy $info(x)]
                            set units [$histogram hints xunits]
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

        foreach {mapx mapy} [_getAxes $_elem2histo($elem)] break

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
                    set tipx "-0"
                } else {
                    set tipx "-[expr {$x-4}]"  ;# move tooltip to the left
                }
            } else {
                if {$x < -4} {
                    set tipx "+0"
                } else {
                    set tipx "+[expr {$x+4}]"  ;# move tooltip to the right
                }
            }
            if {$y > 0.5*[winfo height $g]} {
                if {$y < 4} {
                    set tipy "-0"
                } else {
                    set tipy "-[expr {$y-4}]"  ;# move tooltip to the top
                }
            } else {
                if {$y < -4} {
                    set tipy "+0"
                } else {
                    set tipy "+[expr {$y+4}]"  ;# move tooltip to the bottom
                }
            }
            Rappture::Tooltip::text $g $tip
            Rappture::Tooltip::tooltip show $g $tipx,$tipy
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

        # only cancel in plotting area or we'll mess up axes
        if {[$g inside $x $y]} {
            Rappture::Tooltip::tooltip cancel
        }

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
itcl::body Rappture::Historesult::_axis {option args} {
    set inner [$itk_component(hull).axes component inner]

    switch -- $option {
        hilite {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_axis hilite axis state\""
            }
            set g $itk_component(plot)
            set axis [lindex $args 0]
            set state [lindex $args 1]

            if {$state} {
                $g axis configure $axis \
                    -color $itk_option(-activecolor) \
                    -titlecolor $itk_option(-activecolor)

                set x [expr {[winfo pointerx $g]+4}]
                set y [expr {[winfo pointery $g]+4}]
                Rappture::Tooltip::tooltip pending $g-$axis @$x,$y
            } else {
                $g axis configure $axis \
                    -color $itk_option(-foreground) \
                    -titlecolor $itk_option(-foreground)
                Rappture::Tooltip::tooltip cancel
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
            Rappture::Tooltip::tooltip cancel
        }
        drag {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"_axis drag axis x y\""
            }
            if {![info exists _axis(moved)]} {
                return  ;# must have skipped click event -- ignore
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
            if {![info exists _axis(moved)]} {
                return  ;# must have skipped click event -- ignore
            }
            set axis [lindex $args 0]
            set x [lindex $args 1]
            set y [lindex $args 2]

            if {!$_axis(moved)} {
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
# USAGE: _getAxes <histoObj>
#
# Used internally to figure out the axes used to plot the given
# <histoObj>.  Returns a list of the form {x y}, where x is the
# x-axis name (x, x2, x3, etc.), and y is the y-axis name.
# ----------------------------------------------------------------------
itcl::body Rappture::Historesult::_getAxes {xydata} {
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
itcl::configbody Rappture::Historesult::gridcolor {
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
itcl::configbody Rappture::Historesult::autocolors {
    foreach c $itk_option(-autocolors) {
        if {[catch {winfo rgb $itk_component(hull) $c}]} {
            error "bad color \"$c\""
        }
    }
    if {$_autoColorI >= [llength $itk_option(-autocolors)]} {
        set _autoColorI 0
    }
}
