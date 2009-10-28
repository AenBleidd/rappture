
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
package require Itk
package require BLT

#option add *XyPrint.width 3i widgetDefault
#option add *XyPrint.height 3i widgetDefault
option add *XyPrint.gridColor #d9d9d9 widgetDefault
option add *XyPrint.activeColor blue widgetDefault
option add *XyPrint.dimColor gray widgetDefault
option add *XyPrint.controlBackground gray widgetDefault
option add *XyPrint*font "Arial 10"
option add *XyPrint*Entry*width "6" widgetDefault
option add *XyPrint*Entry*background "white" widgetDefault

array set Rappture::axistypes {
    x x
    y y 
    x2 x
    y2 y
}

itcl::class Rappture::XyPrint {
    inherit itk::Widget

    constructor {args} {}
    destructor {}

    private variable _graph "";		# Original graph. 
    private variable _clone "";		# Cloned graph.
    private variable _preview "";	# Preview image.
    private variable _savedSettings;	# Array of settings.
	
    public method print { graph toolName plotName }

    private method CopyOptions { cmd orig clone } 
    private method CopyBindings { oper orig clone args } 
    private method CloneGraph { orig }

    private method BuildGeneralTab {}
    private method BuildLayoutTab {}
    private method BuildLegendTab {}
    private method BuildAxisTab {}
    private method SetOption { opt }
    private method SetComponentOption { comp opt }
    private method SetNamedComponentOption { comp name opt }
    private method GetAxis {}
    private method GetElement { args }
    private method RegeneratePreview {} 
    private method InitClone {} 
    private method Pixels2Inches { pixels } 
    private method Inches2Pixels { inches } 

    private method ApplyGeneralSettings {} 
    private method ApplyLegendSettings {} 
    private method ApplyAxisSettings {} 
    private method ApplyElementSettings {} 
    private method ApplyLayoutSettings {} 
    private method InitializeSettings {} 
    private method CreateSettings { toolName plotName } 
    private method RestoreSettings { toolName plotName } 
    private method SaveSettings { toolName plotName } 
    private method ResetSettings { } 
    private method GetOutput {}
    private method Done { state }
    private method DestroySettings {}
    private method restore { toolName plotName data } 
    private common _settings
    private common _wait
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyPrint::constructor { args } {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"
    
#     option add hull.width hull.height
#     pack propagate $itk_component(hull) no

    set w [winfo pixels . 2.5i]
    set h [winfo pixels . 2i]
    set _preview [image create photo -width $w -height $h]
    itk_component add tabs {
        blt::tabset $itk_interior.tabs \
            -highlightthickness 0 -tearoff 0 -side top \
            -bd 0 -gap 0 -tabborderwidth 1 \
            -outerpad 1 -background grey
    } {
        keep -cursor
        ignore -highlightthickness -borderwidth -background
    }
    itk_component add preview {
        label $itk_interior.preview \
            -highlightthickness 0 -bd 0 -image $_preview -width 2.5i \
		-height 2.25i -background grey -padx 10 -pady 10
    } {
	ignore -background
    }
    itk_component add ok {
        button $itk_interior.ok -text "Print" \
            -highlightthickness 0 \
	    -command [itcl::code $this Done 1] \
	    -compound left \
	    -image [Rappture::icon download]
    }
    itk_component add cancel {
        button $itk_interior.cancel -text "Cancel" \
            -highlightthickness 0 \
	    -command [itcl::code $this Done 0] \
	    -compound left \
	    -image [Rappture::icon cancel]
    }
    blt::table $itk_interior \
	0,0 $itk_component(preview) -cspan 2 -fill both \
	1,0 $itk_component(tabs) -fill both -cspan 2 \
        2,1 $itk_component(cancel) -padx 2 -pady 2 -width .9i -fill y \
	2,0 $itk_component(ok) -padx 2 -pady 2 -width .9i -fill y 
    blt::table configure $itk_interior r1 -resize none
    blt::table configure $itk_interior r1 -resize both

    BuildGeneralTab
    BuildAxisTab
    BuildLegendTab
    BuildLayoutTab
    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::XyPrint::destructor {} {
    destroy $_clone
    image delete $_preview
    array unset _settings $this-*
}

itcl::body Rappture::XyPrint::DestroySettings {} {
    destroy $_clone
    array unset _settings $this-*
    set _clone ""
    set _graph ""
}

itcl::body Rappture::XyPrint::Done { state } {
    set _wait($this) $state
}

itcl::body Rappture::XyPrint::print { graph toolName plotName } {
    set _graph $graph
    set _clone [CloneGraph $graph]
    InitClone
    InitializeSettings
    # RestoreSettings $toolName $plotName 
    set _wait($this) 0
    tkwait variable [itcl::scope _wait($this)]
    set output ""
    if { $_wait($this) } {
	set output [GetOutput]
    }
    # SaveSettings $toolName $plotName 
    DestroySettings 
    return $output
}

itcl::body Rappture::XyPrint::GetOutput {} {
    # Get the selected format of the download.
    set page $itk_component(graph_page)
    set format [$page.format current]

    if { $format == "jpg" } {
	set img [image create photo]
	$_clone snap $img
	set bytes [$img data -format "jpeg -quality 100"]
	set bytes [Rappture::encoding::decode -as b64 $bytes]
	image delete $img
	return [list .jpg $bytes]
    } elseif { $format == "png" } {
	set img [image create photo]
	$_clone snap $img
	set bytes [$img data -format "png"]
	set bytes [Rappture::encoding::decode -as b64 $bytes]
	image delete $img
	return [list .png $bytes]
    }

    # Handle encapsulated postscript or portable document format.

    # Append an "i" for the graph postscript component.
    set w $_settings($this-layout-width)i
    set h $_settings($this-layout-height)i

    $_clone postscript configure \
	-maxpect false \
	-decoration yes \
	-center yes \
	-width $w -height $h
    
    set psdata [$_clone postscript output]

    if 1 { 
	set f [open "junk.raw" "w"] 
	puts -nonewline $f $psdata
	close $f
    }

    set cmd ""
    # | eps2eps << $psdata 
    lappend cmd "|" "/usr/bin/gs" \
	"-q" "-sDEVICE=epswrite" "-sstdout=%stderr" \
	"-sOutputFile=-" "-dNOPAUSE" "-dBATCH" "-dSAFER" \
	"-dDEVICEWIDTH=250000" "-dDEVICEHEIGHT=250000" "-" "<<" "$psdata"
    if { $format == "pdf" } {
	# | eps2eps << $psdata | ps2pdf 
	lappend cmd "|" "/usr/bin/gs" \
	    "-q" "-sDEVICE=pdfwrite" "-sstdout=%stderr" \
	    "-sOutputFile=-" "-dNOPAUSE" "-dBATCH" "-dSAFER" \
	    "-dCompatibilityLevel=1.4" "-c" ".setpdfwrite" "-f" "-"
    }
    if { [catch {
	set f [open $cmd "r"]
	fconfigure $f -translation binary -encoding binary
	set output [read $f]
	close $f
    } err ] != 0 } {
	global errorInfo 
	puts stderr "failed to generate file: $err\n$errorInfo"
	return ""
    }
    if 0 {
	set f [open "junk.$format" "w"] 
	fconfigure $f -translation binary -encoding binary
	puts -nonewline $f $output
	close $f
    }
    return [list .$format $output]
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

itcl::body Rappture::XyPrint::CloneGraph { orig } {
    set top $itk_interior
    set clone [blt::graph $top.graph]
    CopyOptions "configure" $orig $clone
    # Axis component
    foreach axis [$orig axis names] {
	if { [$orig axis cget $axis -hide] } {
	    continue
	}
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
	CopyOptions [list element configure $elem] $orig $clone
    }
    # Fix element display list
    $clone element show [$orig element show]
    # Legend component
    CopyOptions {legend configure} $orig $clone
    # Postscript component
    CopyOptions {postscript configure} $orig $clone
    # Grid component
    CopyOptions {grid configure} $orig $clone
    # Grid component
    CopyOptions {crosshairs configure} $orig $clone
    return $clone
}

itcl::body Rappture::XyPrint::InitClone {} {
    
    $_clone configure -width 3.4i -height 3.4i -background white \
	-borderwidth 0 \
	-leftmargin 0 \
	-rightmargin 0 \
	-topmargin 0 \
	-bottommargin 0
    
    # Kill the title and create a border around the plot
    $_clone configure \
	-title "" \
	-plotborderwidth 1 -plotrelief solid  \
	-plotbackground white -plotpadx 0 -plotpady 0
    # 
    set _fonts(legend) [font create -family times -size 10 -weight normal]
    update
    $_clone legend configure \
	-position right \
	-font $_fonts(legend) \
	-hide yes -borderwidth 0 -background white -relief solid \
	-anchor nw -activeborderwidth 0
    # 
    foreach axis [$_clone axis names] {
	if { [$_clone axis cget $axis -hide] } {
	    continue
	}
	set _fonts($axis-ticks) [font create -family courier -size 10 \
				     -weight normal -slant roman]
	set _fonts($axis-title) [font create -family symbol -size 10 \
				     -weight normal -slant roman]
	puts stderr "tick fonts $_fonts($axis-ticks):  [font configure $_fonts($axis-ticks)]"
	puts stderr "title fonts $_fonts($axis-title): [font configure $_fonts($axis-title)]"
	update
	$_clone axis configure $axis -ticklength 5  \
	    -majorticks {} -minorticks {}
	$_clone axis configure $axis \
	    -tickfont $_fonts($axis-ticks) \
	    -titlefont $_fonts($axis-title)
    }
    #$_clone grid off
    #$_clone yaxis configure -rotate 90
    #$_clone y2axis configure -rotate 270
    foreach elem [$_clone element names] {
	$_clone element configure $elem -linewidth 1 \
	    -pixels 3 
    }
    # Create markers representing lines at zero for the x and y axis.
    $_clone marker create line -name x-zero \
	-coords "0 -Inf 0 Inf" -dashes 1 -hide yes
    $_clone marker create line -name y-zero \
	-coords "-Inf 0 Inf 0" -dashes 1 -hide yes
}

itcl::body Rappture::XyPrint::SetOption { opt } {
    set new $_settings($this-graph-$opt) 
    set old [$_clone cget -$opt]
    set code [catch [list $_clone configure -$opt $new] err]
    if { $code != 0 } {
	bell
	global errorInfo
	puts stderr "$err: $errorInfo"
	set _settings($this-graph-$opt) $old
    }
}

itcl::body Rappture::XyPrint::SetComponentOption { comp opt } {
    set new $_settings($this-$comp-$opt) 
    set old [$_clone $comp cget -$opt]
    set code [catch [list $_clone $comp configure -$opt $new] err]
    if { $code != 0 } {
	bell
	global errorInfo
	puts stderr "$err: $errorInfo"
	set _settings($this-$comp-$opt) $old
    }
}

itcl::body Rappture::XyPrint::SetNamedComponentOption { comp name opt } {
    set new $_settings($this-$comp-$opt) 
    set old [$_clone $comp cget $name -$opt]
    set code [catch [list $_clone $comp configure $name -$opt $new] err]
    if { $code != 0 } {
	bell
	global errorInfo
	puts stderr "$err: $errorInfo"
	set _settings($this-$comp-$opt) $old
    }
}

itcl::body Rappture::XyPrint::RegeneratePreview {} {
    update 
    set img [image create photo]

    set w [Inches2Pixels $_settings($this-layout-width)]
    set h [Inches2Pixels $_settings($this-layout-height)]
    set pixelsPerInch [winfo pixels . 1i]
    set sx [expr 2.5*$pixelsPerInch/$w]
    set sy [expr 2.0*$pixelsPerInch/$h]
    set s [expr min($sx,$sy)]
    $_clone snap $img -width $w -height $h

    if 0 {
    if { ![winfo exists .labeltest] } {
	toplevel .labeltest -bg red
	label .labeltest.label -image $img
	pack .labeltest.label -fill both 
    } 
    }
    set pw [expr int(round($s * $w))]
    set ph [expr int(round($s * $h))]
    $_preview configure -width $pw -height $ph
    #.labeltest.label configure -image $img
    blt::winop resample $img $_preview box
    image delete $img
}

itcl::body Rappture::XyPrint::Pixels2Inches { pixels } {
    if { [llength $pixels] == 2 } {
	set pixels [lindex $pixels 0]
    }
    set pixelsPerInch [winfo pixels . 1i]
    set inches [expr { double($pixels) / $pixelsPerInch }]
    return [format %.3g ${inches}]
}

itcl::body Rappture::XyPrint::Inches2Pixels { inches } {
    return  [winfo pixels . ${inches}i]
}


itcl::body Rappture::XyPrint::GetAxis {} {
    set axis [$itk_component(axis_combo) current]
    foreach option { min max loose title stepsize subdivisions } {
	set _settings($this-axis-$option) [$_clone axis cget $axis -$option]
    }
    set type $Rappture::axistypes($axis)
    if { [$_clone grid cget -map${type}] == $axis } {
	set _settings($this-axis-grid) 1
    }  else {
	set _settings($this-axis-grid) 0
    }
    set _settings($this-axis-zero) [$_clone marker cget ${type}-zero -hide]
}

itcl::body Rappture::XyPrint::GetElement { args } {
    set index 1
    foreach elem [$_clone element show] {
	set _settings($this-element-$index) $elem
	incr index
    }
    set index [$itk_component(element_slider) get]
    set elem $_settings($this-element-$index)
    set _settings($this-element-symbol) [$_clone element cget $elem -symbol]
    set _settings($this-element-color) [$_clone element cget $elem -color]
    set _settings($this-element-dashes) [$_clone element cget $elem -dashes]
    set _settings($this-element-hide) [$_clone element cget $elem -hide]
    set _settings($this-element-label) [$_clone element cget $elem -label]
    set page $itk_component(legend_page)
    $page.color value [$page.color label $_settings($this-element-color)]
    $page.symbol value [$page.symbol label $_settings($this-element-symbol)]
    $page.dashes value [$page.dashes label $_settings($this-element-dashes)]
    #FixElement
}

itcl::body Rappture::XyPrint::BuildLayoutTab {} {
    itk_component add layout_page {
	frame $itk_component(tabs).layout_page 
    }
    set page $itk_component(layout_page)
    $itk_component(tabs) insert end "layout" \
        -text "Layout" -padx 2 -pady 2 -window $page -fill both

    label $page.width_l -text "width"  
    entry $page.width -width 6 \
	-textvariable [itcl::scope _settings($this-layout-width)]
    bind  $page.width <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    label $page.height_l -text "height" 
    entry $page.height -width 6 \
	-textvariable [itcl::scope _settings($this-layout-height)]
    bind  $page.height <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    label $page.margin_l -text "Margins"  

    label $page.left_l -text "left"
    entry $page.left -width 6 \
	-textvariable [itcl::scope _settings($this-layout-leftmargin)]
    bind  $page.left <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    label $page.right_l -text "right" 
    entry $page.right -width 6 \
	-textvariable [itcl::scope _settings($this-layout-rightmargin)]
    bind  $page.right <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    label $page.top_l -text "top"
    entry $page.top -width 6 \
	-textvariable [itcl::scope _settings($this-layout-topmargin)]
    bind  $page.top <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    label $page.bottom_l -text "bottom"
    entry $page.bottom -width 6 \
	-textvariable [itcl::scope _settings($this-layout-bottommargin)]
    bind  $page.bottom <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    blt::table $page \
	1,0 $page.width_l -anchor e \
	1,1 $page.width -fill x -cspan 2 \
	1,3 $page.height_l -anchor e \
	1,4 $page.height -fill x -cspan 2 \
	3,2 $page.top_l -anchor e \
	3,3 $page.top -fill x \
	4,0 $page.left_l -anchor e \
	4,1 $page.left -fill x  \
	4,4 $page.right_l -anchor e \
	4,5 $page.right -fill x \
	5,2 $page.bottom_l -anchor e \
	5,3 $page.bottom -fill x  

    blt::table configure $page r* -resize none
    blt::table configure $page r2 -resize both
    blt::table configure $page c2 c4 -width .5i
    blt::table configure $page r6 -height .125i
}


itcl::body Rappture::XyPrint::BuildGeneralTab {} {
    itk_component add graph_page {
	frame $itk_component(tabs).graph_page 
    }
    set page $itk_component(graph_page)
    $itk_component(tabs) insert end "graph" \
        -text "General" -padx 2 -pady 2 -window $page -fill both

    label $page.format_l -text "format"
    Rappture::Combobox $page.format -width 30 -editable no
    $page.format choices insert end \
	"eps" "EPS Encapsulated PostScript"  \
	"pdf" "PDF Portable Document Format" \
	"jpg"  "JPEG Joint Photographic Experts Group Format" \
	"png"  "PNG Portable Network Graphics Format"         

    bind $page.format <<Value>> [itcl::code $this ApplyGeneralSettings]

    label $page.style_l -text "style" 
    Rappture::Combobox $page.style -width 20 -editable no
    $page.style choices insert end \
	"ieee" "IEEE Journal"  \
	"gekco" "G Klimeck"  
    bind $page.style <<Value>> [itcl::code $this ApplyGeneralSettings]

    checkbutton $page.remember -text "remember settings" \
	-variable [itcl::scope _settings($this-general-remember)]  

    blt::table $page \
	2,0 $page.format_l -anchor e \
	2,1 $page.format -fill x \
	3,0 $page.style_l -anchor e \
	3,1 $page.style -fill x \
	5,0 $page.remember -cspan 2 -anchor w 
    blt::table configure $page r* -resize none
    blt::table configure $page r4 -resize both

}
    
itcl::body Rappture::XyPrint::ApplyLegendSettings {} {
    set page $itk_component(legend_page)
    set _settings($this-legend-position)  [$page.position current]
    set _settings($this-legend-anchor)    [$page.anchor current]
    foreach option { hide position anchor borderwidth } {
	SetComponentOption legend $option
    }
    ApplyElementSettings
}

itcl::body Rappture::XyPrint::BuildLegendTab {} {
    itk_component add legend_page {
	frame $itk_component(tabs).legend_page 
    }
    set page $itk_component(legend_page)
    $itk_component(tabs) insert end "legend" \
        -text "Legend" -padx 2 -pady 2 -window $page -fill both

    checkbutton $page.show -text "show legend" \
	-offvalue 1 -onvalue 0 \
	-variable [itcl::scope _settings($this-legend-hide)]  \
	-command [itcl::code $this ApplyLegendSettings] 

    label $page.position_l -text "position"
    Rappture::Combobox $page.position -width 15 -editable no
    $page.position choices insert end \
	"leftmargin" "left margin"  \
	"rightmargin" "right margin"  \
	"bottommargin" "bottom margin"  \
	"topmargin" "top margin"  \
	"plotarea" "inside plot" 
    bind $page.position <<Value>> [itcl::code $this ApplyLegendSettings]

    Rappture::Combobox $page.anchor -width 10 -editable no
    $page.anchor choices insert end \
	"nw" "northwest"  \
	"n" "north"  \
	"ne" "northeast"  \
	"sw" "southwest"  \
	"s" "south"  \
	"se" "southeast"  \
	"c" "center"  \
	"e" "east"  \
	"w" "west"  
    bind $page.anchor <<Value>> [itcl::code $this ApplyLegendSettings]

    checkbutton $page.border -text "border" \
	-font "Arial 10"  \
	-variable [itcl::scope _settings($this-legend-borderwidth)] \
	-onvalue 1 -offvalue 0 \
	-command [itcl::code $this ApplyLegendSettings]

    label $page.slider_l -text "legend\nentry"  -font "Arial 10" -justify right
    itk_component add element_slider {
	::scale $page.slider -from 1 -to 1 \
	    -orient horizontal -width 12 \
	    -command [itcl::code $this GetElement]
    } 
    checkbutton $page.hide -text "hide" \
	-font "Arial 10"  \
	-variable [itcl::scope _settings($this-element-hide)] \
	-command [itcl::code $this ApplyElementSettings]

    label $page.label_l -text "label" -font "Arial 10" 
    entry $page.label \
	-background white \
	-font "Arial 10" \
	-textvariable [itcl::scope _settings($this-element-label)]
    bind  $page.label <KeyPress-Return> [itcl::code $this ApplyElementSettings]

    label $page.color_l -text "color "  -font "Arial 10" 
    Rappture::Combobox $page.color -width 15 -editable no
    $page.color choices insert end \
	"#000000" "black" \
	"#ffffff" "white" \
	"#0000cd" "blue" \
	"#cd0000" "red" \
	"#00cd00" "green" \
	"#3a5fcd" "royal blue" \
        "#cdcd00" "yellow" \
        "#cd1076" "deep pink" \
        "#009acd" "deep sky blue" \
	"#00c5cd" "torquise" \
        "#a2b5cd" "light steel blue" \
	"#7ac5cd" "cadet blue" \
	"#66cdaa" "aquamarine" \
	"#a2cd5a" "dark olive green" \
        "#cd9b9b" "rosy brown"
    bind $page.color <<Value>> [itcl::code $this ApplyElementSettings]

    label $page.dashes_l -text "line style"  -font "Arial 10" 
    Rappture::Combobox $page.dashes -width 15 -editable no
    $page.dashes choices insert end \
	"" "solid"  \
        "1" "dot" \
        "5 2" "dash" \
        "2 4 2" "dashdot" \
        "2 4 2 2" "dashdotdot"
    bind $page.dashes <<Value>> [itcl::code $this ApplyElementSettings]

    label $page.symbol_l -text "symbol"  -font "Arial 10" 
    Rappture::Combobox $page.symbol -editable no
    $page.symbol choices insert end \
	"none" "none"  \
	"square" "square" \
	"circle" "circle" \
	"diamond" "diamond" \
	"plus" "plus" \
	"cross" "cross" \
	"splus" "skinny plus"  \
	"scross" "skinny cross" \
	"triangle" "triangle"
    bind $page.symbol <<Value>> [itcl::code $this ApplyElementSettings]

    blt::table $page \
	1,0 $page.show -cspan 2 -anchor w \
	1,2 $page.border -cspan 2 -anchor w \
	2,0 $page.position_l -anchor e \
	2,1 $page.position -fill x \
	2,2 $page.anchor -fill x  -cspan 2 \
	3,0 $page.slider_l -anchor e \
	3,1 $page.slider -fill x -cspan 4 \
	4,0 $page.label_l -anchor e \
	4,1 $page.label -fill x -cspan 3 \
	5,0 $page.color_l -anchor e \
	5,1 $page.color -fill x \
	5,2 $page.symbol_l -anchor e \
	5,3 $page.symbol -fill both \
	6,0 $page.dashes_l -anchor e \
	6,1 $page.dashes -fill x \

    blt::table configure $page r* -resize none
    blt::table configure $page r8 -resize both

}

itcl::body Rappture::XyPrint::BuildAxisTab {} {
    itk_component add axis_page {
	frame $itk_component(tabs).axis_page 
    }
    set page $itk_component(axis_page)
    $itk_component(tabs) insert end "axis" \
        -text "Axis" -padx 2 -pady 2 -window $page -fill both
    
    label $page.axis_l -text "axis"  -font "Arial 10" 
    itk_component add axis_combo {
	Rappture::Combobox $page.axis -width 20 -editable no
    } 
    bind $itk_component(axis_combo) <<Value>> [itcl::code $this GetAxis]

    label $page.title_l -text "title"  -font "Arial 10" 
    entry $page.title \
	-textvariable [itcl::scope _settings($this-axis-title)]
    bind  $page.title <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    label $page.min_l -text "min"  -font "Arial 10" 
    entry $page.min -width 10 \
	-textvariable [itcl::scope _settings($this-axis-min)]
    bind  $page.min <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    label $page.max_l -text "max"  -font "Arial 10" 
    entry $page.max -width 10 \
	-textvariable [itcl::scope _settings($this-axis-max)]
    bind  $page.max <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    label $page.subdivisions_l -text "subdivisions"  -font "Arial 10" 
    entry $page.subdivisions \
	-textvariable [itcl::scope _settings($this-axis-subdivisions)]
    bind  $page.subdivisions <KeyPress-Return> \
	[itcl::code $this ApplyAxisSettings]

    label $page.stepsize_l -text "step size"  -font "Arial 10" 
    entry $page.stepsize \
	-textvariable [itcl::scope _settings($this-axis-stepsize)]
    bind  $page.stepsize <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    checkbutton $page.loose -text "loose limits" \
	-onvalue "always" -offvalue "0" \
	-variable [itcl::scope _settings($this-axis-loose)] \
	-command [itcl::code $this ApplyAxisSettings]

    label $page.plotpadx_l -text "pad x-axis"  
    entry $page.plotpadx -width 6 \
	-textvariable [itcl::scope _settings($this-graph-plotpadx)]
    bind  $page.plotpadx <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    label $page.plotpady_l -text "pad y-axis" 
    entry $page.plotpady -width 6 \
	-textvariable [itcl::scope _settings($this-graph-plotpady)]
    bind  $page.plotpady <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    checkbutton $page.grid -text "show grid lines" \
	-variable [itcl::scope _settings($this-axis-grid)] \
	-command [itcl::code $this ApplyAxisSettings]

    checkbutton $page.zero -text "mark zero" \
	-offvalue 1 -onvalue 0 \
	-variable [itcl::scope _settings($this-axis-zero)] \
	-command [itcl::code $this ApplyAxisSettings]

    label $page.tickfont_l -text "tick font"
    Rappture::Combobox $page.tickfontfamily -width 10 -editable no
    $page.tickfontfamily choices insert end \
	"courier" "Courier" \
	"helvetica" "Helvetica"  \
	"new*century*schoolbook"  "New Century Schoolbook" \
	"symbol"  "Symbol" \
	"times"  "Times"         
    bind $page.tickfontfamily <KeyPress-Return> \
	[itcl::code $this ApplyAxisSettings]

    Rappture::Combobox $page.tickfontsize -width 4 -editable no
    $page.tickfontsize choices insert end \
 	"8" "8" \
	"10" "10" \
	"11" "11" \
	"12" "12" \
	"14" "14" \
	"17" "17" \
	"18" "18" \
	"20" "20" 
    bind  $page.tickfontsize <KeyPress-Return> \
	[itcl::code $this ApplyAxisSettings]

    Rappture::PushButton $page.tickfontbold \
	-onimage [Rappture::icon format-text-bold] \
	-offimage [Rappture::icon format-text-bold] \
	-command [itcl::code $this ApplyAxisSettings] \
	-variable [itcl::scope _settings($this-axis-tickfont-bold)]

    Rappture::PushButton $page.tickfontitalic \
	-onimage [Rappture::icon format-text-italic] \
	-offimage [Rappture::icon format-text-italic] \
	-command [itcl::code $this ApplyAxisSettings] \
	-variable [itcl::scope _settings($this-axis-tickfont-italic)]

    blt::table $page \
	1,0 $page.axis_l -anchor e  -pady 4 \
	1,1 $page.axis -fill x -cspan 4 \
	2,1 $page.title_l -anchor e \
	2,2 $page.title -fill x -cspan 3 \
	3,1 $page.min_l -anchor e \
	3,2 $page.min -fill x \
	3,3 $page.max_l -anchor e \
	3,4 $page.max -fill both \
	5,1 $page.stepsize_l -anchor e \
	5,2 $page.stepsize -fill both \
	5,3 $page.subdivisions_l -anchor e \
	5,4 $page.subdivisions -fill both \
	6,1 $page.plotpadx_l -anchor e \
	6,2 $page.plotpadx -fill both \
	6,3 $page.plotpady_l -anchor e \
	6,4 $page.plotpady -fill both \
	7,1 $page.loose -cspan 2 -anchor w \
	7,3 $page.grid -anchor w -cspan 2 \
	8,1 $page.zero -cspan 2 -anchor w \
	9,1 $page.tickfont_l -anchor e \
	9,2 $page.tickfontfamily -fill x \
	9,3 $page.tickfontsize -fill x \
	9,4 $page.tickfontbold -anchor e \
	9,5 $page.tickfontitalic -anchor e 
}

itcl::body Rappture::XyPrint::ApplyGeneralSettings {} {
    RegeneratePreview
}

itcl::body Rappture::XyPrint::ApplyLegendSettings {} {
    set page $itk_component(legend_page)
    set _settings($this-legend-position)  [$page.position current]
    set _settings($this-legend-anchor)    [$page.anchor current]

    foreach option { hide position anchor borderwidth } {
	SetComponentOption legend $option
    }
    ApplyElementSettings
}

itcl::body Rappture::XyPrint::ApplyAxisSettings {} {
    set plotpadx [Inches2Pixels $_settings($this-graph-plotpadx)]
    set plotpady [Inches2Pixels $_settings($this-graph-plotpady)]
    SetOption plotpadx
    SetOption plotpady
    set axis [$itk_component(axis_combo) current]
    set type $Rappture::axistypes($axis)
    if { $_settings($this-axis-grid) } {
	$_clone grid configure -hide no -map${type} ${axis}
    } else {
	$_clone grid configure -hide no -map${type} ""
    }
    foreach option { min max loose title stepsize subdivisions } {
	SetNamedComponentOption axis $axis $option
    }
    $_clone marker configure ${type}-zero -hide $_settings($this-axis-zero)
    GetAxis
    RegeneratePreview
}

itcl::body Rappture::XyPrint::ApplyElementSettings {} {
    set index [$itk_component(element_slider) get]
    set elem $_settings($this-element-$index)
    set page $itk_component(legend_page)
    set _settings($this-element-color)  [$page.color current]
    set _settings($this-element-symbol) [$page.symbol current]
    set _settings($this-element-dashes) [$page.dashes current]
    foreach option { symbol color dashes hide label } {
	SetNamedComponentOption element $elem $option
    }
    RegeneratePreview
}

itcl::body Rappture::XyPrint::ApplyLayoutSettings {} {
    foreach opt { leftmargin rightmargin topmargin bottommargin } {
	set new [Inches2Pixels $_settings($this-layout-$opt)]
	set old [$_clone cget -$opt]
	set code [catch [list $_clone configure -$opt $new] err]
	if { $code != 0 } {
	    bell
	    global errorInfo
	    puts stderr "$err: $errorInfo"
	    set _settings($this-layout-$opt) [Pixel2Inches $old]
	}
    }
    RegeneratePreview
}


itcl::body Rappture::XyPrint::InitializeSettings {} {
    # General settings

    # Always set to "eps" "ieee"
    set _settings($this-general-format) eps
    set _settings($this-general-style) ieee
    set _settings($this-general-remember) 1
    set page $itk_component(graph_page)
    $page.format value [$page.format label $_settings($this-general-format)]
    $page.style value [$page.style label $_settings($this-general-style)]

    # Layout settings
    set _settings($this-layout-width) [Pixels2Inches [$_clone cget -width]]
    set _settings($this-layout-height) [Pixels2Inches [$_clone cget -height]]
    set _settings($this-layout-leftmargin) \
	[Pixels2Inches [$_clone cget -leftmargin]]
    set _settings($this-layout-rightmargin) \
	[Pixels2Inches [$_clone cget -rightmargin]]
    set _settings($this-layout-topmargin) \
	[Pixels2Inches [$_clone cget -topmargin]]
    set _settings($this-layout-bottommargin) \
	[Pixels2Inches [$_clone cget -bottommargin]]

    # Legend settings
    set page $itk_component(legend_page)

    set names [$_clone element show]
    $itk_component(element_slider) configure -from 1 -to [llength $names] 
    set state [expr  { ([llength $names] < 2) ? "disabled" : "normal" } ]
    $page.slider configure -state $state
    $page.slider_l configure -state $state
    # Always initialize the slider to the first entry in the element list.
    $itk_component(element_slider) set 1
    # Always set the borderwidth to be not displayed
    set _settings($this-legend-borderwidth) 0

    set _settings($this-legend-hide) [$_clone legend cget -hide]
    set _settings($this-legend-position) [$_clone legend cget -position]
    set _settings($this-legend-anchor) [$_clone legend cget -anchor]
    $page.position value \
	[$page.position label $_settings($this-legend-position)]
    $page.anchor value [$page.anchor label $_settings($this-legend-anchor)]
    GetElement

    # Axis settings
    set names [lsort [$_clone axis names]] 
    $itk_component(axis_combo) choices delete 0 end
    foreach axis $names {
	if { ![$_clone axis cget $axis -hide] } {
	    $itk_component(axis_combo) choices insert end $axis $axis
	}
	lappend axisnames $axis
    }
    # Always hide the zero line.
    set _settings($this-axis-zero) 1

    # Pick the first axis to initially display
    set axis [lindex $axisnames 0]
    $itk_component(axis_combo) value $axis
    blt::table configure $page r* -resize none
    blt::table configure $page r9 -resize both
    set _settings($this-graph-plotpadx) [Pixels2Inches [$_clone cget -plotpadx]]
    set _settings($this-graph-plotpady) [Pixels2Inches [$_clone cget -plotpady]]
    GetAxis 
}


itcl::body Rappture::XyPrint::restore { toolName plotName data } {
    set key [list $toolName $plotName]
    set _savedSettings($key) $data
}

itcl::body Rappture::XyPrint::RestoreSettings { toolName plotName } {
    if { ![file readable "~/.rappture"] } {
	return;				# No file or not readable
    }
    # Read the file by sourcing it into a safe interpreter The only commands
    # executed will be "xyprint" which will simply add the data into an array
    # _savedSettings.
    set parser [interp create -safe]
    $parser alias xyprint [itcl::code $this restore]
    set f [open $file "r"]
    set code [read $f]
    close $f
    $parser eval $code
    
    # Now see if there's an entry for this tool/plot combination.  The data
    # associated with the variable is itself code to update the graph (clone).
    set key [list $toolName $plotName]
    if { [info exists _savedSettings($key)] }  {
	$parser alias "xygraph" $_clone
	$parser eval $_savedSettings($key)
    }
    foreach {name value} [$parser eval "array get general"] {
	set _settings($this-graph-$name) $value
    }
    interp delete $parser
}

itcl::body Rappture::XyPrint::ResetSettings {} {
    # Revert the widget back to the original graph's settings
    destroy $_clone
    set _clone [CloneGraph $graph]
    InitClone
    InitializeSettings
}

itcl::body Rappture::XyPrint::SaveSettings { toolName plotName } {
    if { ![file writable "~/.rappture"] } {
	return
    }
    set out [CreateSettings $toolName $plotName]
    # Write the settings out
    set f [open ".rappture" "w" 0600] 
    foreach key [lsort [array names _savedSettings]] {
	set tool [lindex $key 0]
	set plot [lindex $key 1]
	if { $plotName == "plot" && $toolName == "$tool" } {
	    continue
	}
	puts $f "xyprint \"$tool\" \"$plot\" \{"
	puts $f "$_savedSettings($key)"
	puts $f "\}\n"
    }
    # Now write the new setting
    puts $f "xyprint \"$toolName\" \"$plotName\" \{"
    puts $f "$out"
    puts $f "\}\n"
    close $f
}

itcl::body Rappture::XyPrint::CreateSettings { toolName plotName } {
    # Create stanza associated with tool and plot title.
    # General settings
    append out "\n"
    append out "xyprint \"\$toolName\" \"\$plotName\" \{\n"
    append out "  set general(format) $_settings($this-general-format)\n"
    append out "  set general(style) $_settings($this-general-style)\n"
    append out "  set general(remember) 1\n"

    # Layout settings
    append out "  xygraph configure \\\n" 
    append out "    -width \"[Pixels2Inches [$_clone cget -width]]\" \\\n"
    append out "    -height \"[Pixels2Inches [$_clone cget -height]]\" \\\n"
    append out "    -leftmargin \"[Pixels2Inches [$_clone cget -leftmargin]]\" \\\n"
    append out "    -rightmargin \"[Pixels2Inches [$_clone cget -rightmargin]]\" \\\n"
    append out "    -topmargin \"[Pixels2Inches [$_clone cget -topmargin]]\" \\\n"
    append out "    -bottommargin \"[Pixels2Inches [$_clone cget -bottommargin]]\"\\\n"
    append out "    -plotpadx \"[Pixels2Inches [$_clone cget -plotpadx]]\" \\\n"
    append out "    -plotpady \"[Pixels2Inches [$_clone cget -plotpady]]\" \n"

    # Legend settings
    append out "  xygraph legend configure \\\n" 
    append out "    -position \"[$_clone legend cget -position]\" \\\n"
    append out "    -anchor \"[$_clone legend cget -anchor]\" \\\n"
    append out "    -borderwidth \"[$_clone legend cget -borderwidth]\" \\\n"
    append out "    -hide \"[$_clone legend cget -hide]\" \n"
    
    # Element settings
    foreach elem [$_clone element show] {
 	append out "  if \{ \[xygraph element exists \"$elem\"\] \} \{\n"
	append out "    xygraph element configure \"$elem\" \\\n"
	append out "      -symbol \"[$_clone element cget $elem -symbol]\" \\\n"
	append out "      -color \"[$_clone element cget $elem -color]\" \\\n"
	append out "      -dashes \"[$_clone element cget $elem -dashes]\" \\\n"
	append out "      -hide \"[$_clone element cget $elem -hide]\" \\\n"
	append out "      -label \"[$_clone element cget $elem -label]\" \n"
	append out "  \}\n"
    }
    
    # Axis settings
    foreach axis [$_clone axis names] {
	if { [$_clone axis cget $axis -hide] } {
	    continue
	}
 	append out "  if \{ \[xygraph axis names \"$axis\"\] == \"$axis\" \} \{\n"
	append out "    xygraph axis configure \"$axis\" \\\n"
	append out "      -hide \"[$_clone axis cget $axis -hide]\" \\\n"
	append out "      -min \"[$_clone axis cget $axis -min]\" \\\n"
	append out "      -max \"[$_clone axis cget $axis -max]\" \\\n"
	append out "      -loose \"[$_clone axis cget $axis -loose]\" \\\n"
	append out "      -title \"[$_clone axis cget $axis -title]\" \\\n"
	append out "      -stepsize \"[$_clone axis cget $axis -stepsize]\" \\\n"
	append out "      -subdivisions \"[$_clone axis cget $axis -subdivisions]\"\n"
	set hide [$_clone marker cget ${axis}-zero -hide]
	append out "    xygraph marker configure \"${axis}-zero\" -hide $hide\n"
	append out "  \}\n"
    }	
    append out "  xygraph grid configure \\\n" 
    append out "    -hide \"[$_clone grid cget -hide]\" \\\n"
    append out "    -mapx \"[$_clone grid cget -mapx]\" \\\n"
    append out "    -mapy \"[$_clone grid cget -mapy]\"\n"
    append out "\}\n"
    return $out
}

