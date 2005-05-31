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
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *EnergyLevels.width 4i widgetDefault
option add *EnergyLevels.height 4i widgetDefault
option add *EnergyLevels.levelColor blue widgetDefault
option add *EnergyLevels.levelTextForeground blue widgetDefault
option add *EnergyLevels.levelTextBackground #d9d9d9 widgetDefault

option add *EnergyLevels.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

option add *EnergyLevels.detailFont \
    -*-helvetica-medium-r-normal-*-*-100-* widgetDefault

itcl::class Rappture::EnergyLevels {
    inherit itk::Widget

    itk_option define -layout layout Layout ""
    itk_option define -output output Output ""
    itk_option define -levelcolor levelColor LevelColor ""
    itk_option define -leveltextforeground levelTextForeground Foreground ""
    itk_option define -leveltextbackground levelTextBackground Background ""

    constructor {args} { # defined below }
    destructor { # defined below }

    protected method _render {}
    protected method _adjust {what val}
    protected method _getColumn {name}
    protected method _getUnits {name}
    protected method _getMidPt {elist pos}
}

itk::usual EnergyLevels {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::constructor {args} {
    itk_option add hull.width hull.height
    pack propagate $itk_component(hull) no

    #
    # Add label for the title.
    #
    itk_component add title {
        label $itk_interior.title
    }
    pack $itk_component(title) -side top


    itk_component add cntls {
        frame $itk_interior.cntls
    }
    pack $itk_component(cntls) -side right -fill y
    grid rowconfigure $itk_component(cntls) 0 -weight 1
    grid rowconfigure $itk_component(cntls) 1 -minsize 10
    grid rowconfigure $itk_component(cntls) 2 -weight 1

    #
    # Add MORE/FEWER levels control for TOP of graph
    #
    itk_component add upperE {
        frame $itk_component(cntls).upperE
    }

    itk_component add upperEmore {
        label $itk_component(upperE).morel -text "More"
    } {
        usual
        rename -font -detailfont detailFont Font
    }
    pack $itk_component(upperEmore) -side top

    itk_component add upperEfewer {
        label $itk_component(upperE).fewerl -text "Fewer"
    } {
        usual
        rename -font -detailfont detailFont Font
    }
    pack $itk_component(upperEfewer) -side bottom

    itk_component add upperEcntl {
        scale $itk_component(upperE).cntl -orient vertical -showvalue 0 \
            -command [itcl::code $this _adjust upper]
    }
    pack $itk_component(upperEcntl) -side top -fill y

    #
    # Add MORE/FEWER levels control for BOTTOM of graph
    #
    itk_component add lowerE {
        frame $itk_component(cntls).lowerE
    }

    itk_component add lowerEmore {
        label $itk_component(lowerE).morel -text "More"
    } {
        usual
        rename -font -detailfont detailFont Font
    }
    pack $itk_component(lowerEmore) -side bottom

    itk_component add lowerEfewer {
        label $itk_component(lowerE).fewerl -text "Fewer"
    } {
        usual
        rename -font -detailfont detailFont Font
    }
    pack $itk_component(lowerEfewer) -side top

    itk_component add lowerEcntl {
        scale $itk_component(lowerE).cntl -orient vertical -showvalue 0 \
            -command [itcl::code $this _adjust lower]
    }
    pack $itk_component(lowerEcntl) -side top -fill y

    #
    # Add graph showing levels
    #
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
# USAGE: _render
#
# Used internally to load a list of energy levels from a <table> within
# the -output XML object.  The -layout object indicates how information
# should be extracted from the table.  The <layout> should have an
# <energies> tag and perhaps a <labels> tag, which indicates the table
# and the column within the table containing the energies.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::_render {} {
    #
    # Clear any information shown in the graph.
    #
    set graph $itk_component(graph)
    eval $graph element delete [$graph element names]
    eval $graph marker delete [$graph marker names]

    #
    # Plug in the title from the layout
    #
    set title ""
    if {$itk_option(-layout) != ""} {
        set title [$itk_option(-layout) get label]
    }
    if {"" != $title} {
        pack $itk_component(title) -side top -before $graph
        $itk_component(title) configure -text $title
    } else {
        pack forget $itk_component(title)
    }

    #
    # Look through the layout and figure out what to extract
    # from the table.
    #
    set elist [_getColumn energies]
    if {[llength $elist] == 0} {
        return
    }
    set units [_getUnits energies]

    set llist [_getColumn names]
    if {[llength $llist] == 0} {
        # no labels? then invent some
        set i 0
        foreach name $elist {
            lappend llist "E$i"
            incr i
        }
    }

    #
    # Update the graph to show the current set of levels.
    #
    set n 0
    set nlumo -1
    set emax ""
    set emin ""
    set ehomo ""
    set elumo ""
    foreach eval $elist lval $llist {
        if {$lval == "HOMO"} {
            set ehomo $eval
            set lval "HOMO = $eval $units"
            set nlumo [expr {$n+1}]
        } elseif {$lval == "LUMO" || $n == $nlumo} {
            set elumo $eval
            set lval "LUMO = $eval $units"
        } else {
            set lval ""
        }

        set elem "elem[incr n]"
        $graph element create $elem \
            -xdata {0 1} -ydata [list $eval $eval] \
            -color $itk_option(-levelcolor) -symbol "" -linewidth 1

        if {$lval != ""} {
            $graph marker create text -coords [list 0.5 $eval] \
                -text $lval -anchor c \
                -foreground $itk_option(-leveltextforeground) \
                -background $itk_option(-leveltextbackground)
        }

        if {$emax == ""} {
            set emax $eval
            set emin $eval
        } else {
            if {$eval > $emax} {set emax $eval}
            if {$eval < $emin} {set emin $eval}
        }
    }
    $graph xaxis configure -min 0 -max 1 -showticks off -linewidth 0
    if {$units != ""} {
        $graph yaxis configure -title "Energy ($units)"
    } else {
        $graph yaxis configure -title "Energy"
    }

    # bump the limits so they are big enough to show labels
    set fnt $itk_option(-font)
    set h [expr {0.5*([font metrics $fnt -linespace] + 5)}]
    set emin [expr {$emin-($emax-$emin)*$h/150.0}]
    set emax [expr {$emax+($emax-$emin)*$h/150.0}]
    $graph yaxis configure -min $emin -max $emax

    #
    # If we found HOMO/LUMO levels, then add the band gap at
    # that point.  Also, fix the controls for energy range.
    #
    if {$ehomo != "" && $elumo != ""} {
        set id [$graph marker create line \
            -coords [list 0.2 $elumo 0.2 $ehomo]]
        $graph marker after $id

        set egap [expr {$elumo-$ehomo}]
        set emid [expr {0.5*($ehomo+$elumo)}]
        $graph marker create text \
            -coords [list 0.21 $emid] -background "" \
            -text "Eg = [format %.2g $egap] $units" -anchor w

        # fix the limits for the lower scale
        set elim [_getMidPt $elist [expr {$nlumo-1}]]
        if {"" != $elim} {
            $itk_component(lowerEcntl) configure -from $elim -to $emin \
                -resolution [expr {0.02*($elim-$emin)}]
            grid $itk_component(lowerE) -row 2 -column 0 -sticky ns

            set e0 [_getMidPt $elist [expr {$nlumo-3}]]
            if {"" != $e0} {
                $itk_component(lowerEcntl) set $e0
            } else {
                $itk_component(lowerEcntl) set $elim
            }
        } else {
            grid forget $itk_component(lowerE)
        }

        # fix the limits for the upper scale
        set elim [_getMidPt $elist [expr {$nlumo+1}]]
        if {"" != $elim} {
            $itk_component(upperEcntl) configure -from $emax -to $elim \
                -resolution [expr {0.02*($emax-$elim)}]
            grid $itk_component(upperE) -row 0 -column 0 -sticky ns

            set e0 [_getMidPt $elist [expr {$nlumo+3}]]
            if {"" != $e0} {
                $itk_component(upperEcntl) set $e0
            } else {
                $itk_component(upperEcntl) set $elim
            }
        } else {
            grid forget $itk_component(upperE)
        }
    } else {
        grid forget $itk_component(upperE)
        grid forget $itk_component(lowerE)
    }
}

# ----------------------------------------------------------------------
# USAGE: _adjust <what> <val>
#
# Used internally to adjust the upper/lower limits of the graph
# as the user drags the slider from "More" to "Fewer".  Sets
# the specified limit to the given value.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::_adjust {what val} {
    switch -- $what {
        upper {
            $itk_component(graph) yaxis configure -max $val
        }
        lower {
            $itk_component(graph) yaxis configure -min $val
        }
        default {
            error "bad limit \"$what\": should be upper or lower"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _getColumn <name>
#
# Used internally to load a list of energy levels from a <table> within
# the -output XML object.  The -layout object indicates how information
# should be extracted from the table.  The <layout> should have an
# <energies> tag and perhaps a <labels> tag, which indicates the table
# and the column within the table containing the energies.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::_getColumn {name} {
puts "_getColumn $name"
    if {$itk_option(-output) == ""} {
        return
    }

    #
    # Figure out which column in which table contains the data.
    # Then, find that table and extract the column.  Figure out
    # the position of the column from the list of all column names.
    #
    if {$itk_option(-layout) != ""} {
        set table [$itk_option(-layout) get $name.table]
        set col [$itk_option(-layout) get $name.column]

        set clist ""
        foreach c [$itk_option(-output) children -type column $table] {
            lappend clist [$itk_option(-output) get $table.$c.label]
        }
        set ipos [lsearch $clist $col]
        if {$ipos < 0} {
            return  ;# can't find data -- bail out!
        }
        set units [$itk_option(-output) get $table.column$ipos.units]
        set path "$table.data"
    } else {
        set clist ""
        foreach c [$itk_option(-output) children -type column] {
            lappend clist [$itk_option(-output) get $c.units]
        }
        if {$name == "energies"} {
            set units "eV"
        } else {
            set units ""
        }
        set ipos [lsearch -exact $clist $units]
        if {$ipos < 0} {
            return  ;# can't find data -- bail out!
        }
        set path "data"
    }

    set rlist ""
    foreach line [split [$itk_option(-output) get $path] "\n"] {
        if {"" != [string trim $line]} {
            set val [lindex $line $ipos]

            if {$units != ""} {
                set val [Rappture::Units::convert $val \
                    -context $units -to $units -units off]
            }
            lappend rlist $val
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: _getUnits <name>
#
# Used internally to extract the units from a <table> within the
# -output XML object.  The -layout object indicates how information
# should be extracted from the table.  The <layout> should have an
# <energies> tag and perhaps a <labels> tag, which indicates the table
# and the column within the table containing the units.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::_getUnits {name} {
    if {$itk_option(-output) == ""} {
        return
    }

    #
    # Figure out which column in which table contains the data.
    # Then, find that table and extract the column.  Figure out
    # the position of the column from the list of all column names.
    #
    if {$itk_option(-layout) != ""} {
        set table [$itk_option(-layout) get $name.table]
        set col [$itk_option(-layout) get $name.column]

        set clist ""
        foreach c [$itk_option(-output) children -type column $table] {
            lappend clist [$itk_option(-output) get $table.$c.label]
        }
        set ipos [lsearch $clist $col]
        if {$ipos < 0} {
            return  ;# can't find data -- bail out!
        }
        set units [$itk_option(-output) get $table.column$ipos.units]
    } else {
        if {$name == "energies"} {
            set units "eV"
        } else {
            set units ""
        }
    }
    return $units
}

# ----------------------------------------------------------------------
# USAGE: _getMidPt <elist> <pos>
#
# Used internally to compute the midpoint between two energy levels
# at <pos> and <pos-1> in the <elist>.  Returns a number representing
# the mid-point (average value) or "" if the levels involved do
# no exist in <elist>.
# ----------------------------------------------------------------------
itcl::body Rappture::EnergyLevels::_getMidPt {elist pos} {
    if {$pos < [llength $elist] && $pos > 1} {
        set e1 [lindex $elist $pos]
        set e0 [lindex $elist [expr {$pos-1}]]
        return [expr {0.5*($e0+$e1)}]
    }
    return ""
}

# ----------------------------------------------------------------------
# OPTION: -layout
# ----------------------------------------------------------------------
itcl::configbody Rappture::EnergyLevels::layout {
    if {$itk_option(-layout) != ""
          && ![Rappture::library isvalid $itk_option(-layout)]} {
        error "bad value \"$itk_option(-layout)\": should be Rappture::library object"
    }
    after idle [itcl::code $this _render]
}

# ----------------------------------------------------------------------
# OPTION: -output
# ----------------------------------------------------------------------
itcl::configbody Rappture::EnergyLevels::output {
    if {$itk_option(-output) != ""
          && ![Rappture::library isvalid $itk_option(-output)]} {
        error "bad value \"$itk_option(-output)\": should be Rappture::library object"
    }
    after cancel [itcl::code $this _render]
    after idle [itcl::code $this _render]
}

# ----------------------------------------------------------------------
# OPTION: -levelColor
# ----------------------------------------------------------------------
itcl::configbody Rappture::EnergyLevels::levelcolor {
    after cancel [itcl::code $this _render]
    after idle [itcl::code $this _render]
}

# ----------------------------------------------------------------------
# OPTION: -leveltextforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::EnergyLevels::leveltextforeground {
    after cancel [itcl::code $this _render]
    after idle [itcl::code $this _render]
}

# ----------------------------------------------------------------------
# OPTION: -leveltextbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::EnergyLevels::leveltextbackground {
    after cancel [itcl::code $this _render]
    after idle [itcl::code $this _render]
}
