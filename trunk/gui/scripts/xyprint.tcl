# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: xyprint - Print X-Y plot.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
option add *XyPrint*Font "Arial 9" widgetDefault
option add *XyPrint*Entry*width "6" widgetDefault
option add *XyPrint*Entry*background "white" widgetDefault

itcl::class Rappture::XyPrint {
    inherit itk::Widget

    constructor {args} {}
    destructor {}

    private variable _graph "";         # Original graph.
    private variable _clone "";         # Cloned graph.
    private variable _preview "";       # Preview image.
    private variable _savedSettings;    # Array of settings.

    private common _oldSettingsFile "~/.rpsettings"
    private common _settingsFile "~/.rp_settings"

    public method print { graph toolName plotName }
    public method reset {}

    private method CopyOptions { cmd orig clone {exclude {}}}
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
    private method Inches2Pixels { inches {defValue ""}}
    private method Color2RGB { color }

    private method ApplyGeneralSettings {}
    private method ApplyLegendSettings {}
    private method ApplyAxisSettings {}
    private method ApplyElementSettings {}
    private method ApplyLayoutSettings {}
    private method InitializeSettings {}
    private method CreateSettings { toolName plotName }
    private method RestoreSettings { toolName plotName }
    private method SaveSettings { toolName plotName }
    private method DestroySettings {}
    private method ResetSettings { }
    private method GetOutput {}
    private method SetWaitVariable { state }
    private method SetLayoutOption { option }
    private method GetAxisType { axis }
    private method restore { toolName plotName data }

    # Same dialog may be used for different graphs
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
    set inner [frame $itk_interior.frame -bg grey]
    itk_component add preview {
        label $inner.preview \
            -highlightthickness 0 -bd 0 -image $_preview -width 2.5i \
                -height 2.5i -background grey
    } {
        ignore -background
    }
    itk_component add ok {
        button $itk_interior.ok -text "Save" \
            -highlightthickness 0 -pady 2 -padx 0 \
            -command [itcl::code $this SetWaitVariable 1] \
            -compound left \
            -image [Rappture::icon download]
    }
    itk_component add cancel {
        button $itk_interior.cancel -text "Cancel" \
            -highlightthickness 0 -pady 2 -padx 0 \
            -command [itcl::code $this SetWaitVariable 0] \
            -compound left \
            -image [Rappture::icon cancel]
    }
    blt::table $itk_interior \
        0,0 $inner -fill both \
        0,1 $itk_component(tabs) -fill both -cspan 2 \
        1,2 $itk_component(cancel) -padx 2 -pady 2 -width .9i -fill y \
        1,1 $itk_component(ok) -padx 2 -pady 2 -width .9i -fill y
    blt::table $inner \
        0,0 $itk_component(preview) -fill both -padx 10 -pady 10

    #blt::table configure $itk_interior c1 c2 -resize none
    blt::table configure $itk_interior c0 -resize both
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
    set _clone ""
    set _graph ""
}

itcl::body Rappture::XyPrint::reset {} {
    SetWaitVariable 0
}

itcl::body Rappture::XyPrint::SetWaitVariable { state } {
    set _wait($this) $state
}

itcl::body Rappture::XyPrint::print { graph toolName plotName } {
    set _graph $graph
    CloneGraph $graph
    InitClone
    RestoreSettings $toolName $plotName
    InitializeSettings
    SetWaitVariable 0
    tkwait variable [itcl::scope _wait($this)]
    SaveSettings $toolName $plotName
    set output ""
    if { $_wait($this) } {
        set output [GetOutput]
    }
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

    if 0 {
        set f [open "junk.raw" "w"]
        puts -nonewline $f $psdata
        close $f
    }
    if { $format == "eps" } {
        return [list .$format $psdata]
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

itcl::body Rappture::XyPrint::CopyOptions { cmd orig clone {exclude {}} } {
    set all [eval $orig $cmd]
    set configLine $clone
    foreach name $exclude {
        set ignore($name) 1
    }
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
        if { [info exists ignore($switch)] } {
            continue
        }
        if { [string compare $initial $current] == 0 } {
            continue
        }
        lappend configLine $switch $current
    }
    eval $configLine
}

itcl::body Rappture::XyPrint::CloneGraph { orig } {
    set top $itk_interior
    if { [winfo exists $top.graph] } {
        destroy $top.graph
    }
    set _clone [blt::graph $top.graph]
    CopyOptions "configure" $orig $_clone
    # Axis component
    foreach axis [$orig axis names] {
        if { $axis == "z" } {
            continue
        }
        if { [$orig axis cget $axis -hide] } {
            continue
        }
        if { [$_clone axis name $axis] == "" } {
            $_clone axis create $axis
        }
        CopyOptions [list axis configure $axis] $orig $_clone
    }
    foreach axis { x y x2 y2 } {
        $_clone ${axis}axis use [$orig ${axis}axis use]
    }
    # Pen component
    foreach pen [$orig pen names] {
        if { [$_clone pen name $pen] == "" } {
            $_clone pen create $pen
        }
        CopyOptions [list pen configure $pen] $orig $_clone
    }
    # Marker component
    foreach marker [$orig marker names] {
        $_clone marker create [$orig marker type $marker] -name $marker
        CopyOptions [list marker configure $marker] $orig $_clone -name
    }
    # Element component
    foreach elem [$orig element names] {
        set oper [$orig element type $elem]
        $_clone $oper create $elem
        CopyOptions [list $oper configure $elem] $orig $_clone -data
        if { [$_clone $oper cget $elem -hide] } {
            $_clone $oper configure $elem -label ""
        }
    }
    # Fix element display list
    $_clone element show [$orig element show]
    # Legend component
    CopyOptions {legend configure} $orig $_clone
    # Postscript component
    CopyOptions {postscript configure} $orig $_clone
    # Crosshairs component
    CopyOptions {crosshairs configure} $orig $_clone
    # Grid component
    CopyOptions {grid configure} $orig $_clone

    # Create markers representing lines at zero for the x and y axis.
    $_clone marker create line -name x-zero \
        -coords "0 -Inf 0 Inf" -dashes 1 -hide yes
    $_clone marker create line -name y-zero \
        -coords "-Inf 0 Inf 0" -dashes 1 -hide yes
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

    set _settings($this-layout-width) [Pixels2Inches [$_clone cget -width]]
    set _settings($this-layout-height) [Pixels2Inches [$_clone cget -height]]

    set _settings($this-legend-fontfamily) helvetica
    set _settings($this-legend-fontsize)   10
    set _settings($this-legend-fontweight) normal
    set _settings($this-legend-fontslant)  roman
    set font "helvetica 10 normal roman"
    $_clone legend configure \
        -position right \
        -font $font \
        -hide yes -borderwidth 0 -background white -relief solid \
        -anchor nw -activeborderwidth 0
    #
    set _settings($this-axis-ticks-fontfamily) helvetica
    set _settings($this-axis-ticks-fontsize)   10
    set _settings($this-axis-ticks-fontweight) normal
    set _settings($this-axis-ticks-fontslant)  roman
    set _settings($this-axis-title-fontfamily) helvetica
    set _settings($this-axis-title-fontsize)   10
    set _settings($this-axis-title-fontweight) normal
    set _settings($this-axis-title-fontslant)  roman
    foreach axis [$_clone axis names] {
        if { $axis == "z" } {
            continue
        }
        if { [$_clone axis cget $axis -hide] } {
            continue
        }
        set _settings($this-$axis-ticks-fontfamily) helvetica
        set _settings($this-$axis-ticks-fontsize)   10
        set _settings($this-$axis-ticks-fontweight) normal
        set _settings($this-$axis-ticks-fontslant)  roman
        set _settings($this-$axis-title-fontfamily) helvetica
        set _settings($this-$axis-title-fontsize)   10
        set _settings($this-$axis-title-fontweight) normal
        set _settings($this-$axis-title-fontslant)  roman
        set tickfont "helvetica 10 normal roman"
        set titlefont "helvetica 10 normal roman"
        $_clone axis configure $axis -ticklength 5  \
            -majorticks {} -minorticks {}
        $_clone axis configure $axis \
            -tickfont $tickfont \
            -titlefont $titlefont
    }
    set count 0
    foreach elem [$_clone element names] {
        if { [$_clone element type $elem] == "bar" } {
            continue
        }
        incr count
        if { [$_clone element cget $elem -linewidth] > 1 } {
            $_clone element configure $elem -linewidth 1 -pixels 3
        }
    }
    if { $count == 0 } {
        # There are no "line" elements in the graph.
        # Remove the symbol and dashes controls.
        set page $itk_component(legend_page)
        # May have already been forgotten.
        catch {
            blt::table forget $page.symbol_l
            blt::table forget $page.symbol
            blt::table forget $page.dashes_l
            blt::table forget $page.dashes
        }
    }
}

itcl::body Rappture::XyPrint::SetOption { opt } {
    set new $_settings($this$opt)
    set old [$_clone cget $opt]
    set code [catch [list $_clone configure $opt $new] err]
    if { $code != 0 } {
        bell
        global errorInfo
        puts stderr "$err: $errorInfo"
        set _settings($this$opt) $old
        $_clone configure $opt $old
    }
}

itcl::body Rappture::XyPrint::SetComponentOption { comp opt } {
    set new $_settings($this-$comp$opt)
    set old [$_clone $comp cget $opt]
    set code [catch [list $_clone $comp configure $opt $new] err]
    if { $code != 0 } {
        bell
        global errorInfo
        puts stderr "$err: $errorInfo"
        set _settings($this-$comp$opt) $old
        $_clone $comp configure $opt $old
    }
}

itcl::body Rappture::XyPrint::SetNamedComponentOption { comp name opt } {
    set new $_settings($this-$comp$opt)
    set old [$_clone $comp cget $name $opt]
    set code [catch [list $_clone $comp configure $name $opt $new] err]
    if { $code != 0 } {
        bell
        global errorInfo
        puts stderr "$err: $errorInfo"
        set _settings($this-$comp$opt) $old
        $_clone $comp configure $name $opt $old
    }
}

itcl::body Rappture::XyPrint::RegeneratePreview {} {
    update
    set img [image create photo]
    set w [Inches2Pixels $_settings($this-layout-width) 3.4]
    set h [Inches2Pixels $_settings($this-layout-height) 3.4]
    $_clone snap $img -width $w -height $h

    set pixelsPerInch [winfo pixels . 1i]
    set cw [winfo width $itk_component(preview)]
    set ch [winfo height $itk_component(preview)]
    set rw [expr 2.5*$pixelsPerInch]
    set rh [expr 2.5*$pixelsPerInch]
    set maxwidth $rw
    set maxheight $rh
    if { $maxwidth > $cw } {
        set maxwidth $cw
    }
    if { $maxheight > $ch } {
        set maxheight $ch
    }
    set sx [expr double($maxwidth)/$w]
    set sy [expr double($maxheight)/$h]
    set s [expr min($sx,$sy)]

    set pw [expr int(round($s * $w))]
    set ph [expr int(round($s * $h))]
    $_preview configure -width $pw -height $ph
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

itcl::body Rappture::XyPrint::Inches2Pixels { inches {defValue ""}} {
    set n [scan $inches %g dummy]
    if { $n != 1  && $defValue != "" } {
        set inches $defValue
    }
    return  [winfo pixels . ${inches}i]
}

itcl::body Rappture::XyPrint::Color2RGB { color } {
    foreach { r g b } [winfo rgb $_clone $color] {
        set r [expr round($r / 257.0)]
        set g [expr round($g / 257.0)]
        set b [expr round($b / 257.0)]
    }
    return [format "\#%02x%02x%02x" $r $g $b]
}

itcl::body Rappture::XyPrint::GetAxisType { axis } {
    foreach type { x y x2 y2 } {
        set axes [$_clone ${type}axis use]
        if { [lsearch $axes $axis] >= 0 } {
            return [string range $type 0 0]
        }
    }
    return ""
}

itcl::body Rappture::XyPrint::GetAxis {} {
    set axis [$itk_component(axis_combo) current]
    foreach option { -min -max -loose -title -stepsize -subdivisions } {
        set _settings($this-axis$option) [$_clone axis cget $axis $option]
    }
    foreach attr { fontfamily fontsize fontweight fontslant } {
        set specific $this-$axis-ticks
        set general $this-axis-ticks
        set _settings(${general}-${attr}) $_settings(${specific}-${attr})
        set specific $this-$axis-title
        set general $this-axis-title
        set _settings(${general}-${attr}) $_settings(${specific}-${attr})
    }
    set type [GetAxisType $axis]
    if { [$_clone grid cget -map${type}] == $axis } {
        set _settings($this-axis-grid) 1
    }  else {
        set _settings($this-axis-grid) 0
    }
    set _settings($this-axis-plotpad${type}) \
        [Pixels2Inches [$_clone cget -plotpad${type}]]
    set _settings($this-axis-zero) [$_clone marker cget ${type}-zero -hide]
}

itcl::body Rappture::XyPrint::GetElement { args } {
    set index 1
    array unset _settings $this-element-*
    foreach elem [$_clone element show] {
        set _settings($this-element-$index) $elem
        incr index
    }
    set index [$itk_component(element_slider) get]
    set elem $_settings($this-element-$index)
    set _settings($this-element-label) [$_clone element cget $elem -label]
    set _settings($this-element-color) [$_clone element cget $elem -color]
    if { [$_clone element type $elem] != "bar" } {
        set _settings($this-element-symbol) [$_clone element cget $elem -symbol]
        set _settings($this-element-dashes) [$_clone element cget $elem -dashes]
    }
    set page $itk_component(legend_page)
    set color [$page.color label $_settings($this-element-color)]
    if { $color == "" } {
        set color [Color2RGB $_settings($this-element-color)]
        $page.color choices insert end $color $color
        $page.color value $color
    } else {
        $page.color value [$page.color label $_settings($this-element-color)]
    }
    if { [$_clone element type $elem] != "bar" } {
        $page.symbol value [$page.symbol label $_settings($this-element-symbol)]
        $page.dashes value [$page.dashes label $_settings($this-element-dashes)]
    }
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
    Rappture::Tooltip::for $page.width \
        "Set the width (inches) of the output image. Must be a positive number."

    label $page.height_l -text "height"
    entry $page.height -width 6 \
        -textvariable [itcl::scope _settings($this-layout-height)]
    bind  $page.height <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]
    Rappture::Tooltip::for $page.height \
        "Set the height (inches) of the output image. Must be a positive number"

    label $page.margin_l -text "Margins"

    label $page.left_l -text "left"
    entry $page.left -width 6 \
        -textvariable [itcl::scope _settings($this-layout-leftmargin)]
    bind  $page.left <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]
    Rappture::Tooltip::for $page.left \
        "Set the size (inches) of the left margin. If zero, the size is automatically determined."

    label $page.right_l -text "right"
    entry $page.right -width 6 \
        -textvariable [itcl::scope _settings($this-layout-rightmargin)]
    bind  $page.right <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]
    Rappture::Tooltip::for $page.right \
        "Set the size (inches) of the right margin. If zero, the size is automatically determined."


    label $page.top_l -text "top"
    entry $page.top -width 6 \
        -textvariable [itcl::scope _settings($this-layout-topmargin)]
    bind  $page.top <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]
    Rappture::Tooltip::for $page.top \
        "Set the size (inches) of the top margin. If zero, the size is automatically determined."

    label $page.bottom_l -text "bottom"
    entry $page.bottom -width 6 \
        -textvariable [itcl::scope _settings($this-layout-bottommargin)]
    bind  $page.bottom <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]
    Rappture::Tooltip::for $page.bottom \
        "Set the size (inches) of the bottom margin. If zero, the size is automatically determined."


    label $page.map -image [Rappture::icon graphmargins]
    blt::table $page \
        0,0 $page.width_l -anchor e \
        0,1 $page.width -fill x  \
        1,0 $page.height_l -anchor e \
        1,1 $page.height -fill x \
        3,0 $page.left_l -anchor e \
        3,1 $page.left -fill x  \
        4,0 $page.right_l -anchor e \
        4,1 $page.right -fill x \
        5,0 $page.top_l -anchor e \
        5,1 $page.top -fill x \
        6,0 $page.bottom_l -anchor e \
        6,1 $page.bottom -fill x  \
        0,2 $page.map -fill both -rspan 7 -padx 2

    blt::table configure $page c0 r* -resize none
    blt::table configure $page c1 r2 -resize both
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
        "pdf" "PDF Portable Document Format" \
        "ps"  "PS PostScript Format"  \
        "eps" "EPS Encapsulated PostScript"  \
        "jpg" "JPEG Joint Photographic Experts Group Format" \
        "png" "PNG Portable Network Graphics Format"

    bind $page.format <<Value>> [itcl::code $this ApplyGeneralSettings]
    Rappture::Tooltip::for $page.format \
        "Set the format of the image."

    label $page.style_l -text "style"
    Rappture::Combobox $page.style -width 20 -editable no
    $page.style choices insert end \
        "ieee" "IEEE Journal"  \
        "gekco" "G Klimeck"
    bind $page.style <<Value>> [itcl::code $this ApplyGeneralSettings]
    Rappture::Tooltip::for $page.format \
        "Set the style template."

    checkbutton $page.remember -text "remember settings" \
        -variable [itcl::scope _settings($this-general-remember)]
    Rappture::Tooltip::for $page.remember \
        "Remember the settings. They will be override the default settings."

    button $page.revert -text "revert" -image [Rappture::icon revert] \
        -compound left -padx 3 -pady 1 -overrelief raised -relief flat \
        -command [itcl::code $this ResetSettings]
    Rappture::Tooltip::for $page.revert \
        "Revert to the default settings."

    blt::table $page \
        2,0 $page.format_l -anchor e \
        2,1 $page.format -fill x -cspan 2 \
        3,0 $page.style_l -anchor e \
        3,1 $page.style -fill x -cspan 2 \
        5,0 $page.remember -cspan 2 -anchor w \
        5,2 $page.revert -anchor e
    blt::table configure $page r* -resize none  -pady { 0 2 }
    blt::table configure $page r4 -resize both

}

itcl::body Rappture::XyPrint::ApplyLegendSettings {} {
    set page $itk_component(legend_page)
    set _settings($this-legend-anchor)    [$page.anchor current]
    if { $_clone != "" } {
        lappend font $_settings($this-legend-fontfamily)
        lappend font $_settings($this-legend-fontsize)
        lappend font $_settings($this-legend-fontweight)
        lappend font $_settings($this-legend-fontslant)
        foreach option { -hide -position -anchor -borderwidth } {
            SetComponentOption legend $option
        }
        $_clone legend configure -font fixed -font $font
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
    Rappture::Tooltip::for $page.show \
        "Display the legend."

    label $page.position_l -text "position"
    Rappture::Combobox $page.position -width 15 -editable no
    $page.position choices insert end \
        "leftmargin" "left margin"  \
        "rightmargin" "right margin"  \
        "bottommargin" "bottom margin"  \
        "topmargin" "top margin"  \
        "plotarea" "inside plot"
    bind $page.position <<Value>> [itcl::code $this ApplyLegendSettings]
    Rappture::Tooltip::for $page.position \
        "Set the position of the legend.  This option and the anchor determine the legend's location."

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
    Rappture::Tooltip::for $page.anchor \
        "Set the anchor of the legend.  This option and the anchor determine the legend's location."

    checkbutton $page.border -text "border" \
        -variable [itcl::scope _settings($this-legend-borderwidth)] \
        -onvalue 1 -offvalue 0 \
        -command [itcl::code $this ApplyLegendSettings]
    Rappture::Tooltip::for $page.border \
        "Display a solid border around the legend."

    label $page.slider_l -text "legend\nentry"  -justify right
    itk_component add element_slider {
        ::scale $page.slider -from 1 -to 1 \
            -orient horizontal -width 12 \
            -command [itcl::code $this GetElement]
    }
    Rappture::Tooltip::for $page.slider \
        "Select the current entry."

    label $page.label_l -text "label"
    entry $page.label \
        -background white \
        -textvariable [itcl::scope _settings($this-element-label)]
    bind  $page.label <KeyPress-Return> [itcl::code $this ApplyElementSettings]
    Rappture::Tooltip::for $page.label \
        "Set the label of the current entry in the legend."

    label $page.color_l -text "color "
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
        "#cd9b9b" "rosy brown" \
        "#0000ff" "blue1" \
        "#ff0000" "red1" \
        "#00ff00" "green1"
    bind $page.color <<Value>> [itcl::code $this ApplyElementSettings]
    Rappture::Tooltip::for $page.color \
        "Set the color of the current entry."

    label $page.dashes_l -text "line style"
    Rappture::Combobox $page.dashes -width 15 -editable no
    $page.dashes choices insert end \
        "" "solid"  \
        "1" "dot" \
        "5 2" "dash" \
        "2 4 2" "dashdot" \
        "2 4 2 2" "dashdotdot"
    bind $page.dashes <<Value>> [itcl::code $this ApplyElementSettings]
    Rappture::Tooltip::for $page.dashes \
        "Set the line style of the current entry."

    label $page.symbol_l -text "symbol"
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
    Rappture::Tooltip::for $page.symbol \
        "Set the symbol of the current entry. \"none\" display no symbols."

    label $page.font_l -text "font"
    Rappture::Combobox $page.fontfamily -width 10 -editable no
    $page.fontfamily choices insert end \
        "courier" "courier" \
        "helvetica" "helvetica"  \
        "new*century*schoolbook"  "new century schoolbook" \
        "symbol"  "symbol" \
        "times"  "times"
    bind $page.fontfamily <<Value>> [itcl::code $this ApplyLegendSettings]
    Rappture::Tooltip::for $page.fontfamily \
        "Set the font of entries in the legend."

    Rappture::Combobox $page.fontsize -width 4 -editable no
    $page.fontsize choices insert end \
        "8" "8" \
        "9" "9" \
        "10" "10" \
        "11" "11" \
        "12" "12" \
        "14" "14" \
        "17" "17" \
        "18" "18" \
        "20" "20"
    bind  $page.fontsize <<Value>> [itcl::code $this ApplyLegendSettings]
    Rappture::Tooltip::for $page.fontsize \
        "Set the size (points) of the font."

    Rappture::PushButton $page.fontweight \
        -onimage [Rappture::icon font-bold] \
        -offimage [Rappture::icon font-bold] \
        -onvalue "bold" -offvalue "normal" \
        -command [itcl::code $this ApplyLegendSettings] \
        -variable [itcl::scope _settings($this-legend-fontweight)]
    Rappture::Tooltip::for $page.fontweight \
        "Use the bold version of the font."

    Rappture::PushButton $page.fontslant \
        -onimage [Rappture::icon font-italic] \
        -offimage [Rappture::icon font-italic] \
        -onvalue "italic" -offvalue "roman" \
        -command [itcl::code $this ApplyLegendSettings] \
        -variable [itcl::scope _settings($this-legend-fontslant)]
    Rappture::Tooltip::for $page.fontslant \
        "Use the italic version of the font."

    blt::table $page \
        1,0 $page.show -cspan 2 -anchor w \
        1,2 $page.border -cspan 2 -anchor w \
        2,0 $page.position_l -anchor e \
        2,1 $page.position -fill x \
        2,2 $page.anchor -fill x  -cspan 3 \
        3,0 $page.font_l -anchor e \
        3,1 $page.fontfamily -fill x \
        3,2 $page.fontsize -fill x \
        3,3 $page.fontweight -anchor e \
        3,4 $page.fontslant -anchor e \
        4,0 $page.slider_l -anchor e \
        4,1 $page.slider -fill x -cspan 5 \
        5,0 $page.label_l -anchor e \
        5,1 $page.label -fill x -cspan 5 \
        6,0 $page.color_l -anchor e \
        6,1 $page.color -fill x \
        6,2 $page.symbol_l -anchor e \
        6,3 $page.symbol -fill both -cspan 3 \
        7,0 $page.dashes_l -anchor e \
        7,1 $page.dashes -fill x \

    blt::table configure $page r* -resize none -pady { 0 2 }
    blt::table configure $page c3 c4 -resize none
    blt::table configure $page r8 -resize both

}

itcl::body Rappture::XyPrint::BuildAxisTab {} {
    itk_component add axis_page {
        frame $itk_component(tabs).axis_page
    }
    set page $itk_component(axis_page)
    $itk_component(tabs) insert end "axis" \
        -text "Axis" -padx 2 -pady 2 -window $page -fill both

    label $page.axis_l -text "axis"
    itk_component add axis_combo {
        Rappture::Combobox $page.axis -width 20 -editable no
    }
    bind $itk_component(axis_combo) <<Value>> [itcl::code $this GetAxis]
    Rappture::Tooltip::for $page.axis \
        "Select the current axis."

    label $page.title_l -text "title"
    entry $page.title \
        -textvariable [itcl::scope _settings($this-axis-title)]
    bind  $page.title <KeyPress-Return> [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.title \
        "Set the title of the current axis."

    label $page.min_l -text "min"
    entry $page.min -width 10 \
        -textvariable [itcl::scope _settings($this-axis-min)]
    bind  $page.min <KeyPress-Return> [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.min \
        "Set the minimum limit for the current axis. If empty, the minimum is automatically determined."

    label $page.max_l -text "max"
    entry $page.max -width 10 \
        -textvariable [itcl::scope _settings($this-axis-max)]
    bind  $page.max <KeyPress-Return> [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.max \
        "Set the maximum limit for the current axis. If empty, the maximum is automatically determined."

    label $page.subdivisions_l -text "subdivisions"
    entry $page.subdivisions \
        -textvariable [itcl::scope _settings($this-axis-subdivisions)]
    bind  $page.subdivisions <KeyPress-Return> \
        [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.subdivisions \
        "Set the number of subdivisions (minor ticks) for the current axis."

    label $page.stepsize_l -text "step size"
    entry $page.stepsize \
        -textvariable [itcl::scope _settings($this-axis-stepsize)]
    bind  $page.stepsize <KeyPress-Return> [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.stepsize \
        "Set the interval between major ticks for the current axis. If zero, the interval is automatically determined."

    checkbutton $page.loose -text "loose limits" \
        -onvalue "always" -offvalue "0" \
        -variable [itcl::scope _settings($this-axis-loose)] \
        -command [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.loose \
        "Set major ticks outside of the limits for the current axis."

    label $page.plotpad_l -text "pad"
    entry $page.plotpad -width 6 \
        -textvariable [itcl::scope _settings($this-axis-plotpad)]
    bind  $page.plotpad <KeyPress-Return> [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.plotpad \
        "Set padding (points) between the current axis and the plot."

    checkbutton $page.grid -text "show grid lines" \
        -variable [itcl::scope _settings($this-axis-grid)] \
        -command [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.grid \
        "Display grid lines for the current axis."

    checkbutton $page.zero -text "mark zero" \
        -offvalue 1 -onvalue 0 \
        -variable [itcl::scope _settings($this-axis-zero)] \
        -command [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.zero \
        "Display a line at zero for the current axis."

    label $page.tickfont_l -text "tick font"
    Rappture::Combobox $page.tickfontfamily -width 10 -editable no
    $page.tickfontfamily choices insert end \
        "courier" "courier" \
        "helvetica" "helvetica"  \
        "new*century*schoolbook"  "new century schoolbook" \
        "symbol"  "symbol" \
        "times"  "times"
    bind $page.tickfontfamily <<Value>> [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.tickfontfamily \
        "Set the font of the ticks for the current axis."

    Rappture::Combobox $page.tickfontsize -width 4 -editable no
    $page.tickfontsize choices insert end \
        "8" "8" \
        "9" "9" \
        "10" "10" \
        "11" "11" \
        "12" "12" \
        "14" "14" \
        "17" "17" \
        "18" "18" \
        "20" "20"
    bind $page.tickfontsize <<Value>> [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.tickfontsize \
        "Set the size (points) of the tick font."

    Rappture::PushButton $page.tickfontweight \
        -onimage [Rappture::icon font-bold] \
        -offimage [Rappture::icon font-bold] \
        -onvalue "bold" -offvalue "normal" \
        -command [itcl::code $this ApplyAxisSettings] \
        -variable [itcl::scope _settings($this-axis-ticks-fontweight)]
    Rappture::Tooltip::for $page.tickfontweight \
        "Use the bold version of the tick font."

    Rappture::PushButton $page.tickfontslant \
        -onimage [Rappture::icon font-italic] \
        -offimage [Rappture::icon font-italic] \
        -onvalue "italic" -offvalue "roman" \
        -command [itcl::code $this ApplyAxisSettings] \
        -variable [itcl::scope _settings($this-axis-ticks-fontslant)]
    Rappture::Tooltip::for $page.tickfontslant \
        "Use the italic version of the tick font."

    label $page.titlefont_l -text "title font"
    Rappture::Combobox $page.titlefontfamily -width 10 -editable no
    $page.titlefontfamily choices insert end \
        "courier" "courier" \
        "helvetica" "helvetica"  \
        "new*century*schoolbook"  "new century schoolbook" \
        "symbol"  "symbol" \
        "times"  "times"
    bind $page.titlefontfamily <<Value>> [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.titlefontfamily \
        "Set the font of the title for the current axis."

    Rappture::Combobox $page.titlefontsize -width 4 -editable no
    $page.titlefontsize choices insert end \
        "8" "8" \
        "9" "9" \
        "10" "10" \
        "11" "11" \
        "12" "12" \
        "14" "14" \
        "17" "17" \
        "18" "18" \
        "20" "20"
    bind $page.titlefontsize <<Value>> [itcl::code $this ApplyAxisSettings]
    Rappture::Tooltip::for $page.titlefontsize \
        "Set the size (point) of the title font."

    Rappture::PushButton $page.titlefontweight \
        -onimage [Rappture::icon font-bold] \
        -offimage [Rappture::icon font-bold] \
        -onvalue "bold" -offvalue "normal" \
        -command [itcl::code $this ApplyAxisSettings] \
        -variable [itcl::scope _settings($this-axis-title-fontweight)]
    Rappture::Tooltip::for $page.titlefontweight \
        "Use the bold version of the title font."

    Rappture::PushButton $page.titlefontslant \
        -onimage [Rappture::icon font-italic] \
        -offimage [Rappture::icon font-italic] \
        -onvalue "italic" -offvalue "roman" \
        -command [itcl::code $this ApplyAxisSettings] \
        -variable [itcl::scope _settings($this-axis-title-fontslant)]
    Rappture::Tooltip::for $page.titlefontslant \
        "Use the italic version of the title font."

    blt::table $page \
        1,1 $page.axis_l -anchor e  -pady 6 \
        1,2 $page.axis -fill x -cspan 6 \
        2,1 $page.title_l -anchor e \
        2,2 $page.title -fill x -cspan 5 \
        3,1 $page.min_l -anchor e \
        3,2 $page.min -fill x \
        3,3 $page.stepsize_l -anchor e \
        3,4 $page.stepsize -fill both -cspan 3 \
        4,1 $page.max_l -anchor e \
        4,2 $page.max -fill both \
        4,3 $page.subdivisions_l -anchor e \
        4,4 $page.subdivisions -fill both -cspan 3  \
        5,1 $page.titlefont_l -anchor e \
        5,2 $page.titlefontfamily -fill x -cspan 2 \
        5,4 $page.titlefontsize -fill x \
        5,5 $page.titlefontweight -anchor e \
        5,6 $page.titlefontslant -anchor e \
        6,1 $page.tickfont_l -anchor e \
        6,2 $page.tickfontfamily -fill x -cspan 2 \
        6,4 $page.tickfontsize -fill x \
        6,5 $page.tickfontweight -anchor e \
        6,6 $page.tickfontslant -anchor e \
        7,1 $page.loose -cspan 2 -anchor w \
        7,3 $page.grid -anchor w -cspan 2 \
        8,1 $page.zero -cspan 2 -anchor w \
        8,3 $page.plotpad_l -anchor e \
        8,4 $page.plotpad -fill both -cspan 3

    blt::table configure $page  c0 c4 c5 c6 c7 c8 -resize none
}

itcl::body Rappture::XyPrint::ApplyGeneralSettings {} {
    RegeneratePreview
}

itcl::body Rappture::XyPrint::ApplyLegendSettings {} {
    set page $itk_component(legend_page)
    set _settings($this-legend-position)  [$page.position current]
    set _settings($this-legend-anchor)    [$page.anchor current]

    foreach option { -hide -position -anchor -borderwidth } {
        SetComponentOption legend $option
    }
    set _settings($this-legend-fontfamily)  [$page.fontfamily value]
    set _settings($this-legend-fontsize)    [$page.fontsize value]
    lappend font $_settings($this-legend-fontfamily)
    lappend font $_settings($this-legend-fontsize)
    lappend font $_settings($this-legend-fontweight)
    lappend font $_settings($this-legend-fontslant)
    $_clone legend configure -font fixed -font $font
    ApplyElementSettings
}

itcl::body Rappture::XyPrint::ApplyAxisSettings {} {
    set axis [$itk_component(axis_combo) current]
    set type [GetAxisType $axis]
    set page $itk_component(axis_page)
    if { $_settings($this-axis-grid) } {
        $_clone grid configure -hide no -map${type} ${axis}
    } else {
        $_clone grid configure -hide no -map${type} ""
    }
    $_clone configure -plotpad${type} $_settings($this-axis-plotpad)
    foreach option { -min -max -loose -title -stepsize -subdivisions } {
        SetNamedComponentOption axis $axis $option
    }
    set _settings($this-axis-ticks-fontfamily)  [$page.tickfontfamily value]
    set _settings($this-axis-ticks-fontsize)    [$page.tickfontsize value]
    set _settings($this-axis-title-fontfamily)  [$page.titlefontfamily value]
    set _settings($this-axis-title-fontsize)    [$page.titlefontsize value]

    set tickfont {}
    set titlefont {}

    foreach attr { fontfamily fontsize fontweight fontslant } {
        set specific $this-$axis-ticks
        set general  $this-axis-ticks
        set _settings(${specific}-${attr}) $_settings(${general}-${attr})
        lappend tickfont $_settings(${general}-${attr})
        set specific $this-$axis-title
        set general  $this-axis-title
        set _settings(${specific}-${attr}) $_settings(${general}-${attr})
        lappend titlefont $_settings(${general}-${attr})
    }
    $_clone axis configure $axis -tickfont $tickfont -titlefont $titlefont
    $_clone marker configure ${type}-zero -hide $_settings($this-axis-zero)
    GetAxis
    RegeneratePreview
}

itcl::body Rappture::XyPrint::ApplyElementSettings {} {
    set index [$itk_component(element_slider) get]
    set page $itk_component(legend_page)
    set _settings($this-element-color)  [$page.color current]
    if { $_clone != "" } {
        set elem $_settings($this-element-$index)
        if { [$_clone element type $elem] != "bar" } {
            set _settings($this-element-symbol) [$page.symbol current]
            set _settings($this-element-dashes) [$page.dashes current]
            foreach option { -symbol -dashes } {
                SetNamedComponentOption element $elem $option
            }
        }
        foreach option { -color -label } {
            SetNamedComponentOption element $elem $option
        }
        RegeneratePreview
    }
}

itcl::body Rappture::XyPrint::SetLayoutOption { opt } {
    set new [Inches2Pixels $_settings($this-layout$opt)]
    $_clone configure $opt $new
}

itcl::body Rappture::XyPrint::ApplyLayoutSettings {} {
    foreach opt { -width -height -leftmargin -rightmargin -topmargin
        -bottommargin } {
        set old [$_clone cget $opt]
        set code [catch { SetLayoutOption $opt } err]
        if { $code != 0 } {
            bell
            global errorInfo
            puts stderr "$err: $errorInfo"
            set _settings($this-layout$opt) [Pixels2Inches $old]
            $_clone configure $opt [Pixels2Inches $old]
        }
    }
    RegeneratePreview
}


itcl::body Rappture::XyPrint::InitializeSettings {} {
    # General settings

    # Always set to "ps" "ieee"
    set _settings($this-general-format) ps
    set _settings($this-general-style) ieee
    set _settings($this-general-remember) 0
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

    $page.fontfamily value $_settings($this-legend-fontfamily)
    $page.fontsize value $_settings($this-legend-fontsize)
    if { $_settings($this-legend-fontweight) == "bold" } {
        set _settings($this-legend-font-bold) 1
    }
    set _settings($this-legend-hide) [$_clone legend cget -hide]
    set _settings($this-legend-position) [$_clone legend cget -position]
    set _settings($this-legend-anchor) [$_clone legend cget -anchor]
    $page.position value \
        [$page.position label $_settings($this-legend-position)]
    $page.anchor value [$page.anchor label $_settings($this-legend-anchor)]
    GetElement

    # Axis settings
    set page $itk_component(axis_page)
    set names [lsort [$_clone axis names]]
    $itk_component(axis_combo) choices delete 0 end
    foreach axis $names {
        if { $axis == "z" } {
            continue
        }
        if { ![$_clone axis cget $axis -hide] } {
            $itk_component(axis_combo) choices insert end $axis $axis
        }
        lappend axisnames $axis
    }
    set axis [lindex $names 0]

    $page.titlefontfamily value $_settings($this-$axis-title-fontfamily)
    $page.titlefontsize value   $_settings($this-$axis-title-fontsize)
    $page.tickfontfamily value  $_settings($this-$axis-ticks-fontfamily)
    $page.tickfontsize value    $_settings($this-$axis-ticks-fontsize)

    # Always hide the zero line.
    set _settings($this-axis-zero) 1
    set _settings($this-axis-plotpad) [Pixels2Inches [$_clone cget -plotpadx]]
    # Pick the first axis to initially display
    set axis [lindex $axisnames 0]
    $itk_component(axis_combo) value $axis
    GetAxis
    RegeneratePreview
}


itcl::body Rappture::XyPrint::restore { toolName plotName data } {
    set key [list $toolName $plotName]
    set _savedSettings($key) $data
}

itcl::body Rappture::XyPrint::RestoreSettings { toolName plotName } {
    if { ![file readable $_settingsFile] } {
        return;                         # No file or not readable
    }
    if { [file exists $_oldSettingsFile] } {
        file delete $_oldSettingsFile
    }
    # Read the file by sourcing it into a safe interpreter The only commands
    # executed will be "xyprint" which will simply add the data into an array
    # _savedSettings.
    set parser [interp create -safe]
    $parser alias xyprint [itcl::code $this restore]
    set f [open $_settingsFile "r"]
    set code [read $f]
    close $f
    if { [catch { $parser eval $code }] != 0 } {
        file delete $_settingsFile
    }
    # Now see if there's an entry for this tool/plot combination.  The data
    # associated with the variable is itself code to update the graph (clone).
    set key [list $toolName $plotName]
    if { [info exists _savedSettings($key)] }  {
        $parser alias "preview" $_clone
        $parser eval $_savedSettings($key)
    }
    # Restore settings to this instance
    foreach {name value} [$parser eval "array get settings"] {
        set _settings($this-$name) $value
    }
    interp delete $parser
}

itcl::body Rappture::XyPrint::ResetSettings {} {
    set graph $_graph
    DestroySettings
    set _graph $graph
    CloneGraph $graph
    InitClone
    InitializeSettings
}

itcl::body Rappture::XyPrint::SaveSettings { toolName plotName } {
    if { !$_settings($this-general-remember) } {
        return
    }
    if { [catch { open $_settingsFile "w" 0600 } f ] != 0 } {
        puts stderr "$_settingsFile isn't writable: $f"
        bell
        return
    }
    set key [list $toolName $plotName]
    set _savedSettings($key) [CreateSettings $toolName $plotName]
    # Write the settings out
    foreach key [lsort [array names _savedSettings]] {
        set tool [lindex $key 0]
        set plot [lindex $key 1]
        puts $f "xyprint \"$tool\" \"$plot\" \{"
        set settings [string trim $_savedSettings($key) \n]
        puts $f "$settings"
        puts $f "\}\n"
    }
    close $f
}

itcl::body Rappture::XyPrint::CreateSettings { toolName plotName } {
    # Create stanza associated with tool and plot title.
    # General settings
    set length [string length "${this}-"]
    append out "    array set settings {\n"
    foreach item [array names _settings ${this}-*] {
        set field [string range $item $length end]
        if { [regexp {^element-[0-9]+$} $field] } {
            continue
        }
        set value $_settings($item)
        append out "        [list $field] [list $value]\n"
    }
    append out "    }\n"
    # Legend font
    lappend legendfont $_settings($this-legend-fontfamily)
    lappend legendfont $_settings($this-legend-fontsize)
    lappend legendfont $_settings($this-legend-fontweight)
    lappend legendfont $_settings($this-legend-fontslant)
    # Axis tick font
    lappend axistickfont $_settings($this-axis-ticks-fontfamily)
    lappend axistickfont $_settings($this-axis-ticks-fontsize)
    lappend axistickfont $_settings($this-axis-ticks-fontweight)
    lappend axistickfont $_settings($this-axis-ticks-fontslant)
    # Axis title font
    lappend axistitlefont $_settings($this-axis-title-fontfamily)
    lappend axistitlefont $_settings($this-axis-title-fontsize)
    lappend axistitlefont $_settings($this-axis-title-fontweight)
    lappend axistitlefont $_settings($this-axis-title-fontslant)
    append out "\n"

    # Layout settings
    append out "    preview configure"
    foreach opt { -width -height -leftmargin -rightmargin -topmargin
        -bottommargin -plotpadx -plotpady } {
        set value [$_clone cget $opt]
        append out " $opt [list $value]"
    }
    append out "\n"

    # Legend settings
    append out "    preview legend configure"
    foreach opt { -position -anchor -borderwidth -hide } {
        set value [$_clone legend cget $opt]
        append out " $opt [list $value]"
    }
    append out " -font [list $legendfont]\n"

    # Element settings
    foreach elem [$_clone element show] {
        set label [$_clone element cget $elem -label]
        if { $label == "" } {
            continue
        }
        set label [list $label]
        append out "    if \{ \[preview element exists $label] \} \{\n"
        append out "        preview element configure $label"
        if { [$_clone element type $elem] != "bar" } {
            set options { -symbol -color -dashes -label }
        } else {
            set options { -color -label }
        }
        foreach opt $options {
            set value [$_clone element cget $elem $opt]
            append out " $opt [list $value]"
        }
        append out "    \}\n"
    }

    # Axis settings
    foreach axis [$_clone axis names] {
        if { [$_clone axis cget $axis -hide] } {
            continue
        }
        set axis [list $axis]
        append out "    if \{ \[llength \[preview axis names $axis\]\] == 1 \} \{\n"
        append out "        preview axis configure $axis"
        foreach opt { -hide -min -max -loose -title -stepsize -subdivisions } {
            set value [$_clone axis cget $axis $opt]
            append out " $opt [list $value]"
        }
        append out " -tickfont [list $axistickfont]"
        append out " -titlefont [list $axistitlefont]\n"
        set hide [$_clone marker cget ${axis}-zero -hide]
        append out "        preview marker configure ${axis}-zero -hide $hide\n"
        append out "    \}\n"
    }

    append out "    preview grid configure"
    append out " -hide \"[$_clone grid cget -hide]\""
    append out " -mapx \"[$_clone grid cget -mapx]\""
    append out " -mapy \"[$_clone grid cget -mapy]\""
    return $out
}
