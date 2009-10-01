
# ----------------------------------------------------------------------
#  COMPONENT: xyprint - Print X-Y plot.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

itcl::class ::Rappture::XyPrint {
    constructor {args} {}
    destructor {}

    public method postscript { graph args }
    private method CopyOptions { cmd orig clone } 
    private method CopyBindings { oper orig clone args } 
    private method CloneGraph { orig }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyPrint::constructor { args } {
}

itcl::body Rappture::XyPrint::postscript { graph args } {
    
    set clone [CloneGraph $graph]
    
    # Now override the settings for printing.

    #
    $clone configure -width 3.4i -height 3.4i -background white \
	-borderwidth 0 \
	-leftmargin 0 \
	-rightmargin 0 \
	-topmargin 0 \
	-bottommargin 0
    
    # Kill the title and create a border around the plot
    $clone configure -title "" -plotborderwidth 1 -plotrelief solid  \
	-plotbackground white -plotpadx 0 -plotpady 0
    # 
    $clone legend configure -position right -font "*-helvetica-medium-r-normal-*-12-*" \
	-hide yes -borderwidth 0 -background white -relief flat \
	-anchor nw -activeborderwidth 0
    # 
    foreach axis [$clone axis names] {
	$clone axis configure $axis -ticklength 5 -loose always 
#-min "" -max ""
	$clone axis configure $axis -tickfont "*-helvetica-medium-r-normal-*-12-*" \
	    -titlefont "*-helvetica-medium-r-normal-*-12-*" 
    }
    $clone grid off
    #$clone yaxis configure -rotate 90
    #$clone y2axis configure -rotate 270
    foreach elem [$clone element names] {
	$clone element configure $elem -linewidth 1
    }
    set output [$clone postscript output \
		    -maxpect false -decoration false ]
    update
    $clone postscript output junk.eps -maxpect false -decorations yes
    #destroy [winfo toplevel $clone]
    return $output
}

itcl::body Rappture::XyPrint::CopyOptions { cmd orig clone } {
    set all [eval $orig $cmd]
    set configLine $clone
    foreach arg $cmd {
	lappend configLine $arg
    }
    foreach option $all {
	if { [llength $option] != 5 } {
	    continue
	}
	set switch [lindex $option 0]
	set initial [lindex $option 3]
	set current [lindex $option 4]
	if { [string compare $initial $current] == 0 } {
	    continue
	}
	lappend configLine $switch $current
    }
    eval $configLine
}

itcl::body Rappture::XyPrint::CopyBindings { oper orig clone args } {
    set tags [$orig $oper bind]
    if { [llength $args] > 0 } {
	lappend tags [lindex $args 0]
    }
    foreach tag $tags {
	foreach binding [$orig $oper bind $tag] {
	    set cmd [$orig $oper bind $tag $binding]
	    $clone $oper bind $tag $binding $cmd
	}
    }
}

itcl::body Rappture::XyPrint::CloneGraph { orig } {
    set top [toplevel .clone]
    set clone [blt::graph $top.graph]
    #wm geometry $top -1000-1000
    pack $clone -expand yes -fill both
    update
    CopyOptions "configure" $orig $clone
    # Axis component
    foreach axis [$orig axis names] {
	if { [$clone axis name $axis] == "" } {
	    $clone axis create $axis
	}
	CopyOptions [list axis configure $axis] $orig $clone
    }
    foreach axis { x y x2 y2 } {
	$clone ${axis}axis use [$orig ${axis}axis use]
    }
    # Pen component
    foreach pen [$orig pen names] {
	if { [$clone pen name $pen] == "" } {
	    $clone pen create $pen
	}
	CopyOptions [list pen configure $pen] $orig $clone
    }
    # Marker component
    foreach marker [$orig marker names] {
	$clone marker create [$orig marker type $marker] -name $marker
	CopyBindings marker $orig $clone $marker
	CopyOptions [list marker configure $marker] $orig $clone
    }
    # Element component
    foreach elem [$orig element names] {
	$clone element create $elem
	#CopyBindings element $orig $clone $elem
	CopyOptions [list element configure $elem] $orig $clone
    }
    # Fix element display list
    $clone element show [$orig element show]
    # Legend component
    CopyOptions {legend configure} $orig $clone
    #CopyBindings legend $orig $clone
    # Postscript component
    CopyOptions {postscript configure} $orig $clone
    # Grid component
    CopyOptions {grid configure} $orig $clone
    # Grid component
    CopyOptions {crosshairs configure} $orig $clone
    # Graph bindings
    #foreach binding [bind $orig] {
    #set cmd [bind $orig $binding]
    #bind $clone $binding $cmd
    #}
    return $clone
}
    
