# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: DrawingEntry - widget for entering numeric values
#
#  This widget represents a <number> entry on a control panel.
#  It is used to enter numeric values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require Img

itcl::class Rappture::DrawingEntry {
    inherit itk::Widget
    itk_option define -state state State "normal"

    private variable _dispatcher ""
    private variable _path
    private variable _owner
    private variable _monitoring ""
    private variable _xmlobj ""

    # slave interpreter where all substituted variables are stored
    private variable _parser ""

    # unique counter for popup names
    private common _popupnum 0

    private variable _canvasHeight 0
    private variable _canvasWidth 0
    private variable _cpath2popup
    private variable _takedown ""
    private variable _cname2id
    private variable _cname2image
    private variable _name2path
    private variable _name2map
    private variable _drawingHeight 0
    private variable _drawingWidth 0
    private variable _showing ""
    private variable _xAspect 0
    private variable _xMin 0
    private variable _xOffset 0
    private variable _xScale 1.0
    private variable _yAspect 0
    private variable _yMin 0
    private variable _yOffset 0
    private variable _yScale 1.0
    private variable _cursor ""

    constructor {owner path args} {
        # defined below
    }
    destructor {
        # defined below
    }
    public method value { args }
    public method label {}
    public method tooltip {}

    private method Activate { tag }
    private method AdjustDrawingArea { xAspect yAspect }
    private method Deactivate { tag }
    private method Highlight { tag }
    private method Invoke { name x y }
    private method ParseBackground {}
    private method ParseDescription {}
    private method ParseGrid { cpath cname }
    private method ParseHotspot { cpath cname }
    private method ParseLine { cpath cname }
    private method ParseOval { cpath cname }
    private method ParsePicture { cpath cname }
    private method ParsePolygon { cpath cname }
    private method ParseRectangle { cpath cname }
    private method ParseScreenCoordinates { values }
    private method ParseText { cpath cname }
    private method Redraw {}
    private method ScreenCoords { coords }
    private method ScreenX { x }
    private method ScreenY { y }
    private method UpdateSubstitutions {}
    private method XmlGet { path }
    private method XmlGetSubst { path }
    private method Hotspot { option cname item args }
}

itk::usual DrawingEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _path $path
    set _owner $owner
    set _xmlobj [$_owner xml object]

    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw "[itcl::code $this Redraw]; list"

    #
    # Display the current drawing.
    #
    itk_component add drawing {
        canvas $itk_interior.canvas -background white -relief sunken -bd 1 \
            -width 400 -height 300
    } {
        ignore -background
    }
    pack $itk_component(drawing) -expand yes -fill both
    bind $itk_component(drawing) <Configure> \
        [itcl::code $_dispatcher event -idle !redraw]

    # scan through all variables and attach notifications for changes
    set subs [$_xmlobj children -as path -type variable $_path.substitutions]
    foreach cpath $subs {
        set map ""
        set name ""
        set path ""
        foreach elem [$_xmlobj children $cpath] {
            switch -glob -- $elem {
                "name*" {
                    set name [XmlGet $cpath.$elem]
                }
                "path*" {
                    set path [XmlGet $cpath.$elem]
                }
                "map*" {
                    set from [XmlGet $cpath.$elem.from]
                    set to [XmlGet $cpath.$elem.to]
                    lappend map $from $to
                }
            }
        }
        if {$name eq ""} {
            puts stderr "no name defined for substitution variable \"$cpath\""
            continue
        }
        if {[info exists _name2path($name)]} {
            puts stderr "substitution variable \"$name\" already defined"
            continue
        }
        set _name2path($name) $path
        if {$path eq ""} {
            puts stderr "no path defined for substitution variable \"$cpath\""
            continue
        }
        set _name2map($name) $map

        # keep track of controls built for each variable (see below)
        set controls($path) unused

        # whenever variable changes, update drawing to report new values
        if {[lsearch $_monitoring $path] < 0} {
            $_owner notify add $this $path \
                [itcl::code $_dispatcher event -idle !redraw]
            lappend _monitoring $path
        }
    }

    # find all embedded controls and build a popup for each hotspot
    set hotspots [$_xmlobj children -type hotspot -as path $_path.components]
    foreach cpath $hotspots {
        set listOfControls [$_xmlobj children -type controls $cpath]
        if {[llength $listOfControls] > 0} {
            set popup .drawingentrypopup[incr _popupnum]
            Rappture::Balloon $popup -title "Change values..."
            set inner [$popup component inner]
            Rappture::Controls $inner.controls $_owner
            pack $inner.controls -fill both -expand yes
            set _cpath2popup($cpath) $popup

            # Add control widgets to this popup.
            # NOTE: if the widget exists elsewhere, it is deleted at this
            #   point and "sucked in" to the popup.
            foreach cname $listOfControls {
                set cntlpath [XmlGetSubst $cpath.$cname]
                $inner.controls insert end $cntlpath
            }
        }
    }
    set c $itk_component(drawing)
    set texts [$_xmlobj children -type text -as path $_path.components]
    foreach cpath $texts {
        set popup ""
        set mode [XmlGetSubst $cpath.hotspot]
        if {$mode eq "off"} {
            # no popup if hotspot is turned off
            continue
        }

        # easiest way to parse embedded variables is to create a hotspot item
        set id [$c create hotspot 0 0 -text [XmlGet $cpath.text]]
        foreach varName [Rappture::hotspot variables $c $id] {
            if {[info exists _name2path($varName)]} {
                set cntlpath $_name2path($varName)

                if {$controls($cntlpath) ne "unused"} {
                    puts stderr "WARNING: drawing variable \"$varName\" is used in two hotspots, but will appear in only one of them."
                    continue
                }
                set controls($cntlpath) "--"

                if {$popup eq ""} {
                    # create the popup for this item, if we haven't already
                    set popup .drawingentrypopup[incr _popupnum]
                    Rappture::Balloon $popup -title "Change values..."
                    set inner [$popup component inner]
                    Rappture::Controls $inner.controls $_owner
                    pack $inner.controls -fill both -expand yes
                }

                # Add the control widget for this variable to this popup.
                # NOTE: if the widget exists elsewhere, it is deleted at this
                #   point and "sucked in" to the popup.
                set inner [$popup component inner]
                $inner.controls insert end $cntlpath
                set _cpath2popup($cntlpath) $popup
            } else {
                puts stderr "unknown variable \"$varName\" in drawing item at $cpath"
            }
        }
        $c delete $id
    }

    # create a parser to manage substitions of variable values
    set _parser [interp create -safe]

    eval itk_initialize $args

    # initialize the drawing at some point
    $_dispatcher event -idle !redraw
}

itcl::body Rappture::DrawingEntry::destructor {} {
    # stop monitoring controls for value changes
    foreach cpath $_monitoring {
        $_owner notify remove $this $cpath
    }

    # tear down the value subsitution parser
    if {$_parser != ""} {
        $_parser delete
    }
}
# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingEntry::label {} {
    set label [string trim [$_owner xml get $_path.about.label]]
    if {$label eq ""} {
        set label "Drawing"
    }
    return $label
}

# ----------------------------------------------------------------------
# USAGE: tooltip
#
# Clients use this to query the tooltip associated with this widget.
# Reaches into the XML and pulls out the appropriate description
# string.  Returns the string that should be used with the
# Rappture::Tooltip facility.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingEntry::tooltip {} {
    set str [$_xmlobj get $_path.about.description]
    return [string trim $str]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::DrawingEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
}

itcl::body Rappture::DrawingEntry::Redraw {} {
    # If a popup is pending, redraw signals a value change; take it down
    if {$_takedown ne ""} {
        $_takedown deactivate
        set _takedown ""
    }

    # Remove exists canvas items and hints
    $itk_component(drawing) delete all

    # Delete any images that we created.
    foreach name [array names _cname2image] {
        image delete $_cname2image($name)
    }
    array unset _cname2id
    array unset _cname2image

    # Recompute the size of the canvas/drawing area
    set _canvasWidth [winfo width $itk_component(drawing)]
    if { $_canvasWidth < 2 } {
        set _canvasWidth [winfo reqwidth $itk_component(drawing)]
    }
    set _canvasHeight [winfo height $itk_component(drawing)]
    if { $_canvasHeight < 2 } {
        set _canvasHeight [winfo reqheight $itk_component(drawing)]
    }
    set _drawingWidth $_canvasWidth
    set _drawingHeight $_canvasHeight
    set _xOffset 0
    set _yOffset 0
    # By default, the screen coordinates are 0 to 1
    ParseScreenCoordinates "0 0 1 1"
    ParseDescription
}

#
# ParseDescription --
#
itcl::body Rappture::DrawingEntry::ParseDescription {} {
    ParseBackground
    UpdateSubstitutions
    foreach cname [$_xmlobj children $_path.components] {
        switch -glob -- $cname {
            "line*" {
                ParseLine $_path.components.$cname $cname
            }
            "grid*" {
                ParseGrid $_path.components.$cname $cname
            }
            "text*" {
                ParseText $_path.components.$cname $cname
            }
            "picture*" {
                ParsePicture $_path.components.$cname $cname
            }
            "rectangle*" {
                ParseRectangle $_path.components.$cname $cname
            }
            "oval*" {
                ParseOval $_path.components.$cname $cname
            }
            "polygon*" {
                ParsePolygon $_path.components.$cname $cname
            }
            "hotspot*" {
                ParseHotspot $_path.components.$cname $cname
            }
        }
    }
}

#
# ParseGrid --
#
itcl::body Rappture::DrawingEntry::ParseGrid { cpath cname } {
    array set attr2option {
        "linewidth"     "-width"
        "arrow"         "-arrow"
        "dash"          "-dash"
        "color"         "-fill"
    }
    # Set default options first and then let tool.xml override them.
    array set options {
        -arrow          none
        -width          0
        -fill           black
        -dash           ""
    }
    # Coords
    set xcoords [XmlGetSubst $cpath.xcoords]
    set ycoords [XmlGetSubst $cpath.ycoords]
    if { $ycoords == "" } {
        set ycoords "0 1"
        set ymax 1
        set ymin 0
    } else {
        set list {}
        set ymax -10000
        set ymin 10000
        foreach c $ycoords {
            set y [ScreenY $c]
            if { $y > $ymax } {
                set ymax $y
            }
            if { $y < $ymin } {
                set ymin $y
            }
            lappend list $y
        }
        set ycoords $list
    }
    if { $xcoords == "" } {
        set xcoords "0 1"
        set xmax 1
        set xmin 0
    } else {
        set list {}
        set xmax -10000
        set xmin 10000
        foreach c $xcoords {
            set x [ScreenX $c]
            if { $x > $xmax } {
                set xmax $x
            }
            if { $x < $xmin } {
                set xmin $x
            }
            lappend list $x
        }
        set xcoords $list
    }

    set list {}
    foreach attr [$_xmlobj children $cpath] {
        if { [info exists attr2option($attr)] } {
            set option $attr2option($attr)
            set value [XmlGetSubst $cpath.$attr]
            set options($option) $value
        }
    }
    set options(-tags) $cname
    foreach y $ycoords {
        lappend ids \
            [eval $itk_component(drawing) create line $xmin $y $xmax $y \
                 [array get options]]
    }
    foreach x $xcoords {
        lappend ids \
            [eval $itk_component(drawing) create line $x $ymin $x $ymax \
                 [array get options]]
    }
    set _cname2id($cname) $ids
}

#
# ParseHotspot --
#
itcl::body Rappture::DrawingEntry::ParseHotspot { cpath cname } {
    array set attr2option {
        "color"  "-fill"
        "anchor" "-anchor"
    }

    # Set default options first and then let tool.xml override them.
    array set options {
        -fill red
        -anchor c
    }
    foreach attr [$_xmlobj children $cpath] {
        if { [info exists attr2option($attr)] } {
            set option $attr2option($attr)
            set value [XmlGetSubst $cpath.$attr]
            set options($option) $value
        }
    }
    # Coordinates
    set coords [XmlGetSubst $cpath.coords]
    if {$coords eq ""} {
        set coords "0 0 1 1"
    }
    set c $itk_component(drawing)
    foreach {x1 y1} [ScreenCoords $coords] break
    set id [$itk_component(drawing) create image $x1 $y1]
    array unset options -fill
    set options(-tags) $cname
    set options(-image) [Rappture::icon hotspot_normal]
    eval $c itemconfigure $id [array get options]
    set _cname2id($cname) $id
    $c bind $id <Enter> [itcl::code $this Activate $cname]
    $c bind $id <Leave> [itcl::code $this Deactivate $cname]
    set bbox [$c bbox $id]
    set y1 [lindex $bbox 1]
    $c bind $id <ButtonPress-1> [itcl::code $this Invoke $cpath $x1 $y1]
}

#
# ParseLine --
#
itcl::body Rappture::DrawingEntry::ParseLine { cpath cname } {
    array set attr2option {
        "linewidth"     "-width"
        "arrow"         "-arrow"
        "dash"          "-dash"
        "color"         "-fill"
    }
    # Set default options first and then let tool.xml override them.
    array set options {
        -arrow          none
        -width          0
        -fill           black
        -dash           ""
    }
    # Coords
    set coords [XmlGetSubst $cpath.coords]
    if {$coords eq ""} {
        set coords "0 0"
    }
    set coords [ScreenCoords $coords]

    set list {}
    foreach attr [$_xmlobj children $cpath] {
        if { [info exists attr2option($attr)] } {
            set option $attr2option($attr)
            set value [XmlGetSubst $cpath.$attr]
            set options($option) $value
        }
    }
    set options(-tags) $cname
    set id [eval $itk_component(drawing) create line $coords]
    set _cname2id($cname) $id
    eval $itk_component(drawing) itemconfigure $id [array get options]
}

#
# ParseOval --
#
itcl::body Rappture::DrawingEntry::ParseOval { cpath cname } {
    array set attr2option {
        "outline"       "-outline"
        "fill"          "-fill"
        "linewidth"     "-width"
    }

    # Set default options first and then let tool.xml override them.
    array set options {
        -fill blue
        -width 1
        -outline black
    }
    foreach attr [$_xmlobj children $cpath] {
        if { [info exists attr2option($attr)] } {
            set option $attr2option($attr)
            set value [XmlGetSubst $cpath.$attr]
            set options($option) $value
        }
    }
    # Coordinates
    set coords [XmlGetSubst $cpath.coords]
    if {$coords eq ""} {
        set coords "0 0 1 1"
    }
    foreach { x1 y1 x2 y2 } [ScreenCoords $coords] break
    set id [$itk_component(drawing) create oval $x1 $y1 $x2 $y2]
    set _cname2id($cname) $id
    eval $itk_component(drawing) itemconfigure $id [array get options]
}

#
# ParsePicture --
#
itcl::body Rappture::DrawingEntry::ParsePicture { cpath cname } {
    array set attr2option {
        "anchor"        "-anchor"
    }

    # Set default options first and then let tool.xml override them.
    array set options {
        -anchor nw
    }
    foreach attr [$_xmlobj children $cpath] {
        if { [info exists attr2option($attr)] } {
            set option $attr2option($attr)
            set value [XmlGetSubst $cpath.$attr]
            set options($option) $value
        }
    }
    set contents [string trim [XmlGetSubst $cpath.contents]]
    set img ""
    if { [string compare -length 7 $contents "file://"] == 0 } {
        set fileName [string range $contents 7 end]
        set path $fileName
        # find the file on a search path
        if { [file pathtype $path] != "absolute" } {
            set dir [[$_owner tool] installdir]
            set searchlist [list $dir [file join $dir docs]]
            foreach dir $searchlist {
                if {[file readable [file join $dir $fileName]]} {
                    set path [file join $dir $fileName]
                    break
                }
            }
        }
        if { [file exists $path] } {
            set img [image create photo -file $path]
        } else {
            puts stderr "WARNING: can't find picture contents \"$path\""
        }
    } elseif { [string compare -length 7 $contents "http://"] == 0 } {
        puts stderr  "don't know how to handle http"
        return
    } elseif { $contents != "" } {
        set img [image create photo -data $contents]
    }
    if {$img == ""} {
        return
    }
    # Coordinates
    set coords [XmlGetSubst $cpath.coords]
    if { [llength $coords] == 2 } {
        foreach { x1 y1 } [ScreenCoords $coords] break
        set w [XmlGetSubst $cpath.width]
        if { $w == "" || ![string is double $w] || $w <= 0.0 } {
            set width [expr [image width $img] / 4]
        } else {
            set width [expr int([ScreenX $w] - [ScreenX 0])]
        }
        set h [XmlGetSubst $cpath.height]
        if { $h == "" || ![string is double $h] || $h <= 0.0 } {
            set height [expr [image height $img] / 4]
        } else {
            set height [expr int([ScreenY $h] - [ScreenY 0])]
        }
        if { $width != [image width $img] || $height != [image height $img] } {
            set dst [image create photo -width $width -height $height]
            blt::winop resample $img $dst box
            image delete $img
            set img $dst
        }
    } elseif { [llength $coords] == 4 } {
        foreach { x1 y1 x2 y2 } [ScreenCoords $coords] break
        if { $x1 > $x2 } {
            set tmp $x1
            set x1 $x2
            set x2 $tmp
        }
        if { $y1 > $y2 } {
            set tmp $x1
            set x1 $x2
            set x2 $tmp
        }
        set width [expr {int($x2 - $x1 + 1)}]
        set height [expr {int($y2 - $y1 + 1)}]
        if { $width != [image width $img] || $height != [image height $img] } {
            set dst [image create photo -width $width -height $height]
            blt::winop resample $img $dst box
            image delete $img
            set img $dst
        }
    } else {
        set width [expr [image width $img] / 4]
        set height [expr [image height $img] / 4]
         set dst [image create photo -width $width -height $height]
        blt::winop resample $img $dst box
        image delete $img
        set img $dst
        set x1 0
        set y1 0
    }
    set options(-tags) $cname
    set options(-image) $img
    set id [$itk_component(drawing) create image $x1 $y1]
    set _cname2image($cname) $img
    set _cname2id($cname) $id
    eval $itk_component(drawing) itemconfigure $id [array get options]
}


itcl::body Rappture::DrawingEntry::ParsePolygon { cpath cname } {
    array set attr2option {
        "outline"       "-outline"
        "color"         "-fill"
        "fill"          "-fill"
        "linewidth"     "-width"
    }
    # Set default options first and then let tool.xml override them.
    array set options {
        -width          1
        -fill           blue
        -outline        black
    }

    # Coords
    set coords [XmlGetSubst $cpath.coords]
    if {$coords eq ""} {
        set coords "0 0"
    }
    set coords [ScreenCoords $coords]

    set list {}
    foreach attr [$_xmlobj children $cpath] {
        if { [info exists attr2option($attr)] } {
            set option $attr2option($attr)
            set value [XmlGetSubst $cpath.$attr]
            set options($option) $value
        }
    }
    set options(-tags) $cname
    set id [eval $itk_component(drawing) create polygon $coords]
    set _cname2id($cname) $id
    eval $itk_component(drawing) itemconfigure $id [array get options]
}

#
# ParseRectangle --
#
itcl::body Rappture::DrawingEntry::ParseRectangle { cpath cname } {
    array set attr2option {
        "outline"       "-outline"
        "fill"          "-fill"
        "linewidth"     "-width"
    }

    # Set default options first and then let tool.xml override them.
    array set options {
        -fill blue
        -width 1
        -outline black
    }
    foreach attr [$_xmlobj children $cpath] {
        if { [info exists attr2option($attr)] } {
            set option $attr2option($attr)
            set value [XmlGetSubst $cpath.$attr]
            set options($option) $value
        }
    }
    # Coordinates
    set coords [XmlGetSubst $cpath.coords]
    if {$coords eq ""} {
        set coords "0 0 1 1"
    }
    foreach { x1 y1 x2 y2 } [ScreenCoords $coords] break
    set id [$itk_component(drawing) create rectangle $x1 $y1 $x2 $y2]
    set _cname2id($cname) $id
    eval $itk_component(drawing) itemconfigure $id [array get options]
}

#
# ParseText --
#
itcl::body Rappture::DrawingEntry::ParseText { cpath cname } {
    array set attr2option {
        "font"          "-font"
        "color"         "-foreground"
        "valuecolor"    "-valueforeground"
        "fill"          "-background"
        "text"          "-text"
        "anchor"        "-anchor"
    }

    # Set default options first and then let tool.xml override them.
    array set options {
        -font {Arial -14}
        -valuefont {Arial -14}
        -valueforeground blue3
        -text {}
        -fill {}
        -anchor c
    }
    foreach attr [$_xmlobj children $cpath] {
        if { [info exists attr2option($attr)] } {
            set option $attr2option($attr)
            if { $attr == "text" } {
                set value [XmlGet $cpath.$attr]
            } else {
                set value [XmlGetSubst $cpath.$attr]
            }
            set options($option) $value
        }
    }
    # Coords
    set coords [XmlGetSubst $cpath.coords]
    if {$coords eq ""} {
        set coords "0 0"
    }
    foreach {x0 y0} [ScreenCoords $coords] break

    set hotspot [XmlGetSubst $cpath.hotspot]
    if {$hotspot eq ""} {
        # assume inline by default
        set hotspot "inline"
    } elseif {[lsearch {inline off} $hotspot] < 0} {
        puts stderr "WARNING: bad hotspot value \"$hotspot\": should be inline or off"
    }

    if {$hotspot eq "inline"} {
        set options(-showicons) 1
    }
    set c $itk_component(drawing)
    set options(-tags) $cname
    set options(-image) [Rappture::icon hotspot_normal]
    set options(-activeimage) [Rappture::icon hotspot_active]
    set id [$c create hotspot $x0 $y0]
    set _cname2id($cname) $id
    set options(-interp) $_parser
    eval $c itemconfigure $id [array get options]

    if {$hotspot eq "inline"} {
        $c bind $id <Enter> \
            [itcl::code $this Hotspot activate $cname $id %x %y]
        $c bind $id <Motion> \
            [itcl::code $this Hotspot activate $cname $id %x %y]
        $c bind $id <Leave> \
            [itcl::code $this Hotspot deactivate $cname $id]
        $c bind $id <ButtonRelease-1> \
            [itcl::code $this Hotspot invoke $cname $id %x %y]
    }
}


itcl::body Rappture::DrawingEntry::Hotspot { option cname item args } {
    set c $itk_component(drawing)

    # see what variable (if any) that we're touching within the text
    set varName ""
    if {[llength $args] >= 2} {
        foreach {x y} $args break
        foreach {varName x0 y0 x1 y1} [Rappture::hotspot identify $c $item $x $y] break
    }

    switch -- $option {
        activate {
            if {$varName ne ""} {
                set active [$c itemcget $item -activevalue]
                if {$varName ne $active} {
                    $c itemconfigure $item -activevalue $varName
                }
                $itk_component(drawing) configure -cursor center_ptr

                # put up a tooltip for this item
                set cpath $_name2path($varName)
                set tip [XmlGet $cpath.about.description]
                if {$tip ne ""} {
                    set x [expr {[winfo rootx $c]+$x0+10}]
                    set y [expr {[winfo rooty $c]+$y1}]
                    set tag "$c-[string map {. ""} $cpath]"
                    Rappture::Tooltip::text $tag $tip -log $cpath
                    Rappture::Tooltip::tooltip pending $tag @$x,$y
                }
            } else {
                $c itemconfigure $item -activevalue ""
                $itk_component(drawing) configure -cursor ""
                Rappture::Tooltip::tooltip cancel
            }
        }
        deactivate {
            $c itemconfigure $item -activevalue ""
            $itk_component(drawing) configure -cursor ""
            Rappture::Tooltip::tooltip cancel
        }
        invoke {
            if {$varName ne ""} {
                set x [expr {($x0+$x1)/2}]
                Invoke $_name2path($varName) $x $y0
            }
        }
        default {
            error "bad option \"$option\": should be activate, deactivate, invoke"
        }
    }
}


itcl::body Rappture::DrawingEntry::ScreenX { x } {
    return [expr {($x - $_xMin)*$_xScale + $_xOffset}]
}

itcl::body Rappture::DrawingEntry::ScreenY { y } {
    return [expr {($y - $_yMin)*$_yScale + $_yOffset}]
}

itcl::body Rappture::DrawingEntry::ScreenCoords { coords } {
    set list {}
    foreach {x y} $coords {
        lappend list [ScreenX $x] [ScreenY $y]
    }
    return $list
}

itcl::body Rappture::DrawingEntry::AdjustDrawingArea { xAspect yAspect } {
    set _drawingWidth $_canvasWidth
    set _drawingHeight $_canvasHeight
    if { $xAspect <= 0 || $yAspect <= 0 } {
        return
    }
    set current [expr double($_canvasWidth) / double($_canvasHeight)]
    set wanted [expr double($xAspect) / double($yAspect)]
    if { $current > $wanted } {
        set sw [ expr int($_canvasWidth * $wanted)]
        if { $sw < 1 } {
            set sw 1
        }
        set _xOffset [expr $_canvasWidth - $sw]
        set _drawingWidth $sw
    } else {
        set sh [expr int($_canvasHeight / $wanted)]
        if { $sh < 1 }  {
            set sh 1
        }
        set _xOffset [expr $_canvasHeight - $sh]
        set _drawingHeight $sh
    }
}

#
#      <background>
#       <!-- background color of the drawing canvas (default white) -->
#       <color>black</color>
#       <!-- coordinate system:  x0 y0 ?at screenx screeny? x1 y1
#                                ?at screenx screeny?
#            The screenx/screeny values are optional, so you can also say
#            something like "-.1 0 1.1 1" as you had in your example.
#            This lets you put the origin at a specific point on screen,
#            and also define the directions of the axes.  We still compute
#            the overall bounding box.  In the example below, the bounding
#            box goes from -1,1 in the upper-left corner to 1,-1 in the
#            lower right.
#       -->
#       <coordinates>0 0 at 50% 50% 1 1 at 100% 100%</coordinates>

#       <!-- aspect ratio:  scales coordinate system so that pixels may not
#            be square.  A coordinate system like the one above implies a
#            square drawing area, since x and y both go from -1 to 1.  But
#            if you set the aspect to 2:1, you'll get something twice as
#            wide as it is tall.  This effectively says that x goes from
#            -1 to 1 in a certain distance, but y goes from -1 to 1 in half
#            that screen distance.  Default is whatever aspect is defined
#            by the coordinates.  If x goes 0-3 and y goes 0-1, then the
#            drawing (without any other aspect ratio) would be 3x wide and
#            1x tall.  The aspect ratio could be used to force it to be
#            square instead by setting "1 1" instead.  In that case, x goes
#            0-3 over the width, and y goes 0-1 over the same screen distance
#            along the height.
#       -->
#       <aspect>2 1</aspect>
#     </background>
#

itcl::body Rappture::DrawingEntry::ParseScreenCoordinates { values } {
    set bad ""
    foreach point {1 2} {
        set xvals($point) [lindex $values 0]
        if {![string is double -strict $xvals($point)]} {
            set bad "missing background coordinate point $point in \"$values\""
            break
        }

        set yvals($point) [lindex $values 1]
        if {![string is double -strict $yvals($point)]} {
            set bad "missing background coordinate point $point in \"$values\""
            break
        }
        set values [lrange $values 2 end]

        # each corner point can be place anywhere from 0% to 100%
        if {[lindex $values 0] eq "at"} {
            if {[regexp {^([0-9]+)%$} [lindex $values 1] match xpcnt]
              && [regexp {^([0-9]+)%$} [lindex $values 2] match ypcnt]} {
                set wherex($point) [expr {0.01*$xpcnt}]
                set wherey($point) [expr {0.01*$ypcnt}]
                set values [lrange $values 3 end]
            } else {
                set bad "expected \"at XX% YY%\" but got \"$values\""; break
            }
        } else {
            set wherex($point) [expr {($point == 1) ? 0 : 1}]
            set wherey($point) [expr {($point == 1) ? 0 : 1}]
        }
    }
    if {$bad eq "" && $wherex(1) == $wherex(2)} {
        set bad [format "drawing background limits have x points both at %d%%" [expr {round($wherex(1)*100)}]]
    }
    if {$bad eq "" && $wherey(1) == $wherey(2)} {
        set bad [format "drawing background limits have y points both at %d%%" [expr {round($wherex(1)*100)}]]
    }

    if {$bad eq "" && $xvals(1) == $xvals(2)} {
        set bad "drawing background coordinates have 0 range in x"
    }
    if {$bad eq "" && $yvals(1) == $yvals(2)} {
        set bad "drawing background coordinates have 0 range in y"
    }
    if {$bad eq "" && [llength $values] > 0} {
        set bad "extra coordinates \"$values\""
    }

    if {$bad ne ""} {
        puts stderr "WARNING: $bad"
        puts stderr "assuming default \"0 0 1 1\" coordinates"
        array set xvals {1 0 2 1}
        array set yvals {1 0 2 1}
        array set wherex {1 0 2 1}
        array set wherey {1 0 2 1}
    }

    # compute min/scale for each axis based on the input values
    if {$wherex(1) < $wherex(2)} {
        set min 1; set max 2
    } else {
        set min 2; set max 1
    }

    set slope [expr {double($xvals($max)-$xvals($min))
                      / ($wherex($max)-$wherex($min))}]
    set _xMin [expr {-$wherex($min)*$slope + $xvals($min)}]
    set xmax [expr {(1-$wherex($max))*$slope + $xvals($max)}]
    set _xScale [expr {[winfo width $itk_component(drawing)]/($xmax-$_xMin)}]

    if {$wherey(1) < $wherey(2)} {
        set min 1; set max 2
    } else {
        set min 2; set max 1
    }

    set slope [expr {double($yvals($max)-$yvals($min))
                      / ($wherey($max)-$wherey($min))}]
    set _yMin [expr {-$wherey($min)*$slope + $yvals($min)}]
    set ymax [expr {(1-$wherey($max))*$slope + $yvals($max)}]
    set _yScale [expr {[winfo height $itk_component(drawing)]/($ymax-$_yMin)}]
}

itcl::body Rappture::DrawingEntry::ParseBackground {} {
    foreach elem [$_xmlobj children $_path.background] {
        switch -- $elem {
            "color" {
                #  Background color of the drawing canvas (default white)
                set value [XmlGet $_path.background.$elem]
                $itk_component(drawing) configure -background $value
            }
            "aspect" {
                set value [XmlGet $_path.background.$elem]
                foreach { xAspect yAspect } $value break
                AdjustDrawingArea $xAspect $yAspect
            }
            "coords" - "coordinates" {
                set value [XmlGet $_path.background.$elem]
                ParseScreenCoordinates $value
            }
            "width" {
                set width [XmlGet $_path.background.$elem]
                $itk_component(drawing) configure -width $width
            }
            "height" {
                set height [XmlGet $_path.background.$elem]
                $itk_component(drawing) configure -height $height
            }
            default {
                puts stderr "WARNING: don't understand \"$elem\" in $_path"
            }
        }
    }
}

#
# Invoke --
#
itcl::body Rappture::DrawingEntry::Invoke {cpath x y} {
    if {![info exists _cpath2popup($cpath)]} {
        puts stderr "WARNING: no controls for hotspot at $cpath"
        return
    }
    set popup $_cpath2popup($cpath)

    # if this popup has only one control, watch for it to change and
    # take it down automatically
    set inner [$popup component inner]
    set n [expr {[$inner.controls index end]+1}]
    if {$n == 1} {
        set _takedown $popup
    } else {
        set _takedown ""
    }

    # Activate the popup and call for the output.
    set rootx [winfo rootx $itk_component(drawing)]
    set rooty [winfo rooty $itk_component(drawing)]

    set x [expr {round($x + $rootx)}]
    set y [expr {round($y + $rooty)}]
    $popup activate @$x,$y above
}

#
# Activate --
#
itcl::body Rappture::DrawingEntry::Activate { cname } {
    $itk_component(drawing) configure -cursor center_ptr
    $itk_component(drawing) itemconfigure $_cname2id($cname) \
        -image [Rappture::icon hotspot_active]
}

#
# Deactivate --
#
itcl::body Rappture::DrawingEntry::Deactivate { cname } {
    $itk_component(drawing) configure -cursor ""
    $itk_component(drawing) itemconfigure $_cname2id($cname) \
        -image [Rappture::icon hotspot_normal]
}

# ----------------------------------------------------------------------
# USAGE: value ?-check? ?<newval>?
#
# Clients use this to query/set the value for this widget.  With
# no args, it returns the current value for the widget.  If the
# <newval> is specified, it sets the value of the widget and
# sends a <<Value>> event.  If the -check flag is included, the
# new value is not actually applied, but just checked for correctness.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingEntry::value {args} {
    # Redraw if there's a new library object.
    if { [llength $args] > 0 } {
        set libobj [lindex $args 0]
        if { $libobj != "" } {
            $_dispatcher event -idle !redraw
        }
    }
    return ""
}

itcl::body Rappture::DrawingEntry::UpdateSubstitutions {} {
    # Load parser with the variables representing the substitution
    foreach name [array names _name2path] {
        set path $_name2path($name)
        set w [$_owner widgetfor $path]
        if {$w ne ""} {
            set value [$w value]
        } else {
            set value ""
        }
        if {$_name2map($name) ne ""} {
            set value [string map $_name2map($name) $value]
        }
        $_parser eval [list set $name $value]
    }
}

itcl::body Rappture::DrawingEntry::XmlGet { path } {
    set value [$_xmlobj get $path]
    return [string trim $value]
}

itcl::body Rappture::DrawingEntry::XmlGetSubst { path } {
    set value [$_xmlobj get $path]
    if {$_parser == ""} {
        return [string trim $value]
    }
    return [string trim [$_parser eval [list subst -nocommands $value]]]
}
