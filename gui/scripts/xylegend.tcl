# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: xylegend - X/Y plot legend.
#
#  This widget is a legend for an X/Y plot, meant to view line graphs produced
#  as output from the run of a Rappture tool.  Use the "add" and
#  "delete" methods to control the curves showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *Xylegend.font \
    -*-helvetica-medium-r-normal-*-8-* widgetDefault

option add *Xylegend.Button.font \
    -*-helvetica-medium-r-normal-*-9-* widgetDefault

itcl::class ::Rappture::XyLegend {
    inherit itk::Widget

    private variable _autocolors {
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
    private variable _lastColorIndex ""
    private variable _dispatcher "" ;# dispatcher for !events
    private variable _graph     ""
    private variable _tree      ""
    private variable _diff      "";     # Polygon marker used for difference.
    private variable _rename    "";     # Node selected to be renamed.
    private variable _diffelements
    private variable _unmapHidden 0

    constructor {graph args} {}
    destructor {}

    public method reset { list }

    private method Add { elem label {flags ""}}
    private method Average {}
    private method BuildPopup { popup }
    private method Check {}
    private method Delete { args }
    private method Difference {}
    private method Editor { option args }
    private method GetData { elem what }
    private method Hide { args }
    private method Lower { args }
    private method Raise { args }
    private method Recolor {}
    private method PopupMenu { x y }
    private method Rename {}
    private method SelectAll {}
    private method Show { args }
    private method Toggle { args }
    private method UnmapHidden {}
}

itk::usual XyLegend {
    keep -background -foreground -cursor
}

itk::usual TreeView {
    keep -background -foreground -cursor
}

blt::bitmap define dot1 {
#define dot1_width 8
#define dot1_height 8
static unsigned char dot1_bits[] = {
   0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyLegend::constructor { graph args } {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no
    itk_component add scrollbars {
        Rappture::Scroller $itk_interior.scrl \
            -xscrollmode auto -yscrollmode auto \
            -width 200 -height 100
    }
    set _tree [blt::tree create]
    itk_component add legend {
        blt::treeview $itk_component(scrollbars).legend -linewidth 0 \
            -bg white -selectmode multiple \
            -highlightthickness 0 \
            -tree $_tree \
            -font "Arial 9" \
            -flat yes -separator /
    }
    $itk_component(scrollbars) contents $itk_component(legend)
    $itk_component(legend) column insert 0 "show" \
        -text "" -weight 0.0 -pad 0 -borderwidth 0
    $itk_component(legend) style checkbox "check" -showvalue no \
        -onvalue 0 -offvalue 1 \
        -boxcolor grey50 -checkcolor black -activebackground grey90
    $itk_component(legend) column configure "treeView" -justify left \
        -weight 1.0 -text "" -pad 0 -borderwidth 0 -edit no
    $itk_component(legend) column configure "show" -style "check" -pad {0 0} \
        -edit yes
    itk_component add controls {
        frame $itk_component(hull).controls -width 100 -relief sunken -bd 2
    }
    set controls $itk_component(controls)
    grid $itk_component(controls) -column 0 -row 1 -sticky nsew
    grid columnconfigure $itk_component(hull) 0 -weight 1
    grid rowconfigure $itk_component(hull) 1 \
        -minsize [winfo reqheight $itk_component(scrollbars)]
    grid rowconfigure $itk_component(hull) 0 -weight 1
    grid $itk_component(scrollbars) -column 0 -row 0 -sticky nsew
    set commands {
        hide    ""
        show    ""
        toggle  ""
        raise        findup
        lower   finddn
        average ""
        difference ""
        delete ""
        rename ""
        recolor ""
    }
    foreach { but icon} $commands {
        set title [string totitle $but]
        button $controls.$but -text $title \
            -relief flat -pady 0 -padx 0  -font "Arial 9" \
            -command [itcl::code $this $title]  -overrelief flat \
            -activebackground grey90
    }
    grid $controls.hide       -column 0 -row 0 -sticky w
    grid $controls.show       -column 0 -row 1 -sticky w
    grid $controls.toggle     -column 0 -row 2 -sticky w
    grid $controls.raise      -column 0 -row 3 -sticky w
    grid $controls.lower      -column 0 -row 4 -sticky w
    grid $controls.difference -column 1 -row 0 -sticky w
    grid $controls.average    -column 1 -row 1 -sticky w
    grid $controls.rename     -column 1 -row 2 -sticky w
    grid $controls.recolor    -column 1 -row 3 -sticky w
    grid $controls.delete     -column 1 -row 4 -sticky w

    grid columnconfigure $controls 0  -weight 1
    grid columnconfigure $controls 1 -weight 1

    set _graph $graph
    set cmd [itcl::code $this Toggle current]
    $itk_component(legend) bind CheckBoxStyle <ButtonRelease-1> \
        [itcl::code [subst -nocommands {
            if { [%W edit -root -test %X %Y] } {
                %W edit -root %X %Y
                $this Toggle [%W nearest -root %X %Y]
                break
            }
        }]]
    bind $itk_component(legend) <Enter> { focus %W }
    $itk_component(legend) bind Entry <Control-KeyRelease-a> \
        [itcl::code $this SelectAll]
    $itk_component(legend) bind Entry <KeyRelease-Return> \
        +[itcl::code $this Toggle focus]
    $itk_component(legend) bind Entry <Escape> \
        "$itk_component(legend) selection clearall"
    $itk_component(legend) configure -selectcommand \
        [itcl::code $this Check]

    itk_component add editor {
        Rappture::Editor $itk_interior.editor \
            -activatecommand [itcl::code $this Editor activate] \
            -validatecommand [itcl::code $this Editor validate] \
            -applycommand [itcl::code $this Editor apply]
    }
    set _lastColorIndex [llength $_autocolors]
    Check
    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyLegend::destructor {} {
    foreach node [$_tree children root] {
        $_tree delete $node
    }
    if { $_diff != "" } {
        catch { $_graph marker delete $_diff }
    }
}

itcl::body Rappture::XyLegend::Add { elem label {flags ""} } {
    set hide [$_graph element cget $elem -hide]
    set im [image create photo]
    $_graph legend icon $elem $im
    set data(show) $hide
    set data(delete) [expr { $flags == "-delete" }]
    set node [$_tree insert root -at 0 -label $elem -data [array get data]]
    $itk_component(legend) entry configure $node -label $label -icon $im \
        -activeicon $im
    update idletasks
    return $node
}

# ----------------------------------------------------------------------
# USAGE: reset
#
# Clients use this to add a curve to the plot.  The optional <settings>
# are used to configure the plot.  Allowed settings are -color,
# -brightness, -width, -linestyle and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::XyLegend::reset { list } {
    foreach node [$_tree children root] {
        $_tree delete $node
    }
    foreach elem $list {
        if {[catch {$_graph element cget $elem -label} label] == 0} {
            if {$label eq ""} {
                set label $elem
            }
            Add $elem $label
        }
    }
    $itk_component(legend) open -recurse root
    Check
}

itcl::body Rappture::XyLegend::Hide { args } {
    if { $args == "" } {
        set nodes [$itk_component(legend) curselection]
    } else {
        set nodes $args
    }
    foreach node $nodes {
        set elem [$_tree label $node]
        if { ![$_graph element exists $elem] } {
            continue
        }
        $_graph element configure $elem -hide yes
        $_tree set $node "show" 1
    }
}

itcl::body Rappture::XyLegend::Show { args } {
    if { $args == "" } {
        set nodes [$itk_component(legend) curselection]
    } else {
        set nodes $args
    }
    foreach node $nodes {
        set elem [$_tree label $node]
        if { ![$_graph element exists $elem] } {
            continue
        }
        $_graph element configure $elem -hide no
        $_tree set $node "show" 0
    }
}

itcl::body Rappture::XyLegend::Toggle { args } {
    if { $args == "" } {
        set nodes [$itk_component(legend) curselection]
    } else {
        set nodes $args
    }
    foreach node $nodes {
        set elem [$_tree label $node]
        if { ![$_graph element exists $elem] } {
            continue
        }
        set hide [$_graph element cget $elem -hide]
        set hide [expr $hide==0]
        $_tree set $node "show" $hide
        $_graph element configure $elem -hide $hide
    }
}

itcl::body Rappture::XyLegend::Raise { args } {
    if { $args == "" } {
        set nodes [$itk_component(legend) curselection]
    } else {
        set nodes $args
    }
    set elements {}
    foreach node $nodes {
        set elem [$_tree label $node]
        set found($elem) 1
        set elements [linsert $elements 0 $elem]
    }
    foreach elem $elements {
        $_tree move [$_tree index $elem] 0 -at 0
    }
    set list {}
    foreach elem [$_graph element show] {
        if { [info exists found($elem)] }  {
            continue
        }
        lappend list $elem
    }
    $_graph element show [concat $list $elements]
}

itcl::body Rappture::XyLegend::Lower { args } {
    if { $args == "" } {
        set nodes [$itk_component(legend) curselection]
    } else {
        set nodes $args
    }
    set elements {}
    foreach node $nodes {
        set elem [$_tree label $node]
        set found($elem) 1
        set elements [linsert $elements 0 $elem]
    }
    set pos [$_tree degree 0]

    foreach elem $elements {
        incr pos -1
        $_tree move [$_tree index $elem] 0 -at $pos
    }

    set list {}
    foreach elem [$_graph element show] {
        if { [info exists found($elem)] }  {
            continue
        }
        lappend list $elem
    }
    $_graph element show [concat $elements $list]
}

itcl::body Rappture::XyLegend::Delete { args } {
    if { $args == "" } {
        set nodes [$itk_component(legend) curselection]
    } else {
        set nodes $args
    }
    set elements {}
    set delnodes {}
    foreach node $nodes {
        if { ![$_tree get $node "delete" 0] } {
            continue
        }
        set elem [$_tree label $node]
        lappend elements $elem
        lappend delnodes $node
        if { $_diff != "" && [info exists _diffelements($elem)] } {
            $_graph marker delete $_diff
            array unset _diffelements
            set _diff ""
        }
    }
    if { [llength $delnodes] > 0 } {
        eval $_tree delete $delnodes
    }
    $itk_component(legend) selection clearall
    eval $_graph element delete $elements
}

itcl::body Rappture::XyLegend::Check {} {
    set nodes [$itk_component(legend) curselection]
    foreach n { hide show toggle raise lower
        rename average difference delete recolor } {
        $itk_component(controls).$n configure -state disabled
    }
    foreach node $nodes {
        if { [$_tree get $node "delete" 0] } {
            $itk_component(controls).delete configure -state normal
            break
        }
    }
    if { [$_tree degree 0] > 1  && [llength $nodes] > 0 } {
        foreach n { raise lower } {
            $itk_component(controls).$n configure -state normal
        }
    }
    switch -- [llength $nodes] {
        0 {
        }
        1 {
            foreach n { hide show toggle rename recolor } {
                $itk_component(controls).$n configure -state normal
            }
        }
        2 {
            foreach n { hide show toggle difference average recolor } {
                $itk_component(controls).$n configure -state normal
            }
        }
        default {
            foreach n { hide show toggle average recolor } {
                $itk_component(controls).$n configure -state normal
            }
        }
    }
}

itcl::body Rappture::XyLegend::GetData { elem what } {
    set y [$_graph element cget $elem $what]
    if { [blt::vector names $y] == $y } {
        set y [$y range 0 end]
    }
    return $y
}

itcl::body Rappture::XyLegend::Average {} {
    set nodes [$itk_component(legend) curselection]
    if { $nodes == "" } {
        return
    }
    set elements {}
    set sum [blt::vector create \#auto -command ""]

    set xcoords [blt::vector create \#auto -command ""]
    set ycoords [blt::vector create \#auto -command ""]

    blt::busy hold $itk_component(hull)
    update
    # Step 1. Get the x-values for each curve, then sort them to get the
    #         unique values.

    set labels {}
    foreach node $nodes {
        set elem [$_tree label $node]
        set label [$_graph element cget $elem -label]
        $xcoords append [GetData $elem -x]
        set elements [linsert $elements 0 $elem]
        set labels [linsert $labels 0 $label]
    }
    # Sort the abscissas keeping unique values.
    $xcoords sort -uniq

    # Step 2. Now for each curve, generate a cubic spline of that curve
    #         and interpolate to get the corresponding y-values for each
    #         abscissa.  Normally the abscissa are the same, so we're
    #         interpolation the knots.

    set x [blt::vector create \#auto -command ""]
    set y [blt::vector create \#auto -command ""]
    $sum length [$xcoords length]

    foreach node $nodes {
        set elem [$_tree label $node]
        $x set [GetData $elem -x]
        $y set [GetData $elem -y]
        blt::spline natural $x $y $xcoords $ycoords

        # Sum the interpolated y-coordinate values.
        $sum expr "$sum + $ycoords"
    }
    blt::vector destroy $x $y

    # Get the average
    $sum expr "$sum / [llength $elements]"

    # Step 3.  Create a new curve which is the average. Append it to the
    #          the end.

    set count 0
    while {[$_graph element exists avg$count] } {
        incr count
    }
    set labels [lsort -dictionary $labels]
    set name "avg$count"
    set label "Avg. [join $labels ,]"

    # Don't use the vector because we don't know when it will be cleaned up.

    if { $_lastColorIndex == 0 } {
        set _lastColorIndex [llength $_autocolors]
    }
    incr _lastColorIndex -1
    set color [lindex $_autocolors $_lastColorIndex]
    $_graph element create $name -label $label -x [$xcoords range 0 end]\
        -y [$sum range 0 end] -symbol scross -pixels 3 -color $color
    blt::vector destroy $xcoords $ycoords $sum
    set node [Add $name $label -delete]
    Raise $node
    blt::busy forget $itk_component(hull)
}

itcl::body Rappture::XyLegend::Difference {} {

    if { $_diff != "" } {
        $_graph marker delete $_diff
        set _diff ""
    }
    set nodes [$itk_component(legend) curselection]
    set elem1 [$_tree label [lindex $nodes 0]]
    set elem2 [$_tree label [lindex $nodes 1]]
    if { [info exists _diffelements($elem1)] &&
         [info exists _diffelements($elem2)] } {
        array unset _diffelements;      # Toggle the difference.
        return;
    }
    array unset _diffelements
    set x [blt::vector create \#auto -command ""]
    set y [blt::vector create \#auto -command ""]
    set m [blt::vector create \#auto -command ""]

    $x append [GetData $elem1 -x]
    $y append [GetData $elem1 -y]
    $x sort -reverse $y
    $x append [GetData $elem2 -x]
    $y append [GetData $elem2 -y]
    $m merge $x $y
    set _diff [$_graph marker create polygon \
                   -coords [$m range 0 end] \
                   -element $elem1 \
                   -stipple dot1 \
                   -outline "" -fill "#cd69c9"]
    blt::vector destroy $m $x $y
    set _diffelements($elem1) 1
    set _diffelements($elem2) 1
}


itcl::body Rappture::XyLegend::UnmapHidden {} {
    $_graph configure -unmaphiddenelements $_unmapHidden
}

itcl::body Rappture::XyLegend::SelectAll { } {
    foreach node [$_tree children 0] {
        $itk_component(legend) selection set $node
    }
}

itcl::body Rappture::XyLegend::Rename {} {
    Editor popup
}

# ----------------------------------------------------------------------
# USAGE: Editor popup
# USAGE: Editor activate
# USAGE: Editor validate <value>
# USAGE: Editor apply <value>
# USAGE: Editor menu <rootx> <rooty>
#
# Used internally to handle the various functions of the pop-up
# editor for the value of this gauge.
# ----------------------------------------------------------------------
itcl::body Rappture::XyLegend::Editor {option args} {
    switch -- $option {
        popup {
            $itk_component(editor) activate
        }
        activate {
            set _rename [$itk_component(legend) curselection]
            if { $_rename == "" } {
                return;
            }
            set label [$itk_component(legend) entry cget $_rename -label]
            foreach { l r w h } [$itk_component(legend) bbox $_rename] break
            set info(text) $label
            set info(x) [expr $l + [winfo rootx $itk_component(legend)]]
            set info(y) [expr $r + [winfo rooty $itk_component(legend)]]
            set info(w) $w
            set info(h) $h
            return [array get info]
        }
        validate {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"editor validate value\""
            }
        }
        apply {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"editor apply value\""
            }
            set label [lindex $args 0]
            $itk_component(legend) entry configure $_rename -label $label
            set elem [$_tree label $_rename]
            $_graph element configure $elem -label $label
        }
        menu {
            eval tk_popup $itk_component(emenu) $args
        }
        default {
            error "bad option \"$option\": should be popup, activate, validate, apply, and menu"
        }
    }
}

itcl::body Rappture::XyLegend::Recolor {} {
    set nodes [$itk_component(legend) curselection]
    if { $nodes == "" } {
        return
    }
    foreach node $nodes {
        set elem [$_tree label $node]
        if { $_lastColorIndex == 0 } {
            set _lastColorIndex [llength $_autocolors]
        }
        incr _lastColorIndex -1
        set color [lindex $_autocolors $_lastColorIndex]
        $_graph element configure $elem -color $color
        set im [$itk_component(legend) entry cget $node -icon]
        $_graph legend icon $elem $im
    }
}
