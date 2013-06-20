# -*- mode: tcl; indent-tabs-mode: nil -*- 
# ----------------------------------------------------------------------
#  COMPONENT: xyresult - X/Y plot in a ResultSet
#
#  This widget is an X/Y plot, meant to view line graphs produced
#  as output from the run of a Rappture tool.  Use the "add" and
#  "delete" methods to control the data objects showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

set autocolors {
    #0000cd
    #cd0000
    #00cd00
    #3a5fcd
    #cdcd00
    #cd1076
    #009acd
    #00c5cd
    #a2b5cd
    #7ac5cd
    #66cdaa
    #a2cd5a
    #cd9b9b
    #cdba96
    #cd3333
    #cd6600
    #cd8c95
    #cd00cd
    #9a32cd
    #6ca6cd
    #9ac0cd
    #9bcd9b
    #00cd66
    #cdc673
    #cdad00
    #cd5555
    #cd853f
    #cd7054
    #cd5b45
    #cd6889
    #cd69c9
    #551a8b
}

option add *XyResult.autoColors $autocolors widgetDefault
option add *XyResult*Balloon*Entry.background white widgetDefault

itcl::class Rappture::XyResult {
    inherit itk::Widget

    itk_option define -gridcolor gridColor GridColor ""
    itk_option define -activecolor activeColor ActiveColor ""
    itk_option define -dimcolor dimColor DimColor ""
    itk_option define -autocolors autoColors AutoColors ""

    constructor {args} { 
        # defined below 
    }
    destructor { 
        # defined below 
    }
    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method parameters {title args} { 
        # do nothing 
    }
    public method download {option args}

    protected method Rebuild {}
    protected method ResetLimits {}
    protected method Zoom {option args}
    protected method Hilite {state x y}
    protected method Axis {option args}
    protected method GetAxes {dataobj}
    protected method GetLineMarkerOptions { style } 
    protected method GetTextMarkerOptions { style } 
    protected method EnterMarker { g name x y text }
    protected method LeaveMarker { g name }

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _dlist ""     ;# list of dataobj objects
    private variable _dataobj2color  ;# maps dataobj => plotting color
    private variable _dataobj2width  ;# maps dataobj => line width
    private variable _dataobj2dashes ;# maps dataobj => BLT -dashes list
    private variable _dataobj2raise  ;# maps dataobj => raise flag 0/1
    private variable _dataobj2desc   ;# maps dataobj => description of data
    private variable _dataobj2type   ;# maps dataobj => type of graph element
    private variable _elem2dataobj   ;# maps graph element => dataobj
    private variable _label2axis   ;# maps axis label => axis ID
    private variable _limits       ;# axis limits:  x-min, x-max, etc.
    private variable _autoColorI 0 ;# index for next "-color auto"
    private variable _hilite       ;# info for element currently highlighted
    private variable _axis         ;# info for axis manipulations
    private variable _axisPopup    ;# info for axis being edited in popup
    common _downloadPopup          ;# download options from popup
    private variable _markers
    private variable _nextElement 0
}
                                                                                
itk::usual XyResult {
    keep -background -foreground -cursor -font
}

itk::usual Panedwindow {
    keep -background -cursor
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    array set _downloadPopup {
        format csv
    }

    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add main {
        Rappture::SidebarFrame $itk_interior.main
    }
    pack $itk_component(main) -expand yes -fill both
    set f [$itk_component(main) component controls]

    itk_component add reset {
        button $f.reset -borderwidth 1 -padx 1 -pady 1 \
            -highlightthickness 0 \
            -image [Rappture::icon reset-view] \
            -command [itcl::code $this Zoom reset]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    pack $itk_component(reset) -padx 4 -pady 2 -anchor e
    Rappture::Tooltip::for $itk_component(reset) \
        "Reset the view to the default zoom level"

    set f [$itk_component(main) component frame]
    itk_component add plot {
        blt::graph $f.plot \
            -highlightthickness 0 -plotpadx 0 -plotpady 4 \
            -rightmargin 10
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(plot) -expand yes -fill both

    $itk_component(plot) pen configure activeLine \
        -symbol square -pixels 3 -linewidth 2 \
        -outline black -fill red -color black

    # Add bindings so you can mouse over points to see values:
    #
    $itk_component(plot) element bind all <Enter> \
        [itcl::code $this Hilite at %x %y]
    $itk_component(plot) element bind all <Motion> \
        [itcl::code $this Hilite at %x %y]
    $itk_component(plot) element bind all <Leave> \
        [itcl::code $this Hilite off %x %y]

    # Add support for editing axes:
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
        "%.6g"  "Auto"         \
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
        set _axisPopup(format-$axis) "%.6g"
    }
    Axis scale x linear
    Axis scale y linear

    $itk_component(plot) legend configure -hide yes

    #
    # Add legend for editing hidden/elements:
    #
    set inner [$itk_component(main) insert end \
        -title "Legend" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    itk_component add legend {
        Rappture::XyLegend $inner.legend $itk_component(plot)
    }
    pack $itk_component(legend) -expand yes -fill both
    after idle [subst {
        update idletasks
        $itk_component(legend) reset 
    }]

    # quick-and-dirty zoom functionality, for now...
    Blt_ZoomStack $itk_component(plot)
    eval itk_initialize $args

    set _hilite(elem) ""
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::destructor {} {
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a dataobj to the plot.  The optional <settings>
# are used to configure the plot.  Allowed settings are -color,
# -brightness, -width, -linestyle and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::add {dataobj {settings ""}} {
    array set params {
        -color auto
        -brightness 0
        -width 1
        -type "line"
        -raise 0
        -linestyle solid
        -description ""
        -param ""
    }
    # Override the defaults with first the <style> specified and then the
    # settings list passed into this routoue.
    array set params [$dataobj hints style]
    foreach {opt val} $settings {
        if {![info exists params($opt)]} {
            error "bad setting \"$opt\": should be [join [lsort [array names params]] {, }]"
        }
        set params($opt) $val
    }

    # if type is set to "scatter", then override the width
    if {"scatter" == $params(-type)} {
        set params(-width) 0
    }
    # if the color is "auto", then select a color from -autocolors
    if { $params(-color) == "auto" || $params(-color) == "autoreset" } {
        if {$params(-color) == "autoreset"} {
            set _autoColorI 0
        }
	set color [lindex $itk_option(-autocolors) $_autoColorI]
        if { "" == $color} { 
            set color black 
        }
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

    set pos [lsearch -exact $dataobj $_dlist]
    if {$pos < 0} {
        lappend _dlist $dataobj
        set _dataobj2color($dataobj) $params(-color)
        set _dataobj2width($dataobj) $params(-width)
        set _dataobj2dashes($dataobj) $params(-linestyle)
        set _dataobj2raise($dataobj) $params(-raise)
        set _dataobj2desc($dataobj) $params(-description)
        set _dataobj2type($dataobj) $params(-type)

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
    set bottom {}
    set top {}
    foreach obj $_dlist {
        if {[info exists _dataobj2raise($obj)] && $_dataobj2raise($obj)} {
            lappend top $obj
        } else {
            lappend bottom $obj 
        }
    }
    set _dlist [concat $bottom $top]
    return $_dlist
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            array unset _dataobj2color  $dataobj
            array unset _dataobj2width  $dataobj
            array unset _dataobj2dashes $dataobj
            array unset _dataobj2raise  $dataobj
            foreach elem [array names _elem2dataobj] {
                if {$_elem2dataobj($elem) == $dataobj} {
                    array unset _elem2dataobj $elem
                }
            }
            set changed 1
        }
    }

    # If anything changed, then rebuild the plot
    if {$changed} {
        $_dispatcher event -idle !rebuild
    }

    # Nothing left? then start over with auto colors
    if {[llength $_dlist] == 0} {
        set _autoColorI 0
    }
}

# ----------------------------------------------------------------------
# USAGE: scale ?<dataobj1> <dataobj2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <dataobj> objects.  This
# accounts for all dataobjs--even those not showing on the screen.
# Because of this, the limits are appropriate for all dataobjs as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::scale {args} {
    set allx [$itk_component(plot) x2axis use]
    lappend allx x  ;# fix main x-axis too
    foreach axis $allx {
        Axis scale $axis linear
    }

    set ally [$itk_component(plot) y2axis use]
    lappend ally y  ;# fix main y-axis too
    foreach axis $ally {
        Axis scale $axis linear
    }

    catch {unset _limits}
    foreach dataobj $args {
        # Find the axes for this dataobj (e.g., {x y2})
        foreach {map(x) map(y)} [GetAxes $dataobj] break
        foreach axis {x y} {
            # Get defaults for both linear and log scales
            foreach type {lin log} {
                # store results -- ex: _limits(x2log-min)
                set id $map($axis)$type
                foreach {min max} [$dataobj limits $axis$type] break
                set amin [$dataobj hints ${axis}min]
                set amax [$dataobj hints ${axis}max]
                if { $amin != "" } {
                    set min $amin
                }
                if { $amax != "" } {
                    set max $amax
                }
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
            if {[$dataobj hints ${axis}scale] == "log"} {
                Axis scale $map($axis) log
            }
        }
    }
    ResetLimits
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
itcl::body Rappture::XyResult::download {option args} {
    switch $option {
        coming {
            # nothing to do
        }
        controls {
            set popup .xyresultdownload
            if {![winfo exists .xyresultdownload]} {
                # if we haven't created the popup yet, do it now
                Rappture::Balloon $popup \
                    -title "[Rappture::filexfer::label downloadWord] as..."
                set inner [$popup component inner]
                label $inner.summary -text "" -anchor w
                pack $inner.summary -side top
                radiobutton $inner.csv -text "Data as Comma-Separated Values" \
                    -variable Rappture::XyResult::_downloadPopup(format) \
                    -value csv
                pack $inner.csv -anchor w
                radiobutton $inner.image -text "Image (PS/PDF/PNG/JPEG)" \
                    -variable Rappture::XyResult::_downloadPopup(format) \
                    -value image
                pack $inner.image -anchor w
                button $inner.go -text [Rappture::filexfer::label download] \
                    -command [lindex $args 0]
                pack $inner.go -side bottom -pady 4
            } else {
                set inner [$popup component inner]
            }
            set num [llength [get]]
            set num [expr {($num == 1) ? "1 result" : "$num results"}]
            $inner.summary configure -text "[Rappture::filexfer::label downloadWord] $num in the following format:"
            update idletasks ;# fix initial sizes
            return $popup
        }
        now {
            set popup .xyresultdownload
            if {[winfo exists .xyresultdownload]} {
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
                        if {[info exists _dataobj2desc($dataobj)]
                            && [llength [split $_dataobj2desc($dataobj) \n]] > 1} {
                            set indent "for:"
                            foreach line [split $_dataobj2desc($dataobj) \n] {
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
                            set xv [$dataobj mesh $comp]
                            set yv [$dataobj values $comp]
                            foreach x [$xv range 0 end] y [$yv range 0 end] {
                                append csvdata [format "%20.15g, %20.15g\n" $x $y]
                            }
                            set first 0
                        }
                        append csvdata "\n"
                    }
                    return [list .txt $csvdata]
                }
                image {
                    set popup .xyprintdownload
                    if { ![winfo exists $popup] } {
                        # Create a popup for the print dialog
                        Rappture::Balloon $popup -title "Save as image..."
                        set inner [$popup component inner]
                        # Create the print dialog widget and add it to the
                        # balloon popup.
                        Rappture::XyPrint $inner.print 
                        $popup configure \
                            -deactivatecommand [list $inner.print reset] 
                        blt::table $inner 0,0 $inner.print -fill both
                    }
                    update
                    # Activate the popup and call for the output.
                    foreach { widget toolName plotName } $args break
                    $popup activate $widget left
                    set inner [$popup component inner]
                    set output [$inner.print print $itk_component(plot) \
                                    $toolName $plotName]
                    $popup deactivate 
                    return $output
                }
            }
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::Rebuild {} {
    set g $itk_component(plot)

    # First clear out the widget
    eval $g element delete [$g element names]
    eval $g marker delete [$g marker names]
    foreach axis [$g axis names] {
        $g axis configure $axis -hide yes -checklimits no
    }
    # Presumably you want at least an X-axis and Y-axis displayed.
    $g xaxis configure -hide no
    $g yaxis configure -hide no
    array unset _label2axis

    #
    # Scan through all objects and create a list of all axes.
    # The first x-axis gets mapped to "x".  The second, to "x2".
    # Beyond that, we must create new axes "x3", "x4", etc.
    # We do the same for y.
    #
    set anum(x) 0
    set anum(y) 0
    foreach dataobj [get] {
        foreach ax {x y} {
            set label [$dataobj hints ${ax}label]
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
                    $g axis configure $axis -title $label -hide no \
                        -checklimits no
                    set _label2axis($ax-$label) $axis

                    # if this axis has a description, add it as a tooltip
                    set desc [string trim [$dataobj hints ${ax}desc]]
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
        set _axisPopup(format-$axis) "%.6g"

        $g axis bind $axis <Enter> \
            [itcl::code $this Axis hilite $axis on]
        $g axis bind $axis <Leave> \
            [itcl::code $this Axis hilite $axis off]
        $g axis bind $axis <ButtonPress-1> \
            [itcl::code $this Axis click $axis %x %y]
        $g axis bind $axis <B1-Motion> \
            [itcl::code $this Axis drag $axis %x %y]
        $g axis bind $axis <ButtonRelease-1> \
            [itcl::code $this Axis release $axis %x %y]
        $g axis bind $axis <KeyPress> \
            [list ::Rappture::Tooltip::tooltip cancel]
    }

    #
    # Plot all of the dataobjs.
    #
    set count 0
    foreach dataobj $_dlist {
        set label [$dataobj hints label]
        foreach {mapx mapy} [GetAxes $dataobj] break
        foreach comp [$dataobj components] {
            set xv [$dataobj mesh $comp]
            set yv [$dataobj values $comp]

            if {[info exists _dataobj2color($dataobj)]} {
                set color $_dataobj2color($dataobj)
            } else {
		set color black
            }
            if {[info exists _dataobj2width($dataobj)]} {
                set lwidth $_dataobj2width($dataobj)
            } else {
                set lwidth 2
            }
            if {[info exists _dataobj2dashes($dataobj)]} {
                set dashes $_dataobj2dashes($dataobj)
            } else {
                set dashes ""
            }
            if {([$xv length] <= 1) || ($lwidth == 0)} {
                set sym square
                set pixels 2
            } else {
                set sym ""
                set pixels 6
            }

            set elem "elem[incr _nextElement]"
            set _elem2dataobj($elem) $dataobj
            lappend label2elem($label) $elem
            $g element create $elem -x $xv -y $yv \
                -symbol $sym -pixels $pixels -linewidth $lwidth \
                -label $label \
                -color $color -dashes $dashes \
                -mapx $mapx -mapy $mapy
        }
    }

    # Fix duplicate labels by appending the simulation number
    foreach label [array names label2elem] {
        if { [llength $label2elem($label)] == 1 } {
            continue
        }
        foreach elem $label2elem($label) {
            set dataobj $_elem2dataobj($elem)
            regexp {^::curve(?:Value)?([0-9]+)$} $dataobj match suffix
            incr suffix
            set elabel [format "%s \#%d" $label $suffix]
            $g element configure $elem -label $elabel
        }
    }        

    foreach dataobj $_dlist {
        set xmin -Inf
        set ymin -Inf
        set xmax Inf
        set ymax Inf
        # 
        # Create text/line markers for each *axis.marker specified. 
        # 
	if { [$dataobj info class] == "Rappture::Curve" } {
	    foreach m [$dataobj xmarkers] {
		foreach {at label style} $m break
		set id [$g marker create line \
			    -coords [list $at $ymin $at $ymax]]
		$g marker bind $id <Enter> \
		    [itcl::code $this EnterMarker $g x-$label $at $ymin $at]
		$g marker bind $id <Leave> \
		    [itcl::code $this LeaveMarker $g x-$label]
		set options [GetLineMarkerOptions $style]
		if { $options != "" } {
		    eval $g marker configure $id $options
		}
		if { $label != "" } {
		    set id [$g marker create text -anchor nw \
				-text $label -coords [list $at $ymax]]
		    set options [GetTextMarkerOptions $style]
		    if { $options != "" } {
			eval $g marker configure $id $options
		    }
		}
	    }
	    foreach m [$dataobj ymarkers] {
		foreach {at label style} $m break
		set id [$g marker create line \
			    -coords [list $xmin $at $xmax $at]]
		$g marker bind $id <Enter> \
		    [itcl::code $this EnterMarker $g y-$label $at $xmin $at]
		$g marker bind $id <Leave> \
		    [itcl::code $this LeaveMarker $g y-$label]
		set options [GetLineMarkerOptions $style]
		if { $options != "" } {
		    eval $g marker configure $id $options
		}
		if { $label != "" } {
		    set id [$g marker create text -anchor se \
				-text $label -coords [list $xmax $at]]
		    set options [GetTextMarkerOptions $style]
		    if { $options != "" } {
			eval $g marker configure $id $options
		    }
		}
	    }
	}
    }
    $itk_component(legend) reset 
}

# ----------------------------------------------------------------------
# USAGE: ResetLimits
#
# Used internally to apply automatic limits to the axes for the
# current plot.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::ResetLimits {} {
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
                if {$min == 0} { set min 1 }
                set max $_limits(${axis}log-max)
                if {$max == 0} { set max 1 }

                if {$min == $max} {
                    set logmin [expr {floor(log10(abs(0.9*$min)))}]
                    set logmax [expr {ceil(log10(abs(1.1*$max)))}]
                } else {
                    set logmin [expr {floor(log10(abs($min)))}]
                    set logmax [expr {ceil(log10(abs($max)))}]
                    if 0 {
                    if {[string match y* $axis]} {
                        # add a little padding
                        set delta [expr {$logmax-$logmin}]
                        if {$delta == 0} { set delta 1 }
                        set logmin [expr {$logmin-0.05*$delta}]
                        set logmax [expr {$logmax+0.05*$delta}]
                    }
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

                if 0 {
                if {[string match y* $axis]} {
                    # add a little padding
                    set delta [expr {$max-$min}]
                    set min [expr {$min-0.05*$delta}]
                    set max [expr {$max+0.05*$delta}]
                }
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
# USAGE: Zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::Zoom {option args} {
    switch -- $option {
        reset {
            ResetLimits
            Rappture::Logger::log curve zoom -reset
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Hilite <state> <x> <y>
#
# Called automatically when the user brushes one of the elements
# on the plot.  Causes the element to highlight and a tooltip to
# pop up with element info.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::Hilite {state x y} {
    set g $itk_component(plot)
    set elem ""
  
    # Peek inside of Blt_ZoomStack package to see if we're currently in the
    # middle of a zoom selection.
    if {[info exists ::zoomInfo($g,corner)] && $::zoomInfo($g,corner) == "B" } {
        return;
    }
    set tip ""
    if {$state == "at"} {
        if {[$g element closest $x $y info -interpolate yes]} {
            # for dealing with xy line plots
            set elem $info(name)

            # Some elements are generated dynamically and therefore will
            # not have a data object associated with them.
            set mapx [$g element cget $elem -mapx]
            set mapy [$g element cget $elem -mapy]
            if {[info exists _elem2dataobj($elem)]} {
                foreach {mapx mapy} [GetAxes $_elem2dataobj($elem)] break
            }

            # search again for an exact point -- this time don't interpolate
            set tip ""
            array unset info
            if {[$g element closest $x $y info -interpolate no]
                  && $info(name) == $elem} {

                set x [$g axis transform $mapx $info(x)]
                set y [$g axis transform $mapy $info(y)]
                
                if {[info exists _elem2dataobj($elem)]} {
                    set dataobj $_elem2dataobj($elem)
                    set yunits [$dataobj hints yunits]
                    set xunits [$dataobj hints xunits]
                } else {
                    set xunits ""
                    set yunits ""
                }
                set tip [$g element cget $elem -label]
                set yval [Axis format y dummy $info(y)]
                append tip "\n$yval$yunits"
                set xval [Axis format x dummy $info(x)]
                append tip " @ $xval$xunits"
                set tip [string trim $tip]
            }
            set state 1
        } elseif {[$g element closest $x $y info -interpolate no]} {
            # for dealing with xy scatter plot
            set elem $info(name)

            # Some elements are generated dynamically and therefore will
            # not have a data object associated with them.
            set mapx [$g element cget $elem -mapx]
            set mapy [$g element cget $elem -mapy]
            if {[info exists _elem2dataobj($elem)]} {
                foreach {mapx mapy} [GetAxes $_elem2dataobj($elem)] break
            }

            set tip ""
            set x [$g axis transform $mapx $info(x)]
            set y [$g axis transform $mapy $info(y)]
                
            if {[info exists _elem2dataobj($elem)]} {
                set dataobj $_elem2dataobj($elem)
                set yunits [$dataobj hints yunits]
                set xunits [$dataobj hints xunits]
            } else {
                set xunits ""
                set yunits ""
            }
            set tip [$g element cget $elem -label]
            set yval [Axis format y dummy $info(y)]
            append tip "\n$yval$yunits"
            set xval [Axis format x dummy $info(x)]
            append tip " @ $xval$xunits"
            set tip [string trim $tip]
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
        if { [$g element exists $_hilite(elem)] && $_hilite(elem) != $elem } {
            $g element deactivate $_hilite(elem)
            $g crosshairs configure -hide yes
            Rappture::Tooltip::tooltip cancel
        }
        $g element activate $elem
        set _hilite(elem) $elem

        set mapx [$g element cget $elem -mapx]
        set mapy [$g element cget $elem -mapy]
        if {[info exists _elem2dataobj($elem)]} {
            foreach {mapx mapy} [GetAxes $_elem2dataobj($elem)] break
        }
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
            Rappture::Logger::log tooltip -for "curve probe -- [string map [list \n " // "] $tip]"
        }
    } else {
        #
        # Highlight OFF:
        # - deactivate (color back to normal)
        # - put all axes back to normal color
        # - take down tooltip
        #
        if { [$g element exists $_hilite(elem)] } {
            $g element deactivate $_hilite(elem)
        }
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

        $g crosshairs configure -hide yes

        # only cancel in plotting area or we'll mess up axes
        if {[$g inside $x $y]} {
            Rappture::Tooltip::tooltip cancel
        }

        # There is no currently highlighted element
        set _hilite(elem) ""
    }
}

# ----------------------------------------------------------------------
# USAGE: Axis hilite <axis> <state>
#
# USAGE: Axis click <axis> <x> <y>
# USAGE: Axis drag <axis> <x> <y>
# USAGE: Axis release <axis> <x> <y>
#
# USAGE: Axis edit <axis>
# USAGE: Axis changed <axis> <what>
# USAGE: Axis format <axis> <widget> <value>
# USAGE: Axis scale <axis> linear|log
#
# Used internally to handle editing of the x/y axes.  The hilite
# operation causes the axis to light up.  The edit operation pops
# up a panel with editing options.  The changed operation applies
# changes from the panel.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::Axis {option args} {
    set inner [$itk_component(hull).axes component inner]
    switch -- $option {
        hilite {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"Axis hilite axis state\""
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
                error "wrong # args: should be \"Axis click axis x y\""
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
                error "wrong # args: should be \"Axis drag axis x y\""
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
                error "wrong # args: should be \"Axis release axis x y\""
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
                    Axis edit $axis
                    return
                }
            }
            # one last movement
            Axis drag $axis $x $y

            # log this change
            Rappture::Logger::log curve axis $axis \
                -drag [$itk_component(plot) axis limits $axis]

            catch {unset _axis}
        }
        edit {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"Axis edit axis\""
            }
            set axis [lindex $args 0]
            set _axisPopup(current) $axis

            # apply last value when deactivating
            $itk_component(hull).axes configure -deactivatecommand \
                [itcl::code $this Axis changed $axis focus]

            # fix axis label controls...
            set label [$itk_component(plot) axis cget $axis -title]
            $inner.label delete 0 end
            $inner.label insert end $label
            bind $inner.label <Return> \
                [itcl::code $this Axis changed $axis label]
            bind $inner.label <KP_Enter> \
                [itcl::code $this Axis changed $axis label]
            bind $inner.label <FocusOut> \
                [itcl::code $this Axis changed $axis label]

            # fix min/max controls...
            foreach {min max} [$itk_component(plot) axis limits $axis] break
            $inner.min delete 0 end
            $inner.min insert end $min
            bind $inner.min <Return> \
                [itcl::code $this Axis changed $axis min]
            bind $inner.min <KP_Enter> \
                [itcl::code $this Axis changed $axis min]
            bind $inner.min <FocusOut> \
                [itcl::code $this Axis changed $axis min]

            $inner.max delete 0 end
            $inner.max insert end $max
            bind $inner.max <Return> \
                [itcl::code $this Axis changed $axis max]
            bind $inner.max <KP_Enter> \
                [itcl::code $this Axis changed $axis max]
            bind $inner.max <FocusOut> \
                [itcl::code $this Axis changed $axis max]

            # fix format control...
            set fmts [$inner.format choices get -value]
            set i [lsearch -exact $fmts $_axisPopup(format-$axis)]
            if {$i < 0} { set i 0 }  ;# use Auto choice
            $inner.format value [$inner.format choices get -label $i]

            bind $inner.format <<Value>> \
                [itcl::code $this Axis changed $axis format]

            # fix scale control...
            if {[$itk_component(plot) axis cget $axis -logscale]} {
                set _axisPopup(scale) "log"
                $inner.format configure -state disabled
            } else {
                set _axisPopup(scale) "linear"
                $inner.format configure -state normal
            }
            $inner.scales.linear configure \
                -command [itcl::code $this Axis changed $axis scale]
            $inner.scales.log configure \
                -command [itcl::code $this Axis changed $axis scale]

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
                error "wrong # args: should be \"Axis changed axis what\""
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
                    Rappture::Logger::log curve axis $axis -title $val
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
                        Rappture::Logger::log curve axis $axis -min $val
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
                        Rappture::Logger::log curve axis $axis -max $val
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
                    Axis scale $axis $_axisPopup(scale)
                    Rappture::Logger::log curve axis $axis -scale $_axisPopup(scale)

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
                error "wrong # args: should be \"Axis format axis widget value\""
            }
            set axis [lindex $args 0]
            set value [lindex $args 2]

            if {[$itk_component(plot) axis cget $axis -logscale]} {
                set fmt "%.6g"
            } else {
                set fmt $_axisPopup(format-$axis)
            }
            return [format $fmt $value]
        }
        scale {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"Axis scale axis type\""
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
                    [itcl::code $this Axis format $axis]
            }
        }
        default {
            error "bad option \"$option\": should be changed, edit, hilite, or format"
        }
    }
}


# ----------------------------------------------------------------------
# USAGE: GetLineMarkerOptions <style>
#
# Used internally to create a list of configuration options specific to the
# axis line marker.  The input is a list of name value pairs.  Options that
# are not recognized are ignored.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::GetLineMarkerOptions {style} {
    array set lineOptions {
        "-color"  "-outline"
        "-dashes" "-dashes"
        "-linecolor" "-outline"
        "-linewidth" "-linewidth"
    }
    set options {}
    foreach {name value} $style {
        if { [info exists lineOptions($name)] } {
            lappend options $lineOptions($name) $value
        }
    }
    return $options
}

# ----------------------------------------------------------------------
# USAGE: GetTextMarkerOptions <style>
#
# Used internally to create a list of configuration options specific to the
# axis text marker.  The input is a list of name value pairs.  Options that
# are not recognized are ignored.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::GetTextMarkerOptions {style} {
    array set textOptions {
        "-color"        "-outline"
        "-textcolor"    "-outline"
        "-font"         "-font"
        "-xoffset"      "-xoffset"
        "-yoffset"      "-yoffset"
        "-anchor"       "-anchor"
        "-rotate"       "-rotate"
    }
    set options {}
    foreach {name value} $style {
        if { [info exists textOptions($name)] } {
            lappend options $textOptions($name) $value
        }
    }
    return $options
}

# ----------------------------------------------------------------------
# USAGE: GetAxes <dataobj>
#
# Used internally to figure out the axes used to plot the given
# <dataobj>.  Returns a list of the form {x y}, where x is the
# x-axis name (x, x2, x3, etc.), and y is the y-axis name.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::GetAxes {dataobj} {
    # rebuild if needed, so we know about the axes
    if 0 {
        # Don't do this. Given dataobj may be deleted in the rebuild

        # rebuild if needed, so we know about the axes
        if {[$_dispatcher ispending !rebuild]} {
            $_dispatcher cancel !rebuild
            $_dispatcher event -now !rebuild
        }
    }
    # what is the x axis?  x? x2? x3? ...
    set xlabel [$dataobj hints xlabel]
    if {[info exists _label2axis(x-$xlabel)]} {
        set mapx $_label2axis(x-$xlabel)
    } else {
        set mapx "x"
    }

    # what is the y axis?  y? y2? y3? ...
    set ylabel [$dataobj hints ylabel]
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

itcl::body Rappture::XyResult::EnterMarker { g name x y text } {
    LeaveMarker $g $name
    set id [$g marker create text \
                -coords [list $x $y] \
                -yoffset -1 \
                -anchor s \
                -text $text]
    set _markers($name) $id
}

itcl::body Rappture::XyResult::LeaveMarker { g name } {
    if { [info exists _markers($name)] } { 
        set id $_markers($name)
        $g marker delete $id
        unset _markers($name)
    }
}
