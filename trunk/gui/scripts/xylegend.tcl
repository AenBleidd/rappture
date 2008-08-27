
# ----------------------------------------------------------------------
#  COMPONENT: xylegend - X/Y plot legend.
#
#  This widget is a legend for an X/Y plot, meant to view line graphs produced
#  as output from the run of a Rappture tool.  Use the "add" and
#  "delete" methods to control the curves showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
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

    private variable _dispatcher "" ;# dispatcher for !events
    private variable graph_	""
    private variable tree_	""
    private variable diff_	""
    private variable focus_	-1
    private variable diffelements_

    constructor {args} { graph }
    destructor {}

    public method Activate {} 
    public method Average {} 
    public method Check {}
    public method Deactivate {} 
    public method Delete { args } 
    public method Difference {} 
    public method Editor { option args }
    public method Hide { args } 
    public method Lower { args } 
    public method Raise { args } 
    public method Rename {} 
    public method Show { args } 
    public method Toggle { args } 
    private method GetData { elem what } 
    private method Add { elem label {flags ""}} 
    private method SelectAll {}
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
    itk_component add scrollbars {
	Rappture::Scroller $itk_interior.scrl \
	    -xscrollmode auto -yscrollmode auto 
    }
    set tree_ [blt::tree create]
    itk_component add legend {
        blt::treeview $itk_component(scrollbars).legend -linewidth 0 \
	    -bg white -selectmode multiple -width 0 \
	    -highlightthickness 0 \
	    -tree $tree_ \
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
	-weight 1.0 -text "" -pad 0 -borderwidth 0 
    $itk_component(legend) column configure "show" -style "check" -pad {0 0} \
	-edit yes
    itk_component add controls {
        frame $itk_component(hull).controls -width 100 -relief sunken -bd 2
    }
    set controls $itk_component(controls)
    grid $itk_component(controls) -column 0 -row 1 -sticky nsew
    grid rowconfigure $itk_component(hull) 1 -weight 0
    grid rowconfigure $itk_component(hull) 0 -weight 1
    grid $itk_component(scrollbars) -column 0 -row 0 -sticky nsew
    set commands {
	hide    ""
	show    ""
	toggle  ""
	raise	findup
	lower   finddn
	average ""
	difference ""
	delete ""
	rename ""
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
    grid $controls.delete     -column 1 -row 3 -sticky w

    grid columnconfigure $controls 0  -weight 1
    grid columnconfigure $controls 1 -weight 1

    set graph_ $graph
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
    Check
    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyLegend::destructor {} {
    Deactivate 
}

itcl::body Rappture::XyLegend::Add { elem label {flags ""} } {
    set hide [$graph_ element cget $elem -hide]
    set im [image create photo]
    $graph_ legend icon $elem $im
    set data(show) $hide
    set data(delete) [expr { $flags == "-delete" }]
    set node [$tree_ insert root -at 0 -label $elem -data [array get data]]
    $itk_component(legend) entry configure $node -label $label -icon $im \
	-activeicon $im
    return $node
}

# ----------------------------------------------------------------------
# USAGE: activate <curve> ?<settings>?
#
# Clients use this to add a curve to the plot.  The optional <settings>
# are used to configure the plot.  Allowed settings are -color,
# -brightness, -width, -linestyle and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::XyLegend::Activate {} {
    foreach elem [$graph_ element show] {
	set label [$graph_ element cget $elem -label]
	if { $label == "" } {
	    continue
	}
	Add $elem $label
    }
    $itk_component(legend) open -recurse root
}

itcl::body Rappture::XyLegend::Deactivate {} {
    foreach node [$tree_ children root] {
	$tree_ delete $node
    }
}

itcl::body Rappture::XyLegend::Hide { args } {
    if { $args == "" } {
	set nodes [$itk_component(legend) curselection]
    } else {
	set nodes $args
    }
    foreach node $nodes {
	set elem [$tree_ label $node]
	if { ![$graph_ element exists $elem] } {
	    continue
	}
	$graph_ element configure $elem -hide yes
	$tree_ set $node "show" 1
    }
}

itcl::body Rappture::XyLegend::Show { args } {
    if { $args == "" } {
	set nodes [$itk_component(legend) curselection]
    } else {
	set nodes $args
    }
    foreach node $nodes {
	set elem [$tree_ label $node]
	if { ![$graph_ element exists $elem] } {
	    continue
	}
	$graph_ element configure $elem -hide no
	$tree_ set $node "show" 0
    }
}

itcl::body Rappture::XyLegend::Toggle { args } {
    if { $args == "" } {
	set nodes [$itk_component(legend) curselection]
    } else {
	set nodes $args
    }
    foreach node $nodes {
	set node [$itk_component(legend) index $node]
	set elem [$tree_ label $node]
	if { ![$graph_ element exists $elem] } {
	    continue
	}
	set hide [$graph_ element cget $elem -hide]
	set hide [expr $hide==0]
	$tree_ set $node "show" $hide
	$graph_ element configure $elem -hide $hide
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
	set elem [$tree_ label $node]
	set found($elem) 1
	set elements [linsert $elements 0 $elem]
    }
    foreach elem $elements {
	$tree_ move [$tree_ index $elem] 0 -at 0
    }
    set list {}
    foreach elem [$graph_ element show] {
	if { [info exists found($elem)] }  {
	    continue
	}
	lappend list $elem
    }
    $graph_ element show [concat $list $elements]
}

itcl::body Rappture::XyLegend::Lower { args } {
    if { $args == "" } {
	set nodes [$itk_component(legend) curselection]
    } else {
	set nodes $args
    }
    set elements {}
    foreach node $nodes {
	set elem [$tree_ label $node]
	set found($elem) 1
	set elements [linsert $elements 0 $elem]
    }
    set pos [$tree_ degree 0]

    foreach elem $elements {
	incr pos -1
	$tree_ move [$tree_ index $elem] 0 -at $pos
    }

    set list {}
    foreach elem [$graph_ element show] {
	if { [info exists found($elem)] }  {
	    continue
	}
	lappend list $elem
    }
    $graph_ element show [concat $elements $list]
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
	if { ![$tree_ get $node "delete" 0] } {
	    continue
	}
	set elem [$tree_ label $node]
	lappend elements $elem
	lappend delnodes $node
	if { $diff_ != "" && [info exists diffelements_($elem)] } {
	    $graph_ element delete $diff_
	    array unset diffelements_
	    set diff_ ""
	}
    }
    if { [llength $delnodes] > 0 } {
	eval $tree_ delete $delnodes 
    }
    $itk_component(legend) selection clearall
    eval $graph_ element delete $elements
}

itcl::body Rappture::XyLegend::Check {} {
    set nodes [$itk_component(legend) curselection]
    foreach n { hide show toggle raise lower 
	rename average difference delete } {
	$itk_component(controls).$n configure -state disabled
    }
    foreach node $nodes {
	if { [$tree_ get $node "delete" 0] } {
	    $itk_component(controls).delete configure -state normal
	    break
	}
    }
    if { [$itk_component(legend) index focus] != -1 } {
	    $itk_component(controls).rename configure -state normal
    }
    if { [$tree_ degree 0] > 1  && [llength $nodes] > 0 } {
	foreach n { raise lower } {
	    $itk_component(controls).$n configure -state normal
	}
    }
    switch -- [llength $nodes] {
	0 {
	}
	1 {
	    foreach n { hide show toggle } {
		$itk_component(controls).$n configure -state normal
	    }
	}
	2 {
	    foreach n { hide show toggle difference } {
		$itk_component(controls).$n configure -state normal
	    }
	}
	default {
	    foreach n { hide show toggle } {
		$itk_component(controls).$n configure -state normal
	    }
	}
    }
}

itcl::body Rappture::XyLegend::GetData { elem what } {
    set y [$graph_ element cget $elem $what]
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

    # Step 1. Get the x-values for each curve, then sort them to get the
    #	      unique values.

    foreach node $nodes {
	set elem [$tree_ label $node]
	$xcoords append [GetData $elem -x]
	set elements [linsert $elements 0 $elem]
    }
    # Sort the abscissas keeping unique values.
    $xcoords sort -uniq

    # Step 2. Now for each curve, generate a cubic spline of that curve
    #	      and interpolate to get the corresponding y-values for each 
    #	      abscissa.  Normally the abscissa are the same, so we're
    #	      interpolation the knots.

    set x [blt::vector create \#auto -command ""]
    set y [blt::vector create \#auto -command ""]
    $sum length [$xcoords length]

    foreach node $nodes {
	set elem [$tree_ label $node]
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
    #	       the end.

    set count 0
    while {[$graph_ element exists avg$count] } {
	incr count
    }
    set elements [lsort -dictionary $elements]
    set name "avg$count" 
    set label "Avg. [join $elements ,]"

    # Don't use the vector because we don't know when it will be cleaned up.

    $graph_ element create $name -label $label -x [$xcoords range 0 end]\
	-y [$sum range 0 end] -symbol scross
    blt::vector destroy $xcoords $ycoords $sum
    set node [Add $name $label -delete]
    Raise $node
}

itcl::body Rappture::XyLegend::Difference {} {
    set nodes [$itk_component(legend) curselection]
    set elements {}
    set sum [blt::vector create \#auto -command ""]

    set xcoords [blt::vector create \#auto -command ""]
    set ycoords [blt::vector create \#auto -command ""]

    # Step 1. Get the x-values for each curve, then sort them to get the
    #	      unique values.

    foreach node $nodes {
	set elem [$tree_ label $node]
	$xcoords append [GetData $elem -x]
	set elements [linsert $elements 0 $elem]
    }
    # Sort the abscissas keeping unique values.
    $xcoords sort -uniq

    # Step 2. Now for each curve, generate a cubic spline of that curve
    #	      and interpolate to get the corresponding y-values for each 
    #	      abscissa.  Normally the abscissa are the same, so we're
    #	      interpolation the knots.

    set x [blt::vector create \#auto -command ""]
    set y [blt::vector create \#auto -command ""]
    $sum length [$xcoords length]

    foreach node $nodes {
	set elem [$tree_ label $node]
	$x set [GetData $elem -x]
	$y set [GetData $elem -y]
	blt::spline natural $x $y $xcoords $ycoords

	# Sum the interpolated y-coordinate values.
	$sum expr "fabs($ycoords - $sum)" 
    }
    blt::vector destroy $x $y

    # Get the average
    #$sum expr "$sum / [llength $elements]"

    # Step 3.  Create a new curve which is the average. Append it to the
    #	       the end.

    set count 0
    while {[$graph_ element exists diff$count] } {
	incr count
    }
    set elements [lsort -dictionary $elements]
    set name "diff$count" 
    set label "Diff. [join $elements ,]"

    # Don't use the vector because we don't know when it will be cleaned up.

    $graph_ element create $name -label $label -x [$xcoords range 0 end] \
	-y [$sum range 0 end]
    blt::vector destroy $xcoords $ycoords $sum
    set node [Add $name $label]
    Raise $node
}

itcl::body Rappture::XyLegend::Difference {} {

    if { $diff_ != "" } {
	$graph_ marker delete $diff_
	set diff_ ""
    }
    set nodes [$itk_component(legend) curselection]
    set elem1 [$tree_ label [lindex $nodes 0]]
    set elem2 [$tree_ label [lindex $nodes 1]]
    if { [info exists diffelements_($elem1)] && 
	 [info exists diffelements_($elem2)] } {
	array unset diffelements_
	return
    }
    array unset diffelements_
    set x [blt::vector create \#auto -command ""]
    set y [blt::vector create \#auto -command ""]
    set m [blt::vector create \#auto -command ""]

    $x append [GetData $elem1 -x]
    $y append [GetData $elem1 -y]
    $x sort -reverse $y 
    $x append [GetData $elem2 -x]
    $y append [GetData $elem2 -y]
    $m merge $x $y
    set diff_ [$graph_ marker create polygon \
		   -coords [$m range 0 end] \
		   -elem $elem1 \
		   -stipple dot1 \
		   -outline "" -fill blue4 ]
    blt::vector destroy $m $x $y
    set diffelements_($elem1) 1
    set diffelements_($elem2) 1
}


itcl::body Rappture::XyLegend::SelectAll { } {
    foreach node [$tree_ children 0] {
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
	    set focus_ [$itk_component(legend) index focus]
	    if { $focus_ == -1 } {
		return;
	    }
	    set label [$itk_component(legend) entry cget $focus_ -label]
	    foreach { l r t b } [$itk_component(legend) bbox $focus_] break
	    set info(text) $label
	    set info(x) [expr $l + [winfo rootx $itk_component(legend)]]
	    set info(y) [expr $r + [winfo rooty $itk_component(legend)]] 
	    set info(w) [expr $r - $l] 
	    set info(h) [expr $b - $t]
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
	    $itk_component(legend) entry configure $focus_ -label $label
        }
        menu {
            eval tk_popup $itk_component(emenu) $args
        }
        default {
            error "bad option \"$option\": should be popup, activate, validate, apply, and menu"
        }
    }
}
