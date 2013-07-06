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
    private variable _dataobj2barwidth ;# maps dataobj => type of graph element
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

    private method FormatAxis { axis w value }
    private method GetFormattedValue { axis g value }
    private method BuildAxisPopup { popup }
    private method ShowAxisPopup { axis }
    private method SetAxis { setting }
    private method SetAxisRangeState { axis }
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
        -barwidth 1
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
        set _dataobj2barwidth($dataobj) $params(-barwidth)
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
            array unset _dataobj2color    $dataobj
            array unset _dataobj2width    $dataobj
            array unset _dataobj2dashes   $dataobj
            array unset _dataobj2raise    $dataobj
            array unset _dataobj2type     $dataobj
            array unset _dataobj2barwidth $dataobj
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
    set g $itk_component(plot)


    set allx [$itk_component(plot) x2axis use]
    lappend allx x  ;# fix main x-axis too

    set ally [$itk_component(plot) y2axis use]
    lappend ally y  ;# fix main y-axis too
    catch {unset _limits}
    
    eval $g element delete [$g element names]
    foreach dataobj $args {
        set label [$dataobj hints label]
        foreach {mapx mapy} [GetAxes $dataobj] break
        foreach comp [$dataobj components] {
            set xv [$dataobj mesh $comp]
            set yv [$dataobj values $comp]

            if {[info exists _dataobj2type($dataobj)]} {
                set type $_dataobj2type($dataobj)
            } else {
                set type "line"
            }
            if {[info exists _dataobj2barwidth($dataobj)]} {
                set barwidth $_dataobj2barwidth($dataobj)
            } else {
                set barwidth 1.0
            }
            if {[info exists _dataobj2width($dataobj)]} {
                set lwidth $_dataobj2width($dataobj)
            } else {
                set lwidth 2
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
            switch -- $type {
                "line" - "scatter" {
                    $g element create $elem -x $xv -y $yv \
                        -symbol $sym -pixels $pixels -linewidth $lwidth \
                        -mapx $mapx -mapy $mapy
                } "bar" {
                    $g bar create $elem -x $xv -y $yv \
                        -barwidth $barwidth \
                        -mapx $mapx -mapy $mapy
                }
            }
        }
    }
    foreach axis {x y} {
        if { [info exists _limits({$axis}log)] } {
            set type "log"
            $g axis configure -logscale 1 
        } else {
            set type "lin"
        }
        foreach {min max} [$g axis limits $axis] break
        set _limits(${axis}-min) $min
        set _limits(${axis}-max) $max
        set min [$dataobj hints ${axis}min]
        set max [$dataobj hints ${axis}max]
        if {"" != $min } {
            set _limits(${axis}-min) $min
        }
        if {"" != $max } {
            set _limits(${axis}-max) $max
        }
    }
    eval $g element delete [$g element names]
    if 0 {
    foreach dataobj $args {
        # Find the axes for this dataobj (e.g., {x y2})
        foreach {map(x) map(y)} [GetAxes $dataobj] break
        foreach axis {x y} {
            if {[$dataobj hints ${axis}scale] == "log"} {
                set _limits(${axis}log) 1
            }
            # Get defaults for both linear and log scales
            foreach type {lin log} {
                # store results -- ex: _limits(x2log-min)
                set id $map($axis)$type
                set min [$dataobj hints ${axis}min]
                set max [$dataobj hints ${axis}max]
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

itcl::body Rappture::XyResult::BuildMarkers { dataobj elem } {
    set g $itk_component(plot)

    foreach m [$dataobj xmarkers] {
        foreach {at label style} $m break
        set id [$g marker create line -coords [list $at $ymin $at $ymax]]
        $g marker bind $id <Enter> \
            [itcl::code $this EnterMarker $g x-$label $at $ymin $at]
        $g marker bind $id <Leave> \
            [itcl::code $this LeaveMarker $g x-$label]
        set options [GetLineMarkerOptions $style]
        $g marker configure $id -element $elem
        if { $options != "" } {
            eval $g marker configure $id $options
        }
        if { $label != "" } {
            set id [$g marker create text -anchor nw \
                        -text $label -coords [list $at $ymax]]
            $g marker configure $id -element $elem
            set options [GetTextMarkerOptions $style]
            if { $options != "" } {
                eval $g marker configure $id $options
            }
        }
    }
    foreach m [$dataobj ymarkers] {
        foreach {at label style} $m break
        set id [$g marker create line -coords [list $xmin $at $xmax $at]]
        $g marker configure $id -element $elem
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
            $g marker configure $id -element $elem
            set options [GetTextMarkerOptions $style]
            if { $options != "" } {
                eval $g marker configure $id $options
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: BuildElementsAndMarkers
#
#       This does what "Rebuild" used to.  It (re)creates all the  
#       the elements and markers for the graph based on the data objects
#       given.  The axes are also set if min and max have been set for
#       any data object.  
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::BuildElementsAndMarkers { dlist } {
    set g $itk_component(plot)

    # First clear out the widget and hide the axes.
    eval $g element delete [$g element names]
    eval $g marker delete [$g marker names]
    foreach label [array names _label2axis] {
        set axis $_label2axis($label)
        switch -- $axis {
            "x" - "x2" - "y" - "y2" {
                # Do nothing
                $g axis configure $axis -hide yes
            }
            default {
                $g axis delete $axis
            }
    }
    array unset _label2axis
    array unset _limits

    # Scan through all objects and create a list of all axes.
    # The first x-axis gets mapped to "x".  The second, to "x2".
    # Beyond that, we must create new axes "x3", "x4", etc.
    # We do the same for y.

    set anum(x) 0
    set anum(y) 0
    foreach dataobj $dlist {
        foreach axis {x y} {
            set label [$dataobj hints ${axis}label]
            if { $label == "" } {
                continue
            }
            # Collect the limits (if set for the axis)
            set min [$dataobj hints ${axis}min]
            set max [$dataobj hints ${axis}max]
            if { $min != "" && (![info exists _limits(${label}-min)] || 
                                $_limits(${label}-min) > $min) } {
                set _limits(${label}-min} $min
            }
            if { $max != "" && (![info exists _limits(${label}-max)] ||
                                $_limits(${label}-max) < $max) } {
                set _limits(${label}-max} $max
            }
            if  {[$dataobj hints ${axis}scale] == "log"} {
                set _limits(${axis}log) 1
            }
            if {![info exists _label2axis($label)]} {
                switch [incr anum($axis)] {
                    1 { set axisName $axis }
                    2 { set axisName ${axis}2 }
                    default {
                        set axis $axis$anum($axis)
                        catch {$g axis create $axisName}
                    }
                }
                $g axis configure $axisName -title $label -hide no \
                    -checklimits no
                set _label2axis($label) $axisName
                
                # if this axis has a description, add it as a tooltip
                set desc [string trim [$dataobj hints ${axis}desc]]
                Rappture::Tooltip::text $g-$axisName $desc
            }
        }
    }

    # Next set the axes based on what we've found.
    foreach label [array names _label2axis] {
        if { [info exist _limits(${label}log)] } {
            set logscale 1
        } else {
            set logscale 0
        }
        set amin ""
        if { [info exists _limits(${label}-min)] } {
            set amin $_limits(${label}-min)
        }
        set amax ""
        if { [info exists _limits(${label}-max)] } {
            set amax $_limits(${label}-max)
        }
        set axis $_label2axis($label)
        $g axis configure $axis \
            -hide no -checklimits no \
            -command [itcl::code $this GetFormattedValue $axis] \
            -min $amin -max $amax -logscale $logscale
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

    # Generate all the data elements and markers, but mark them as hidden.
    # The Rebuild method will un-hide them.
    set count 0
    foreach dataobj $dlist {
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
            if {[info exists _dataobj2type($dataobj)]} {
                set type $_dataobj2type($dataobj)
            } else {
                set type "line"
            }
            if {[info exists _dataobj2barwidth($dataobj)]} {
                set barwidth $_dataobj2barwidth($dataobj)
            } else {
                set barwidth 1.0
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
            switch -- $type {
                "line" - "scatter" {
                    $g element create $elem -x $xv -y $yv \
                        -symbol $sym -pixels $pixels -linewidth $lwidth \
                        -label $label \
                        -color $color -dashes $dashes \
                        -mapx $mapx -mapy $mapy -hide yes
                } "bar" {
                    $g bar create $elem -x $xv -y $yv \
                        -barwidth $barwidth \
                        -label $label \
                        -color $color \
                        -mapx $mapx -mapy $mapy -hide yes
                }
            }
            if { [$dataobj info class] == "Rappture::Curve" } {
                BuildMarkers $dataobj $elem
            }
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
    $itk_component(legend) reset 
}

# ----------------------------------------------------------------------
# USAGE: Rebuild
#
#       Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::Rebuild {} {
    set g $itk_component(plot)

    # First clear out the widget
    eval $g element delete [$g element names]
    eval $g marker delete [$g marker names]

    foreach axis [$g axis names] {
        if { [info exist _limits(${axis}log)] } {
            set type "log"
            set logscale 1
        } else {
            set type "lin"
            set logscale 0
        }
        set amin ""
        if { [info exists _limits(${axis}-min)] } {
            set amin $_limits(${axis}-min)
        }
        set amax ""
        if { [info exists _limits(${axis}-max)] } {
            set amax $_limits(${axis}-max)
        }
        $g axis configure $axis \
            -hide yes -checklimits no \
            -command [itcl::code $this GetFormattedValue $axis] \
            -min $amin -max $amax -logscale $logscale
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
            if {[info exists _dataobj2type($dataobj)]} {
                set type $_dataobj2type($dataobj)
            } else {
                set type "line"
            }
            if {[info exists _dataobj2barwidth($dataobj)]} {
                set barwidth $_dataobj2barwidth($dataobj)
            } else {
                set barwidth 1.0
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
            switch -- $type {
                "line" - "scatter" {
                    $g element create $elem -x $xv -y $yv \
                        -symbol $sym -pixels $pixels -linewidth $lwidth \
                        -label $label \
                        -color $color -dashes $dashes \
                        -mapx $mapx -mapy $mapy
                } "bar" {
                    $g bar create $elem -x $xv -y $yv \
                        -barwidth $barwidth \
                        -label $label \
                        -color $color \
                        -mapx $mapx -mapy $mapy
                }
            }
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

    foreach axis [$g axis names] {
        $g axis configure $axis -min "" -max ""
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
                set yval [GetFormattedValue y $g $info(y)]
                append tip "\n$yval$yunits"
                set xval [GetFormattedValue x $g $info(x)]
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
            set yval [GetFormattedValue y $g $info(y)]
            append tip "\n$yval$yunits"
            set xval [GetFormattedValue x $g $info(x)]
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
                    set tipx "-[expr {$x-20}]"  ;# move tooltip to the left
                }
            } else {
                if {$x < -4} {
                    set tipx "+0"
                } else {
                    set tipx "+[expr {$x+20}]"  ;# move tooltip to the right
                }
            }
            if {$y > 0.5*[winfo height $g]} {
                if {$y < 4} {
                    set tipy "-0"
                } else {
                    set tipy "-[expr {$y-20}]"  ;# move tooltip to the top
                }
            } else {
                if {$y < -4} {
                    set tipy "+0"
                } else {
                    set tipy "+[expr {$y+20}]"  ;# move tooltip to the bottom
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
# Used internally to handle editing of the x/y axes.  The hilite
# operation causes the axis to light up.  The edit operation pops
# up a panel with editing options.  The changed operation applies
# changes from the panel.
# ----------------------------------------------------------------------
itcl::body Rappture::XyResult::Axis {option args} {
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
                    ShowAxisPopup $axis
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
        default {
            error "bad option \"$option\": should be hilite"
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

#
# SetAxis --
#
#       Configures the graph axis with the designated setting using
#       the currently stored value.  User-configurable axis settings 
#       are stored in the _axisPopup variable or in the widgets. This
#       routine syncs the graph with that setting.
#
itcl::body Rappture::XyResult::SetAxis { setting } {
    set g $itk_component(plot)
    set axis $_axisPopup(axis)
    switch -- $setting {
        "logscale" {
            set bool $_axisPopup(logscale)
            $g axis configure $axis -logscale $bool
        }
        "loose" {
            set bool $_axisPopup(loose)
            $g axis configure $axis -loose $bool
        }
        "range" {
            set auto $_axisPopup(auto)
            set _axisPopup($axis-auto) $auto
            if { $auto } {
                # Set the axis range automatically
                $g axis configure $axis -min "" -max ""
            } else {
                # Set the axis range from the entry values.
                set min $_axisPopup($axis-min)
                set max $_axisPopup($axis-max)
                $g axis configure $axis -min $min -max $max
            }
            SetAxisRangeState $axis
        }
        "format" {
            set inner [$itk_component(hull).axes component inner]
            set format [$inner.format translate [$inner.format value]]
            set _axisPopup($axis-format) $format

            # Force the graph to reformat the ticks
            set min [$itk_component(plot) axis cget $axis -min]
            $g axis configure $axis -min $min
        }
        "label" {
            set label $_axisPopup(label)
            $g axis configure $axis -label $label
        }
        "min" {
            set min $_axisPopup(min)
            if { [catch { $g axis configure $axis -min $min } msg] != 0 } {
                set inner [$itk_component(hull).axes component inner]
                Rappture::Tooltip::cue $inner.max $msg
                bell
                return
            }
            set _axisPopup($axis-min) $min
        }
        "max" {
            set max $_axisPopup(max)
            if { [catch { $g axis configure $axis -max $max } msg] != 0 } {
                set inner [$itk_component(hull).axes component inner]
                Rappture::Tooltip::cue $inner.max $msg
                bell
                return
            }
            set _axisPopup($axis-max) $max
        }
    }
}

#
# SetAxisRangeState --
#
#       Sets the state of widgets controlling the axis range based
#       upon whether the automatic or manual setting.  If the 
#       axis is configure to be automatic, the manual setting widgets
#       are disabled.  And vesa-versa the automatic setting widgets
#       are dsiabled if the axis is manual.
#
itcl::body Rappture::XyResult::SetAxisRangeState { axis } {
    set inner [$itk_component(hull).axes component inner]
    set g $itk_component(plot)

    if { $_axisPopup(auto) } {
        foreach {min max} [$g axis limits $axis] break
        $inner.minl configure -state disabled 
        $inner.min configure -state disabled 
        $inner.maxl configure -state disabled 
        $inner.max configure -state disabled 
        $inner.loose configure -state normal
        $inner.tight configure -state normal
    } else {
        foreach {min max} [$g axis limits $axis] break
        $inner.minl configure -state normal 
        $inner.min configure -state normal 
        set _axisPopup(min) [$g axis cget $axis -min]
        $inner.maxl configure -state normal 
        $inner.max configure -state normal 
        set _axisPopup(max) [$g axis cget $axis -max]
        $inner.loose configure -state disabled
        $inner.tight configure -state disabled
    }
}

#
# BuildAxisPopup --
#
#       Creates the popup balloon dialog for axes. This routine is 
#       called only once the first time the user clicks to bring up 
#       an axis dialog.  It is reused for all other axes.  
#
itcl::body Rappture::XyResult::BuildAxisPopup { popup } {
    Rappture::Balloon $popup -title "Axis Options"
    set inner [$itk_component(hull).axes component inner]

    label $inner.labell -text "Label:"
    entry $inner.label \
        -width 15 -highlightbackground $itk_option(-background) \
        -textvariable [itcl::scope _axisPopup(label)]

    bind $inner.label <Return>   [itcl::code $this SetAxis label]
    bind $inner.label <KP_Enter> [itcl::code $this SetAxis label]
    bind $inner.label <FocusOut> [itcl::code $this SetAxis label]

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

    bind $inner.format <<Value>> [itcl::code $this SetAxis format]

    label $inner.rangel -text "Axis Range:"
    radiobutton $inner.auto -text "Automatic" \
        -variable [itcl::scope _axisPopup(auto)] -value 1 \
        -command [itcl::code $this SetAxis range]
    radiobutton $inner.manual -text "Manual" \
        -variable [itcl::scope _axisPopup(auto)] -value 0 \
        -command [itcl::code $this SetAxis range]

    radiobutton $inner.loose -text "loose" \
        -variable [itcl::scope _axisPopup(loose)] -value 1 \
        -command [itcl::code $this SetAxis loose]
    radiobutton $inner.tight -text "tight" \
        -variable [itcl::scope _axisPopup(loose)] -value 0 \
        -command [itcl::code $this SetAxis loose]

    label $inner.minl -text "min"
    entry $inner.min \
        -width 15 -highlightbackground $itk_option(-background) \
        -textvariable [itcl::scope _axisPopup(min)]
    bind $inner.min <Return> [itcl::code $this SetAxis min]
    bind $inner.min <KP_Enter> [itcl::code $this SetAxis min]
    bind $inner.min <FocusOut> [itcl::code $this SetAxis min]

    label $inner.maxl -text "max"
    entry $inner.max \
        -width 15 -highlightbackground $itk_option(-background) \
        -textvariable [itcl::scope _axisPopup(max)]
    bind $inner.max <Return> [itcl::code $this SetAxis max]
    bind $inner.max <KP_Enter> [itcl::code $this SetAxis max]
    bind $inner.max <FocusOut> [itcl::code $this SetAxis max]


    label $inner.scalel -text "Scale:"
    radiobutton $inner.linear -text "linear" \
        -variable [itcl::scope _axisPopup(logscale)] -value 0 \
        -command [itcl::code $this SetAxis logscale]
    radiobutton $inner.log -text "logarithmic" \
        -variable [itcl::scope _axisPopup(logscale)] -value 1 \
        -command [itcl::code $this SetAxis logscale]

    blt::table $inner \
        0,0 $inner.labell -anchor w \
        0,1 $inner.label -anchor w -fill x  -cspan 3 \
        1,0 $inner.formatl -anchor w \
        1,1 $inner.format -anchor w -fill x  -cspan 3 \
        2,0 $inner.scalel -anchor w \
        2,2 $inner.linear -anchor w \
        2,3 $inner.log -anchor w \
        3,0 $inner.rangel -anchor w \
        4,0 $inner.manual -anchor w -padx 4 \
        4,2 $inner.minl -anchor e \
        4,3 $inner.min -anchor w \
        5,2 $inner.maxl -anchor e \
        5,3 $inner.max -anchor w \
        6,0 $inner.auto -anchor w -padx 4 \
        6,2 $inner.tight -anchor w \
        6,3 $inner.loose -anchor w \
        

    blt::table configure $inner r2 -pady 4
    blt::table configure $inner c1 -width 20
    update
}

#
# ShowAxisPopup --
#
#       Displays the axis dialog for an axis.  It initializes the 
#       _axisInfo variables for that axis if necessary.
#
itcl::body Rappture::XyResult::ShowAxisPopup { axis } {
    set g $itk_component(plot)
    set popup $itk_component(hull).axes 

    if { ![winfo exists $popup] } {
        BuildAxisPopup $popup
    }
    set _axisPopup(axis)     $axis
    set _axisPopup(label)    [$g axis cget $axis -title]
    set _axisPopup(logscale) [$g axis cget $axis -logscale]
    set _axisPopup(loose)    [$g axis cget $axis -loose]
    if { ![info exists _axisPopup($axis-format)] } {
        set inner [$itk_component(hull).axes component inner]
        set _axisPopup($axis-format) "%.6g"
        set fmts [$inner.format choices get -value]
        set i [lsearch -exact $fmts $_axisPopup($axis-format)]
        if {$i < 0} { set i 0 }  ;# use Auto choice
        $inner.format value [$inner.format choices get -label $i]
    }
    foreach {min max} [$g axis limits $axis] break
    if { $_axisPopup(logscale) } {
        set type "log"
    } else {
        set type "lin"
    }
    set amin ""
    if { [info exists _limits(${axis}${type}-min)] } {
        set amin $_limits(${axis}${type}-min)
    }
    set amax ""
    if { [info exists _limits(${axis}${type}-max)] } {
        set amax $_limits(${axis}${type}-max)
    }
    set auto 1 
    if { $amin != "" || $amax != "" } {
        set auto 0
    }
    if { ![info exists _axisPopup($axis-auto)] } {
        set _axisPopup($axis-auto) $auto;# Defaults to automatic
    }
    set _axisPopup(auto)  $_axisPopup($axis-auto)
    SetAxisRangeState $axis
    if { ![info exists _axisPopup($axis-min)] } {
        if { $amin != "" } {
            set _axisPopup($axis-min) $amin
            set _axisPopup(min)   $_axisPopup($axis-min)
            SetAxis min
        } else {
            set _axisPopup($axis-min) $min
        }
    }
    if { ![info exists _axisPopup($axis-max)] } {
        if { $amax != "" } {
            set _axisPopup($axis-max) $amax
            set _axisPopup(max)   $_axisPopup($axis-max)
            SetAxis max
        } else {
            set _axisPopup($axis-max) $max
        }
    }
    set _axisPopup(min)   $_axisPopup($axis-min)
    set _axisPopup(max)   $_axisPopup($axis-max)
    set _axisPopup(axis) $axis

    #
    # Figure out where the window should pop up.
    #
    set x [winfo rootx $g]
    set y [winfo rooty $g]
    set w [winfo width $g]
    set h [winfo height $g]
    foreach {x0 y0 pw ph} [$g extents plotarea] break
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
            set ally [$g y2axis use]
            set max [llength $ally]
            set i [lsearch -exact $ally $axis]
            set y [expr {round($y + ($i+0.5)*$y0/double($max))}]
            set x [expr {round($x+$x0+$pw + ($i+0.5)*($w-$x0-$pw)/double($max))}]
        }
    }
    $popup activate @$x,$y $dir
}

#
# GetFormattedValue --
#
#       Callback routine for the axis format procedure.  It formats the
#       axis tick label according to the selected format.  This routine 
#       is also used to format tooltip values.
#
itcl::body Rappture::XyResult::GetFormattedValue { axis g value } {
    if { [$g axis cget $axis -logscale] || 
         ![info exists _axisPopup($axis-format)] } {
        set fmt "%.6g"
    } else {
        set fmt $_axisPopup($axis-format)
    }
    return [format $fmt $value]
}
