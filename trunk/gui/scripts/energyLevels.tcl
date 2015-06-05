# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: EnergyLevels - visualizer for discrete energy levels
#
#  This widget is a simple visualizer for a set of quantized energy
#  levels, as you might find for a molecule or a quantum well.  It
#  takes the Rappture XML representation for a <table> and extracts
#  values from the "energy" column, then plots those energies on a
#  graph.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *EnergyLevels.width 4i widgetDefault
option add *EnergyLevels.height 4i widgetDefault
option add *EnergyLevels.padding 4 widgetDefault
option add *EnergyLevels.controlBackground gray widgetDefault
option add *EnergyLevels.shadeColor gray widgetDefault
option add *EnergyLevels.levelColor black widgetDefault
option add *EnergyLevels.levelTextForeground black widgetDefault
option add *EnergyLevels.levelTextBackground white widgetDefault

option add *EnergyLevels.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::EnergyLevels {
    inherit itk::Widget

    itk_option define -padding padding Padding 0
    itk_option define -shadecolor shadeColor ShadeColor ""
    itk_option define -levelcolor levelColor LevelColor ""
    itk_option define -leveltextforeground levelTextForeground Foreground ""
    itk_option define -leveltextbackground levelTextBackground Background ""

    constructor {args} { # defined below }

    public proc columns {table}

    public method add {table {settings ""}}
    public method delete {args}
    public method get {}
    public method scale {args}
    public method download {args} {}
    public method parameters {title args} { # do nothing }

    protected method _redraw {{what all}}
    protected method _zoom {option args}
    protected method _view {midE delE}
    protected method _hilite {option args}
    protected method _getLayout {}

    private variable _dispatcher "" ;# dispatcher for !events

    private variable _dlist ""     ;# list of data objects
    private variable _dobj2color   ;# maps data obj => color option
    private variable _dobj2raise   ;# maps data obj => raise option
    private variable _dobj2desc    ;# maps data obj => description
    private variable _dobj2cols    ;# maps data obj => column names
    private variable _emin ""      ;# autoscale min for energy
    private variable _emax ""      ;# autoscale max for energy
    private variable _eviewmin ""  ;# min for "zoom" view
    private variable _eviewmax ""  ;# max for "zoom" view
    private variable _edefmin ""   ;# min for default "zoom" view
    private variable _edefmax ""   ;# max for default "zoom" view
    private variable _ehomo ""     ;# energy of HOMO level in topmost dataset
    private variable _lhomo ""     ;# label for HOMO level
    private variable _elumo ""     ;# energy of LUMO level in topmost dataset
    private variable _llumo ""     ;# label for LUMO level
    private variable _hilite ""    ;# item currently highlighted
    common _downloadPopup          ;# download options from popup
}

itk::usual EnergyLevels {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw "[itcl::code $this _redraw all]; list"
    $_dispatcher register !zoom
    $_dispatcher dispatch $this !zoom "[itcl::code $this _redraw zoom]; list"

    array set _downloadPopup {
        format csv
    }

    itk_option add hull.width hull.height
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
        rename -highlightbackground -controlbackground controlBackground Background }
    pack $itk_component(reset) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(controls).zin \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon zoomin] \
            -command [itcl::code $this _zoom in]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomin) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
        button $itk_component(controls).zout \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon zoomout] \
            -command [itcl::code $this _zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomout) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    #
    # Add label for the title.
    #
    itk_component add title {
        label $itk_interior.title
    }
    pack $itk_component(title) -side top

    #
    # Add graph showing levels
    #
    itk_component add graph {
        canvas $itk_interior.graph -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
    }
    pack $itk_component(graph) -expand yes -fill both

    bind $itk_component(graph) <Configure> \
        [list $_dispatcher event -idle !redraw]

    bind $itk_component(graph) <ButtonPress-1> \
        [itcl::code $this _zoom at %x %y]
    bind $itk_component(graph) <B1-Motion> \
        [itcl::code $this _zoom at %x %y]

    bind $itk_component(graph) <Motion> \
        [itcl::code $this _hilite brush %x %y]
    bind $itk_component(graph) <Leave> \
        [itcl::code $this _hilite hide]

    bind $itk_component(graph) <KeyPress-Up> \
        [itcl::code $this _zoom nudge 1]
    bind $itk_component(graph) <KeyPress-Right> \
        [itcl::code $this _zoom nudge 1]
    bind $itk_component(graph) <KeyPress-plus> \
        [itcl::code $this _zoom nudge 1]

    bind $itk_component(graph) <KeyPress-Down> \
        [itcl::code $this _zoom nudge -1]
    bind $itk_component(graph) <KeyPress-Left> \
        [itcl::code $this _zoom nudge -1]
    bind $itk_component(graph) <KeyPress-minus> \
        [itcl::code $this _zoom nudge -1]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: columns <table>
#
# Clients use this to scan a <table> XML object and see if it contains
# columns for energy levels.  If so, it returns a list of two column
# names: {labels energies}.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::columns {dataobj} {
    set names [$dataobj columns -component]
    set epos [lsearch -exact $names column(levels)]
    if {$epos >= 0} {
        set units [$dataobj columns -units $epos]
        if {![string match energy* [Rappture::Units::description $units]]} {
            set epos -1
        }
    }

    # can't find column named "levels"? then look for column with energies
    if {$epos < 0} {
        set index 0
        foreach units [$dataobj columns -units] {
            if {[string match energy* [Rappture::Units::description $units]]} {
                if {$epos >= 0} {
                    # more than one energy column -- bail out
                    set epos -1
                    break
                }
                set epos $index
            }
            incr index
        }
    }

    # look for a column with labels
    set lpos -1
    set index 0
    foreach units [$dataobj columns -units] {
        if {"" == $units} {
            set vals [$dataobj values -column $index]
            if {$lpos != $epos} {
                set lpos $index
                break
            }
        }
        incr index
    }

    if {$epos >= 0 || $lpos >= 0} {
        return [list [lindex $names $lpos] [lindex $names $epos]]
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::add {dataobj {settings ""}} {
    #
    # Make sure this table contains energy levels.
    #
    set cols [Rappture::EnergyLevels::columns $dataobj]
    if {"" == $cols} {
        error "table \"$dataobj\" does not contain energy levels"
    }

    #
    # Scan through the settings and resolve all values.
    #
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
    if {$params(-color) == "auto" || $params(-color) == "autoreset"} {
        # can't handle -autocolors yet
        set params(-color) blue
    }

    set pos [lsearch -exact $_dlist $dataobj]
    if {$pos < 0} {
        lappend _dlist $dataobj
        set _dobj2color($dataobj) $params(-color)
        set _dobj2raise($dataobj) $params(-raise)
        set _dobj2desc($dataobj) $params(-description)

        foreach {lcol ecol} $cols break
        set _dobj2cols($dataobj-label) $lcol
        set _dobj2cols($dataobj-energy) $ecol

        $_dispatcher event -idle !redraw
    }
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified data objs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            catch {unset _dobj2color($dataobj)}
            catch {unset _dobj2raise($dataobj)}
            catch {unset _dobj2desc($dataobj)}
            catch {unset _dobj2cols($dataobj-label)}
            catch {unset _dobj2cols($dataobj-energy)}
            set changed 1
        }
    }

    # if anything changed, then rebuild the plot
    if {$changed} {
        $_dispatcher event -idle !redraw
    }
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::get {} {
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
# USAGE: scale ?<dataobj1> <dataobj2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <dataobj> objects.  This
# accounts for all dataobjs--even those not showing on the screen.
# Because of this, the limits are appropriate for all data as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::scale {args} {
    set _emin ""
    set _emax ""
    foreach obj $args {
        if {![info exists _dobj2cols($obj-energy)]} {
            # don't recognize this object? then ignore it
            continue
        }
        foreach {min max} [$obj limits $_dobj2cols($obj-energy)] break

        if {"" != $min && "" != $max} {
            if {"" == $_emin} {
                set _emin $min
                set _emax $max
            } else {
                if {$min < $_emin} { set _emin $min }
                if {$max > $_emax} { set _emax $max }
            }
        }
    }

    if {"" != $_emin && $_emin == $_emax} {
        set _emin [expr {$_emin-0.1}]
        set _emax [expr {$_emax+0.1}]
    }

    set _eviewmin ""  ;# reset zoom view
    set _eviewmax ""
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
itcl::body Rappture::EnergyLevels::download {option args} {
    switch $option {
        coming {
            # nothing to do
        }
        controls {
            set popup .energyresultdownload
            if {![winfo exists .energyresultdownload]} {
                # if we haven't created the popup yet, do it now
                Rappture::Balloon $popup \
                    -title "[Rappture::filexfer::label downloadWord] as..."
                set inner [$popup component inner]
                label $inner.summary -text "" -anchor w
                pack $inner.summary -side top
                radiobutton $inner.csv -text "Data as Comma-Separated Values" \
                    -variable Rappture::EnergyLevels::_downloadPopup(format) \
                    -value csv
                pack $inner.csv -anchor w
                radiobutton $inner.pdf -text "Image as PDF/PostScript" \
                    -variable Rappture::EnergyLevels::_downloadPopup(format) \
                    -value pdf
                pack $inner.pdf -anchor w
                button $inner.go -text [Rappture::filexfer::label download] \
                    -command [lindex $args 0]
                pack $inner.go -pady 4
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
            set popup .energyresultdownload
            if {[winfo exists .energyresultdownload]} {
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
                    if {[info exists _dobj2desc($dataobj)]
                          && [llength [split $_dobj2desc($dataobj) \n]] > 1} {
                        set indent "for:"
                        foreach line [split $_dobj2desc($dataobj) \n] {
                            append csvdata " $indent $line\n"
                            set indent "    "
                        }
                    }
                    append csvdata "[string repeat - 60]\n"

                    set ecol $_dobj2cols($dataobj-energy)
                    set units [$dataobj columns -units $ecol]
                    foreach eval [$dataobj values -column $ecol] {
                        append csvdata [format "%20.15g $units\n" $eval]
                    }
                    append csvdata "\n"
                }
                return [list .txt $csvdata]
              }
              pdf {
                set psdata [$itk_component(graph) postscript]

                set cmds {
                    set fout "energy[pid].pdf"
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
# USAGE: _redraw
#
# Used internally to load a list of energy levels from a <table> within
# the data objects.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::_redraw {{what all}} {
    # scale data now, if we haven't already
    if {"" == $_emin || "" == $_emax} {
        eval scale $_dlist
    }

    set dlist [get]
    set topdobj [lindex $dlist end]
    _getLayout

    #
    # Redraw the overall layout
    #
    if {$what == "all"} {
        $c delete all
        if {[llength $dlist] == 0} {
            return
        }

        #
        # Scan through all data objects and plot them in order from
        # the bottom up.
        #
        set e2y [expr {($yzoom1-$yzoom0)/($_emax-$_emin)}]

        set title ""
        set dataobj ""
        foreach dataobj $dlist {
            if {"" == $title} {
                set title [$dataobj hints label]
            }

            set ecol $_dobj2cols($dataobj-energy)
            set color $_dobj2color($dataobj)
            if {"" == $color} {
                set color $itk_option(-levelcolor)
            }
            set color [Rappture::color::brightness $color 0.7]

            foreach eval [$dataobj values -column $ecol] {
                set y [expr {($eval-$_emin)*$e2y + $yzoom0}]
                $c create line $xx0 $y $xx1 $y -fill $color -width 1
            }
        }

        #
        # Scan through the data and look for HOMO/LUMO levels.
        # Set the default view to the energy just above and
        # just below the HOMO/LUMO levels.
        #
        set _edefmin [expr {0.4*($_emax-$_emin) + $_emin}]
        set _edefmax [expr {0.6*($_emax-$_emin) + $_emin}]

        set nlumo -1
        set nhomo -1

        set dataobj [lindex $dlist end]
        if {"" != $dataobj} {
            set lcol $_dobj2cols($dataobj-label)
            set ecol $_dobj2cols($dataobj-energy)
            set units [$dataobj columns -units $ecol]

            set n 0
            foreach eval [$dataobj values -column $ecol] \
                    lval [$dataobj values -column $lcol] {

                if {[string equal -nocase $lval "HOMO"]} {
                    set nhomo $n
                    set _lhomo $lval
                    set nlumo [expr {$n+1}]
                    set _llumo "LUMO"
                } elseif {[string equal -nocase $lval "Ground State"]} {
                    set nhomo $n
                    set _lhomo $lval
                    set nlumo [expr {$n+1}]
                    set _llumo "1st Excited State"
                } elseif {[string equal -nocase $lval "LUMO"]
                      || [string equal -nocase $lval "1st Excited State"]} {
                    set nlumo $n
                    set _llumo $lval
                }
                incr n
            }

            if {$nhomo >= 0 && $nlumo >= 0} {
                set elist [$dataobj values -column $ecol]
                set _ehomo [lindex $elist $nhomo]
                set _elumo [lindex $elist $nlumo]
                if {"" != $_elumo && "" != $_ehomo} {
                    set gap [expr {$_elumo - $_ehomo}]
                    set _edefmin [expr {$_ehomo - 0.3*$gap}]
                    set _edefmax [expr {$_elumo + 0.3*$gap}]

                    set y [expr {($_ehomo-$_emin)*$e2y + $yzoom0}]
                    set id [$c create rectangle $xx0 $y $xx1 $y0 \
                        -stipple [Rappture::icon rdiag] \
                        -outline "" -fill $itk_option(-shadecolor)]
                    $c lower $id
                }
            }
        }
        if {"" == $_eviewmin || "" == $_eviewmax} {
            set _eviewmin $_edefmin
            set _eviewmax $_edefmax
        }

        if {"" != $title} {
            pack $itk_component(title) -side top -before $c
            $itk_component(title) configure -text $title
        } else {
            pack forget $itk_component(title)
        }

        # draw the lines for the "zoom" view (fixed up below)
        set color $itk_option(-foreground)
        $c create line $x0 $yzoom0 $x1 $yzoom0 -fill $color -tags zmin
        $c create line $x0 $yzoom0 $x1 $yzoom0 -fill $color -tags zmax

        $c create line $x1 $yzoom0 $x2 $yzoom0 -fill $color -tags zoomup
        $c create line $x1 $yzoom0 $x2 $yzoom1 -fill $color -tags zoomdn

        $c create line $x2 $yzoom0 $x3 $yzoom0 -fill $color
        $c create line $x2 $yzoom1 $x3 $yzoom1 -fill $color
    }

    #
    # Redraw the "zoom" area on the right side
    #
    if {$what == "zoom" || $what == "all"} {
        set e2y [expr {($yzoom1-$yzoom0)/($_emax-$_emin)}]

        set y [expr {($_eviewmin-$_emin)*$e2y + $yzoom0}]
        $c coords zmin $x0 $y $x1 $y
        $c coords zoomup $x1 $y $x2 $yzoom0

        set y [expr {($_eviewmax-$_emin)*$e2y + $yzoom0}]
        $c coords zmax $x0 $y $x1 $y
        $c coords zoomdn $x1 $y $x2 $yzoom1

        # redraw all levels in the current view
        $c delete zlevels zlabels

        set e2y [expr {($yzoom1-$yzoom0)/($_eviewmax-$_eviewmin)}]
        foreach dataobj $dlist {
            set ecol $_dobj2cols($dataobj-energy)
            set color $_dobj2color($dataobj)
            if {"" == $color} {
                set color $itk_option(-levelcolor)
            }

            set n 0
            foreach eval [$dataobj values -column $ecol] {
                set y [expr {($eval-$_eviewmin)*$e2y + $yzoom0}]
                if {$y >= $y1 && $y <= $y0} {
                    set id [$c create line $xx2 $y $xx3 $y \
                        -fill $color -width 1 \
                        -tags [list zlevels $dataobj-$n]]
                }
                incr n
            }
        }

        if {"" != $topdobj && "" != $_ehomo && "" != $_elumo} {
            set ecol $_dobj2cols($topdobj-energy)
            set units [$topdobj columns -units $ecol]

            set yy0 [expr {($_ehomo-$_eviewmin)*$e2y + $yzoom0}]
            set yy1 [expr {($_elumo-$_eviewmin)*$e2y + $yzoom0}]

            set textht [font metrics $itk_option(-font) -linespace]
            if {$yy0-$yy1 >= 1.5*$textht} {
                $c create line [expr {$x3-10}] $yy0 [expr {$x3-10}] $yy1 \
                    -arrow both -fill $itk_option(-foreground) \
                    -tags zlabels
                $c create text [expr {$x3-15}] [expr {0.5*($yy0+$yy1)}] \
                    -anchor e -text "Eg = [expr {$_elumo-$_ehomo}] $units" \
                    -tags zlabels

                # label the HOMO level
                set tid [$c create text [expr {0.5*($x2+$x3)}] $yy0 -anchor c \
                    -text "$_lhomo = $_ehomo $units" \
                    -fill $itk_option(-leveltextforeground) \
                    -tags zlabels]

                foreach {xb0 yb0 xb1 yb1} [$c bbox $tid] break
                set tid2 [$c create rectangle \
                    [expr {$xb0-1}] [expr {$yb0-1}] \
                    [expr {$xb1+1}] [expr {$yb1+1}] \
                    -outline $itk_option(-leveltextforeground) \
                    -fill $itk_option(-leveltextbackground) \
                    -tags zlabels]
                $c lower $tid2 $tid

                # label the LUMO level
                set tid [$c create text [expr {0.5*($x2+$x3)}] $yy1 -anchor c \
                    -text "$_llumo = $_elumo $units" \
                    -fill $itk_option(-leveltextforeground) \
                    -tags zlabels]

                foreach {xb0 yb0 xb1 yb1} [$c bbox $tid] break
                set tid2 [$c create rectangle \
                    [expr {$xb0-1}] [expr {$yb0-1}] \
                    [expr {$xb1+1}] [expr {$yb1+1}] \
                    -outline $itk_option(-leveltextforeground) \
                    -fill $itk_option(-leveltextbackground) \
                    -tags zlabels]
                $c lower $tid2 $tid
            }

            if {$yy0 < $y0} {
                set id [$c create rectangle $xx2 $yy0 $xx3 $y0 \
                    -stipple [Rappture::icon rdiag] \
                    -outline "" -fill $itk_option(-shadecolor) \
                    -tags zlabels]
                $c lower $id
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _zoom in
# USAGE: _zoom out
# USAGE: _zoom reset
# USAGE: _zoom at <x> <y>
# USAGE: _zoom nudge <dir>
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::_zoom {option args} {
    switch -- $option {
        in {
            set midE [expr {0.5*($_eviewmax + $_eviewmin)}]
            set delE [expr {0.8*($_eviewmax - $_eviewmin)}]
            _view $midE $delE
        }
        out {
            set midE [expr {0.5*($_eviewmax + $_eviewmin)}]
            set delE [expr {1.25*($_eviewmax - $_eviewmin)}]
            _view $midE $delE
        }
        reset {
            set _eviewmin $_edefmin
            set _eviewmax $_edefmax
            $_dispatcher event -idle !zoom
        }
        at {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_zoom at x y\""
            }
            set x [lindex $args 0]
            set y [lindex $args 1]

            _getLayout
            set y2e [expr {($_emax-$_emin)/($yzoom1-$yzoom0)}]

            if {$x > $x1} {
                return
            }
            set midE [expr {($y-$yzoom0)*$y2e + $_emin}]
            set delE [expr {$_eviewmax - $_eviewmin}]
            _view $midE $delE
        }
        nudge {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"_zoom nudge dir\""
            }
            set dir [lindex $args 0]

            set midE [expr {0.5*($_eviewmax + $_eviewmin)}]
            set delE [expr {$_eviewmax - $_eviewmin}]
            set midE [expr {$midE + $dir*0.25*$delE}]
            _view $midE $delE
        }
    }
    focus $itk_component(graph)
}

# ----------------------------------------------------------------------
# USAGE: _view <midE> <delE>
#
# Called automatically when the user clicks/drags on the left side
# of the widget where energy levels are displayed.  Sets the zoom
# view so that it's centered on the <y> coordinate.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::_view {midE delE} {
    if {$delE > $_emax-$_emin} {
        set delE [expr {$_emax - $_emin}]
    }
    if {$midE - 0.5*$delE < $_emin} {
        set _eviewmin $_emin
        set _eviewmax [expr {$_eviewmin+$delE}]
    } elseif {$midE + 0.5*$delE > $_emax} {
        set _eviewmax $_emax
        set _eviewmin [expr {$_eviewmax-$delE}]
    } else {
        set _eviewmin [expr {$midE - 0.5*$delE}]
        set _eviewmax [expr {$midE + 0.5*$delE}]
    }
    $_dispatcher event -idle !zoom
}

# ----------------------------------------------------------------------
# USAGE: _hilite brush <x> <y>
# USAGE: _hilite show <dataobj> <level>
# USAGE: _hilite hide
#
# Used internally to highlight energy levels in the zoom view and
# show their associated energy.  The "brush" operation is called
# as the mouse moves in the zoom view, to see if the <x>,<y>
# coordinate is touching a level.  The show/hide operations are
# then used to show/hide level info.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::_hilite {option args} {
    switch -- $option {
        brush {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_hilite brush x y\""
            }
            set x [lindex $args 0]
            set y [lindex $args 1]

            _getLayout
            if {$x < $x2 || $x > $x3} {
                return   ;# pointer must be in "zoom" area
            }

            set c $itk_component(graph)
            set id [$c find withtag current]

            # touching a line? then find the level and show its info
            if {"" != $id} {
                set e2y [expr {($yzoom1-$yzoom0)/($_eviewmax-$_eviewmin)}]

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

                set found 0
                foreach dataobj $dlist {
                    set ecol $_dobj2cols($dataobj-energy)
                    set n 0
                    foreach eval [$dataobj values -column $ecol] {
                        set ylevel [expr {($eval-$_eviewmin)*$e2y + $yzoom0}]
                        if {$y >= $ylevel-3 && $y <= $ylevel+3} {
                            set found 1
                            break
                        }
                        incr n
                    }
                    if {$found} break
                }
                if {$found} {
                    _hilite show $dataobj $n
                } else {
                    _hilite hide
                }
            } else {
                _hilite hide
            }
        }
        show {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_hilite show dataobj level\""
            }
            set dataobj [lindex $args 0]
            set level [lindex $args 1]

            if {$_hilite == "$dataobj $level"} {
                return
            }
            _hilite hide

            set lcol $_dobj2cols($dataobj-label)
            set lval [lindex [$dataobj values -column $lcol] $level]
            set ecol $_dobj2cols($dataobj-energy)
            set eval [lindex [$dataobj values -column $ecol] $level]
            set units [$dataobj columns -units $ecol]

            if {$eval == $_ehomo || $eval == $_elumo} {
                $itk_component(graph) itemconfigure $dataobj-$level -width 2
                set _hilite "$dataobj $level"
                # don't pop up info for the HOMO/LUMO levels
                return
            }

            _getLayout
            set e2y [expr {($yzoom1-$yzoom0)/($_eviewmax-$_eviewmin)}]
            set y [expr {($eval-$_eviewmin)*$e2y + $yzoom0}]

            set tid [$c create text [expr {0.5*($x2+$x3)}] $y -anchor c \
                -text "$lval = $eval $units" \
                -fill $itk_option(-leveltextforeground) \
                -tags hilite]

            foreach {x0 y0 x1 y1} [$c bbox $tid] break
            set tid2 [$c create rectangle \
                [expr {$x0-1}] [expr {$y0-1}] \
                [expr {$x1+1}] [expr {$y1+1}] \
                -outline $itk_option(-leveltextforeground) \
                -fill $itk_option(-leveltextbackground) \
                -tags hilite]
            $c lower $tid2 $tid

            $c itemconfigure $dataobj-$level -width 2
            set _hilite "$dataobj $level"
        }
        hide {
            if {"" != $_hilite} {
                $itk_component(graph) delete hilite
                $itk_component(graph) itemconfigure zlevels -width 1
                set _hilite ""
            }
        }
        default {
            error "bad option \"$option\": should be brush, show, hide"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _getLayout
#
# Used internally to compute a series of variables used when redrawing
# the widget.  Creates the variables with the proper values in the
# calling context.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::_getLayout {} {
    upvar c c
    set c $itk_component(graph)

    upvar w w
    set w [winfo width $c]

    upvar h h
    set h [winfo height $c]

    #
    # Measure the size of a typical label and use that to size
    # the left/right portions.  If the label is too big, leave
    # at least a little room for the labels.
    #
    set size [font measure $itk_option(-font) "$_llumo = X.XXXXXXe-XX eV"]
    set size [expr {$size + 6*$itk_option(-padding)}]

    set textht [font metrics $itk_option(-font) -linespace]
    set ypad [expr {int(0.5*($textht + 6))}]

    if {$size > $w-20} {
        set size [expr {$w-20}]
    } elseif {$size < 0.66*$w} {
        set size [expr {0.66*$w}]
    }
    set xm [expr {$w - $size}]

    upvar x0 x0
    set x0 $itk_option(-padding)

    upvar x1 x1
    set x1 [expr {$xm - $itk_option(-padding)}]

    upvar x2 x2
    set x2 [expr {$xm + $itk_option(-padding)}]

    upvar x3 x3
    set x3 [expr {$w - $itk_option(-padding)}]


    upvar xx0 xx0
    set xx0 [expr {$x0 + $itk_option(-padding)}]

    upvar xx1 xx1
    set xx1 [expr {$x1 - $itk_option(-padding)}]

    upvar xx2 xx2
    set xx2 [expr {$x2 + $itk_option(-padding)}]

    upvar xx3 xx3
    set xx3 [expr {$x3 - $itk_option(-padding)}]


    upvar y0 y0
    set y0 [expr {$h - $itk_option(-padding)}]

    upvar yzoom0 yzoom0
    set yzoom0 [expr {$y0 - $ypad}]

    upvar y1 y1
    set y1 $itk_option(-padding)

    upvar yzoom1 yzoom1
    set yzoom1 [expr {$y1 + $ypad}]
}

# ----------------------------------------------------------------------
# OPTION: -levelColor
# ----------------------------------------------------------------------
itcl::configbody Rappture::EnergyLevels::levelcolor {
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# OPTION: -leveltextforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::EnergyLevels::leveltextforeground {
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# OPTION: -leveltextbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::EnergyLevels::leveltextbackground {
    $_dispatcher event -idle !redraw
}
