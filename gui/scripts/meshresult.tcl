# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: meshresult - mesh plot in a ResultSet
#
#  This widget is a mesh plot, meant to view grid structures produced
#  as output from the run of a Rappture tool.  Use the "add" and
#  "delete" methods to control the meshes showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *MeshResult.width 4i widgetDefault
option add *MeshResult.height 4i widgetDefault
option add *MeshResult.gridColor #d9d9d9 widgetDefault
option add *MeshResult.regionColors {green yellow orange red magenta} widgetDefault
option add *MeshResult.controlBackground gray widgetDefault
option add *MeshResult.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::MeshResult {
    inherit itk::Widget

    itk_option define -gridcolor gridColor GridColor ""
    itk_option define -regioncolors regionColors RegionColors ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method parameters {title args} { # do nothing }
    public method download {option args}

    protected method _rebuild {}
    protected method _fixLimits {}
    protected method _zoom {option args}
    protected method _hilite {state x y}

    private variable _dlist ""     ;# list of dataobj objects
    private variable _dobj2color   ;# maps dataobj => plotting color
    private variable _dobj2width   ;# maps dataobj => line width
    private variable _dobj2dashes  ;# maps dataobj => BLT -dashes list
    private variable _dobj2raise   ;# maps dataobj => raise flag 0/1
    private variable _mrkr2tip     ;# maps graph element => tooltip
    private variable _xmin ""      ;# autoscale min for x-axis
    private variable _xmax ""      ;# autoscale max for x-axis
    private variable _ymin ""      ;# autoscale min for y-axis
    private variable _ymax ""      ;# autoscale max for y-axis
}

itk::usual MeshResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MeshResult::constructor {args} {
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
        blt::graph $itk_interior.plot \
            -highlightthickness 0 -plotpadx 0 -plotpady 0 \
            -rightmargin 10 -invertxy 1
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(plot) -expand yes -fill both

    # special pen for highlighting active traces
    $itk_component(plot) marker bind all <Enter> \
        [itcl::code $this _hilite on %x %y]
    $itk_component(plot) marker bind all <Leave> \
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
itcl::body Rappture::MeshResult::destructor {} {
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a dataobj to the plot.  The optional <settings>
# are used to configure the plot.  Allowed settings are -color,
# -brightness, -width, -linestyle and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::MeshResult::add {dataobj {settings ""}} {
    array set params {
        -color auto
        -brightness 0
        -width 1
        -raise 0
        -linestyle solid
        -description ""
        -param ""
    }
    array set params $settings
    if {$params(-color) == "auto" || $params(-color) == "autoreset"} {
        # can't handle -autocolors yet
        set params(-color) black
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

    set pos [lsearch -exact $_dlist $dataobj]
    if {$pos < 0} {
        lappend _dlist $dataobj
        set _dobj2color($dataobj) $params(-color)
        set _dobj2width($dataobj) $params(-width)
        set _dobj2dashes($dataobj) $params(-linestyle)
        #set _dobj2raise($dataobj) $params(-raise)
        set _dobj2raise($dataobj) 0

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
itcl::body Rappture::MeshResult::get {} {
    # put the dataobj list in order according to -raise options
    set dlist $_dlist
    foreach obj $dlist {
        if {[info exists _dobj2raise($obj)] && $_dobj2raise($obj)} {
            set i [lsearch -exact $dlist $obj]
            if {$i >= 0} {
                set dlist [lreplace $dlist $i $i]
                lappend dlist $obj
            }
        }
    }
    return $dlist
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj> <dataobj> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::MeshResult::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            catch {unset _dobj2color($dataobj)}
            catch {unset _dobj2width($dataobj)}
            catch {unset _dobj2dashes($dataobj)}
            catch {unset _dobj2raise($dataobj)}
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
# USAGE: scale ?<dataobj1> <dataobj2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <dataobj> objects.  This
# accounts for all dataobjs--even those not showing on the screen.
# Because of this, the limits are appropriate for all dataobjs as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::MeshResult::scale {args} {
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
# USAGE: download coming
# USAGE: download controls <downloadCommand>
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::MeshResult::download {option args} {
    switch $option {
        coming {
            # nothing to do
        }
        controls {
            # no controls for this download yet
            return ""
        }
        now {
            set psdata [$itk_component(plot) postscript output -maxpect 1]

            set cmds {
                set fout "mesh[pid].pdf"
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
itcl::body Rappture::MeshResult::_rebuild {} {
    set g $itk_component(plot)
    blt::busy hold [winfo toplevel $g]

    # first clear out the widget
    eval $g marker delete [$g marker names]
    $g axis configure x -min "" -max "" -loose yes -checklimits no \
        -descending yes
    $g axis configure y -min "" -max "" -loose yes -checklimits no

    # extract axis information from the first dataobj
    set dlist [get]
    set xydata [lindex $dlist 0]
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

    set multiple [expr {[llength $dlist] > 1}]
    catch {unset _mrkr2tip}

    # plot all of the dataobjs
    set count 0
    foreach xydata $dlist {
        if {$multiple} {
            if {[info exists _dobj2color($xydata)]} {
                set color $_dobj2color($xydata)
            } else {
                set color [$xydata hints color]
                if {"" == $color} {
                    set color black
                }
            }

            if {[info exists _dobj2width($xydata)]} {
                set lwidth $_dobj2width($xydata)
            } else {
                set lwidth 2
            }
        } else {
            set color black
            set lwidth 1
        }

        if {[info exists _dobj2dashes($xydata)]} {
            set dashes $_dobj2dashes($xydata)
        } else {
            set dashes ""
        }

        foreach {plist r} [$xydata elements] {
            if {$count == 0} {
                if {$r == "unknown"} {
                    set fill gray
                } elseif {![info exists colors($r)]} {
                    set i [array size colors]
                    set fill [lindex $itk_option(-regioncolors) $i]
                    set colors($r) $fill
                } else {
                    set fill $colors($r)
                }
                set mrkr [$g marker create polygon -coords $plist -fill $fill]
                set _mrkr2tip($mrkr) $r
            }
            set mrkr [$g marker create line -coords $plist \
                -linewidth $lwidth -outline $color -dashes $dashes]
            set _mrkr2tip($mrkr) $r
        }
        incr count
    }

    _fixLimits
    blt::busy release [winfo toplevel $g]
}

# ----------------------------------------------------------------------
# USAGE: _fixLimits
#
# Used internally to apply automatic limits to the axes for the
# current plot.
# ----------------------------------------------------------------------
itcl::body Rappture::MeshResult::_fixLimits {} {
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
itcl::body Rappture::MeshResult::_zoom {option args} {
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
itcl::body Rappture::MeshResult::_hilite {state x y} {
    set mrkr [$itk_component(plot) marker get current]
    if {$state} {
        #
        # Highlight ON:
        # - pop up tooltip about data
        #
        set tip ""
        if {[info exists _mrkr2tip($mrkr)]} {
            set tip $_mrkr2tip($mrkr)
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
        # - take down tooltip
        #
        Rappture::Tooltip::tooltip cancel
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -gridcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::MeshResult::gridcolor {
    if {"" == $itk_option(-gridcolor)} {
        $itk_component(plot) grid off
    } else {
        $itk_component(plot) grid configure -color $itk_option(-gridcolor)
        $itk_component(plot) grid on
    }
}
