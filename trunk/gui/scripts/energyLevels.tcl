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
#  Copyright (c) 2004  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *EnergyLevels.width 4i widgetDefault
option add *EnergyLevels.height 4i widgetDefault

itcl::class Rappture::EnergyLevels {
    inherit itk::Widget

    constructor {args} { # defined below }
    destructor { # defined below }

    method load {libobj}
}
                                                                                
itk::usual EnergyLevels {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::constructor {args} {
    itk_option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add graph {
        blt::graph $itk_interior.graph \
            -highlightthickness 0 -plotpadx 0 -plotpady 0 \
            -width 3i -height 3i
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(graph) -expand yes -fill both
    $itk_component(graph) legend configure -hide yes

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::destructor {} {
}

# ----------------------------------------------------------------------
# USAGE: load <libObj>
#
# Clients use this to load a list of energy levels from the specified
# XML libObj, which should be a <table>.  One column in the table
# should have "labels" and the other "energies".
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::load {libobj} {
    set clist [$libobj children]
    set ilabel [lsearch $clist [$libobj element column(labels)]]
    if {$ilabel < 0} {
        error "can't find column(labels) in energy table data"
    }
    set ienergy [lsearch $clist [$libobj element column(energies)]]
    if {$ienergy < 0} {
        error "can't find column(energies) in energy table data"
    }

    set units [$libobj get column(energies).units]
    if {$units == ""} {
        set units "eV"
    }

    #
    # Update the graph to show the current set of levels.
    #
    set graph $itk_component(graph)
    eval $graph element delete [$graph element names]
    eval $graph marker delete [$graph marker names]

    set n 0
    set emax ""
    set emin ""
    set ehomo ""
    set elumo ""
    foreach line [split [$libobj get data] "\n"] {
        set lval [lindex $line $ilabel]
        set eval [lindex $line $ienergy]
        if {$lval == "" || $eval == ""} {
            continue
        }

        set eval [Rappture::Units::convert $eval \
            -context $units -to $units -units off]

        if {$lval == "HOMO"} {
            set ehomo $eval
        } elseif {$lval == "LUMO"} {
            set elumo $eval
        }

        set elem "elem[incr n]"
        $graph element create $elem \
            -xdata {0 0.8} -ydata [list $eval $eval] \
            -color blue -symbol "" -linewidth 2

        $graph marker create text -coords [list 0.8 $eval] \
            -text $lval -anchor w -background ""

        if {$emax == ""} {
            set emax $eval
            set emin $eval
        } else {
            if {$eval > $emax} {set emax $eval}
            if {$eval < $emin} {set emin $eval}
        }
    }
    $graph xaxis configure -min 0 -max 1 -showticks off -linewidth 0
    $graph yaxis configure -title "Energy ($units)"

    # bump the limits so they are big enough to show labels
    set fnt $itk_option(-font)
    set h [expr {0.5*([font metrics $fnt -linespace] + 5)}]
    set emin [expr {$emin-$h/150.0}]
    set emax [expr {$emax+$h/150.0}]
    $graph yaxis configure -min $emin -max $emax

    #
    # If we found HOMO/LUMO levels, then add the band gap at
    # that point.
    #
    if {$ehomo != "" && $elumo != ""} {
        $graph marker create line \
            -coords [list 0.2 $elumo 0.2 $ehomo]

        set egap [expr {$ehomo-$elumo}]
        set emid [expr {0.5*($ehomo+$elumo)}]
        $graph marker create text \
            -coords [list 0.21 $emid] -background "" \
            -text "Eg = [format %.2g $egap] $units" -anchor w
    }
}

package require Rappture
Rappture::EnergyLevels .e
pack .e -expand yes -fill both

set lib [Rappture::library {<?xml version="1.0"?>
<table>
<column id="labels">Name</column>
<column id="energies">Energy</column>
<data>
  E0    0.1
  E1    0.2
  LUMO  0.4
  HOMO  0.9
  E4    1.5
  E5    2.8
</data>
</table>}]

.e load $lib
