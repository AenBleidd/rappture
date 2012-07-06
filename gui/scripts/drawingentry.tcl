
# ----------------------------------------------------------------------
#  COMPONENT: DrawingEntry - widget for entering numeric values
#
#  This widget represents a <number> entry on a control panel.
#  It is used to enter numeric values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require Img

itcl::class Rappture::DrawingEntry {
    inherit itk::Widget
    itk_option define -state state State "normal"

    private variable _canvasHeight 0
    private variable _canvasWidth 0
    private variable _cname2controls
    private variable _cname2id
    private variable _cname2image
    private variable _name2path
    private variable _drawingHeight 0
    private variable _drawingWidth 0
    private variable _owner
    private variable _parser "";	# Slave interpreter where all 
					# substituted variables are stored.
    private variable _path
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
    private method ControlValue {path {units ""}}
    private method Deactivate { tag } 
    private method Highlight { tag } 
    private method InitSubstitutions {} 
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
    private method ParseSubstitutions {}
    private method ParseText { cpath cname }
    private method Redraw {}
    private method ScreenCoords { coords } 
    private method ScreenX { x } 
    private method ScreenY { y } 
    private method XmlGet { path } 
    private method XmlGetSubst { path } 
    private method Withdraw { cname } 
    private method Hotspot { option cname item args } 
    private method IsEnabled { path } 
    private method NumControlsEnabled { cname } 
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
    #
    # Display the current drawing.
    #
    itk_component add drawing {
	canvas $itk_interior.canvas -background white -relief sunken -bd 1 \
	    -width 800 -height 600
    } {
	ignore -background
    }
    pack $itk_component(drawing) -expand yes -fill both
    bind $itk_component(drawing) <Configure> [itcl::code $this Redraw]
    set _parser [interp create -safe]
    Redraw
    eval itk_initialize $args
}

itcl::body Rappture::DrawingEntry::destructor {} {
    if { $_parser != "" } {
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
return ""
    set label [$_owner xml get $_path.about.label]
    if {"" == $label} {
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
return ""
    set str [$_owner xml get $_path.about.description]
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
    # Remove exists canvas items and hints
    $itk_component(drawing) delete all
    # Delete any images that we created.
    foreach name [array names _cname2image] {
	image delete $_cname2image($name)
    }
    array unset _name2path
    array unset _cname2id
    array unset _cnames2controls
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
    ParseDescription
}

#
# ParseDescription -- 
#
itcl::body Rappture::DrawingEntry::ParseDescription {} {
    #puts stderr "ParseDescription owner=$_owner path=$_path"
    ParseBackground
    ParseSubstitutions
    foreach cname [$_owner xml children $_path.components] {
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
    #puts stderr "ParseGrid owner=$_owner cpath=$cpath"
    array set attr2option {
	"linewidth"	"-width"
	"arrow"		"-arrow"
	"dash"		"-dash"
	"color"		"-fill"
    }
    # Set default options first and then let tool.xml override them.
    array set options {
	-arrow		none
	-width		0
	-fill		black
	-dash		""
    }
    # Coords
    set xcoords [XmlGetSubst $cpath.xcoords]
    set xcoords [string trim $xcoords]
    set ycoords [XmlGetSubst $cpath.ycoords]
    set ycoords [string trim $ycoords]
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
    #puts stderr "ParseGrid owner=$_owner cpath=$cpath xcoords=$xcoords ycoords=$ycoords"
    set list {}
    foreach attr [$_owner xml children $cpath] {
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
	"color"	"-fill"
	"anchor" "-anchor"
    }
    #puts stderr "ParseHotspot owner=$_owner cpath=$cpath"
    # Set default options first and then let tool.xml override them.
    array set options {
	-fill red
	-anchor c
    }
    array unset _cname2controls $cname
    foreach attr [$_owner xml children $cpath] {
	if { [info exists attr2option($attr)] } {
	    set option $attr2option($attr)
	    set value [XmlGetSubst $cpath.$attr]
	    set options($option) $value
	} elseif { [string match "controls*" $attr] } {
	    set value [XmlGetSubst $cpath.$attr]
	    lappend _cname2controls($cname) $value
	    $_owner xml put $value.hide 1
	}
    }
    # Coordinates
    set coords [XmlGetSubst $cpath.coords]
    set coords [ScreenCoords $coords]
    if { $coords == "" } {
	set coords "0 0 1 1"
    } 
    set c $itk_component(drawing)
    set img [Rappture::icon hotspot_normal]
    foreach { x1 y1 } $coords break
    set id [$itk_component(drawing) create image $x1 $y1]
    array unset options -fill
    set options(-tags) $cname
    set options(-image) $img
    eval $c itemconfigure $id [array get options]
    set _cname2id($cname) $id
    $c bind $id <Enter> [itcl::code $this Activate $cname]
    $c bind $id <Leave> [itcl::code $this Deactivate $cname]
    #$c bind $id <ButtonPress-1> [itcl::code $this Depress $cname]
    set bbox [$c bbox $id]
    set y1 [lindex $bbox 1]
    $c bind $id <ButtonPress-1> [itcl::code $this Invoke $cname $x1 $y1]
}

#
# ParseLine -- 
#
itcl::body Rappture::DrawingEntry::ParseLine { cpath cname } {
    array set attr2option {
	"linewidth"	"-width"
	"arrow"		"-arrow"
	"dash"		"-dash"
	"color"		"-fill"
    }
    # Set default options first and then let tool.xml override them.
    array set options {
	-arrow		none
	-width		0
	-fill		black
	-dash		""
    }
    # Coords
    set coords {}
    set coords [XmlGetSubst $cpath.coords]
    set coords [string trim $coords]
    if { $coords == "" } {
	set coords "0 0"
    } else {
	set coords [ScreenCoords $coords]
    }
    #puts stderr "ParseLine owner=$_owner cpath=$cpath coords=$coords"
    set list {}
    foreach attr [$_owner xml children $cpath] {
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
	"outline"	"-outline"
	"fill"		"-fill"
	"linewidth"	"-linewidth"
    }
    #puts stderr "ParseOval owner=$_owner cpath=$cpath"

    # Set default options first and then let tool.xml override them.
    array set options {
	-fill blue
	-linewidth 1 
	-outline black
    }
    foreach attr [$_owner xml children $cpath] {
	if { [info exists attr2option($attr)] } {
	    set option $attr2option($attr)
	    set value [XmlGetSubst $cpath.$attr]
	    set options($option) $value
	}
    }
    # Coordinates
    set coords {}
    set coords [XmlGetSubst $cpath.coords]
    set coords [string trim $coords]
    if { $coords == "" } {
	set coords "0 0 1 1"
    }
    foreach { x1 y1 x2 y2 } [ScreenCoords $coords] break
    set id [eval $itk_component(drawing) create oval $coords]
    set _cname2id($cname) $id
}

#
# ParsePicture -- 
#
itcl::body Rappture::DrawingEntry::ParsePicture { cpath cname } {
    array set attr2option {
	"anchor"	"-anchor"
    }
    #puts stderr "ParsePicture owner=$_owner cpath=$cpath"
    # Set default options first and then let tool.xml override them.
    array set options {
	-anchor nw
    }
    foreach attr [$_owner xml children $cpath] {
	if { [info exists attr2option($attr)] } {
	    set option $attr2option($attr)
	    set value [XmlGetSubst $cpath.$attr]
	    set options($option) $value
	}
    }
    set contents [XmlGetSubst $cpath.contents]
    set img ""
    if { [string compare -length 5 $contents "file:"] == 0 } {
	set fileName [string range $contents 5 end]
	if { [file exists $fileName] } {
	    set img [image create photo -file $fileName]
	}
    } elseif { [string compare -length 5 $contents "http:"] == 0 } {
	puts stderr  "don't know how to handle http"
	return
    } else {
	set img [image create photo -data $contents]
    }
    if { $img == "" } {
	return
    }
    # Coordinates
    set coords [XmlGetSubst $cpath.coords]
    set coords [ScreenCoords $coords]
    if { [llength $coords] == 2 } {
	foreach { x1 y1 } $coords break
	set w [XmlGetSubst $cpath.width]
	if { $w == "" || ![string is number $w] || $w <= 0.0 } {
	    set width [expr [image width $img] / 4]
	} else {
	    set width [expr [ScreenX $w] - [ScreenX 0]]
	}
	set h [XmlGetSubst $cpath.height]
	if { $h == "" || ![string is number $h] || $h <= 0.0 } {
	    set height [expr [image height $img] / 4]
	} else {
	    set height [expr [ScreenY $h] - [ScreenY 0]]
	}
	if { $width != [image width $img] || $height != [image height $img] } {
	    set dst [image create photo -width $width -height $height]
	    blt::winop resample $img $dest
	    image delete $img
	    set img $dst
	}
    } elseif { [llength $coords] == 4 } {
	foreach { x1 y1 x2 y2 } $coords break
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
	set width [expr $x2 - $x1 + 1]
	set height [expr $x2 - $x1 + 1]
	if { $width != [image width $img] || $height != [image height $img] } {
	    set dst [image create photo -width $width -height $height]
	    blt::winop resample $img $dst
	    image delete $img
	    set img $dst
	}
    } else {
	set width [expr [image width $img] / 4]
	set height [expr [image height $img] / 4]
 	set dst [image create photo -width $width -height $height]
	blt::winop resample $img $dst
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
	"linewidth"	"-width"
	"arrow"		"-arrow"
	"color"		"-fill"
    }
    # Set default options first and then let tool.xml override them.
    array set options {
	-arrow		none
	-width		0
	-fill		black
    }
    # Coords
    set coords [XmlGetSubst $cpath.coords]
    set coords [string trim $coords]
    if { $coords == "" } {
	set coords "0 0"
    } else {
	set coords [ScreenCoords $coords]
    }
    set x1 [lindex $coords 0]
    set y1 [lindex $coords 1]
    lappend coords $x1 $y1
    #puts stderr "ParsePolygon owner=$_owner cpath=$cpath coords=$coords"
    set list {}
    foreach attr [$_owner xml children $cpath] {
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
	"outline"	"-outline"
	"fill"		"-fill"
	"linewidth"	"-linewidth"
    }
    #puts stderr "ParseRectangle owner=$_owner cpath=$cpath"

    # Set default options first and then let tool.xml override them.
    array set options {
	-fill blue
	-linewidth 1 
	-outline black
    }
    foreach attr [$_owner xml children $cpath] {
	if { [info exists attr2option($attr)] } {
	    set option $attr2option($attr)
	    set value [XmlGetSubst $cpath.$attr]
	    set options($option) $value
	}
    }
    # Coordinates
    set coords [XmlGetSubst $cpath.coords]
    set coords [string trim $coords]
    if { $coords == "" } {
	set coords "0 0 1 1"
    }
    foreach { x1 y1 x2 y2 } [ScreenCoords $coords] break
    set id [eval $itk_component(drawing) create rectangle $coords]
    set _cname2id($cname) $id
}

#
# ParseText -- 
#
itcl::body Rappture::DrawingEntry::ParseText { cpath cname } {
    array set attr2option {
	"font"		"-font"
	"color"		"-foreground"
	"valuecolor"	"-valueforeground"
	"text"		"-text"
	"anchor"	"-anchor"
    }
    #puts stderr "ParseText owner=$_owner cpath=$cpath"

    # Set default options first and then let tool.xml override them.
    array set options {
	-font {Arial 12}
	-valuefont {Arial 12}
	-valueforeground blue3
	-text {}
	-fill {}
	-anchor c
    }
    foreach attr [$_owner xml children $cpath] {
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
    set coords [string trim $coords]
    if { $coords == "" } {
	set coords "0 0"
    } else {
	set coords [ScreenCoords $coords]
    }
    set hotspot [XmlGetSubst $cpath.hotspot]
    if { $hotspot == "inline" } {
	set options(-showicons) 1 
    }
    set c $itk_component(drawing)
    set options(-tags) $cname
    set img [Rappture::icon hotspot_normal]
    set options(-image) $img
    set img [Rappture::icon hotspot_active]
    set options(-activeimage) $img
    set id [eval $c create hotspot $coords]
    set _cname2id($cname) $id
    set options(-interp) $_parser
    eval $c itemconfigure $id [array get options]
    if { $hotspot == "inline" } {
	array unset _cname2controls $cname
	foreach varName [Rappture::hotspot variables $c $id] {
	    if { [info exists _name2path($varName)] } {
		set path $_name2path($varName)
		$_owner xml put $path.hide 1
		lappend _cname2controls($cname) $path
	    } else {
		puts stderr "unknown varName=$varName"
	    }
	}
	$c bind $id <Motion> \
	    [itcl::code $this Hotspot watch $cname $id %x %y]
	$c bind $id <Leave> \
	    [itcl::code $this Hotspot deactivate $cname $id]
	$c bind $id <Enter> \
	    [itcl::code $this Hotspot activate $cname $id %x %y]
	$c bind $id <ButtonRelease-1> \
	    [itcl::code $this Hotspot invoke $cname $id %x %y]
    }
}


itcl::body Rappture::DrawingEntry::Hotspot { option cname item args } {
    if { [NumControlsEnabled $cname] == 0 } {
	return
    }
    set c $itk_component(drawing)
    switch -- $option {
	"activate" {
	    foreach { x y } $args break
	    set varName  [Rappture::hotspot identify $c $item $x $y]
	    $c itemconfigure $item -activevalue $varName
	}
	"deactivate" {
	    $c itemconfigure $item -activevalue ""
	}
	"watch" {
	    foreach { x y } $args break
	    set active [$c itemcget $item -activevalue]
	    set varName  [Rappture::hotspot identify $c $item $x $y]
	    if { $varName != $active  } {
		$c itemconfigure $item -activevalue $varName
	    }
	}
	"invoke" {
	    foreach { x y } $args break
	    set active [$c itemcget $item -activevalue]
	    set varName  [Rappture::hotspot identify $c $item $x $y]
	    if { $varName != "" } {
		set bbox [$c bbox $item]
		Invoke $cname $x [lindex $bbox 1]
	    }
	}
    }
}


itcl::body Rappture::DrawingEntry::ScreenX { x } {
    set norm [expr ($x - $_xMin) * $_xScale]
    set x [expr int($norm * $_drawingWidth) + $_xOffset]
    return $x
}

itcl::body Rappture::DrawingEntry::ScreenY { y } {
    set norm [expr ($y - $_yMin) * $_yScale]
    set y [expr int($norm * $_drawingHeight) + $_yOffset]
    return $y
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
	set sh [expr int($_canvaseHeight / $wanted)]
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
#				?at screenx screeny?
#            The screenx/screeny values are optional, so you can also say
# 	   something like "-.1 0 1.1 1" as you had in your example.
# 	   This lets you put the origin at a specific point on screen,
# 	   and also define the directions of the axes.  We still compute
# 	   the overall bounding box.  In the example below, the bounding
# 	   box goes from -1,1 in the upper-left corner to 1,-1 in the
# 	   lower right.
#       -->
#       <coordinates>0 0 at 50% 50% 1 1 at 100% 100%</coordinates>

#       <!-- aspect ratio:  scales coordinate system so that pixels may not
#            be square.  A coordinate system like the one above implies a
# 	   square drawing area, since x and y both go from -1 to 1.  But
# 	   if you set the aspect to 2:1, you'll get something twice as
# 	   wide as it is tall.  This effectively says that x goes from
# 	   -1 to 1 in a certain distance, but y goes from -1 to 1 in half
# 	   that screen distance.  Default is whatever aspect is defined
# 	   by the coordinates.  If x goes 0-3 and y goes 0-1, then the
# 	   drawing (without any other aspect ratio) would be 3x wide and
# 	   1x tall.  The aspect ratio could be used to force it to be
# 	   square instead by setting "1 1" instead.  In that case, x goes
# 	   0-3 over the width, and y goes 0-1 over the same screen distance
# 	   along the height.
#       -->
#       <aspect>2 1</aspect>
#     </background>
#

itcl::body Rappture::DrawingEntry::ParseScreenCoordinates { values } {
    set len [llength $values]
    if { $len == 4 } {
	if { [scan $values "%g %g %g %g" x1 y1 x2 y2] != 4 } {
	    error "bad coordinates specification \"$values\""
	}
	set _xScale [expr 1.0 / ($x2 - $x1)]
	set _yScale [expr 1.0 / ($y2 - $y1)]
	set _xMin $x1
	set _yMin $y1
    } elseif { $len == 10 } {
	if { [scan $values "%g %g %s %d%% %d%% %g %g %s %d%% %d%%" \
		  sx1 sy1 at1 x1 y1 sx2 sy2 at2 x2 y2] != 10 } {
	    error "bad coordinates specification \"$values\""
	}
	if { $at1 != "at" || $at2 != "at" } {
	    error "bad coordinates specification \"$values\""
	}	    
	set x1 [expr $x1 / 100.0]
	set x2 [expr $x2 / 100.0]
	set y1 [expr $y1 / 100.0]
	set y2 [expr $y2 / 100.0]
	set _xScale [expr ($sx2 - $sx1) / ($x2 - $x1)]
	set _yScale [expr ($sy2 - $sy2) / ($y2 - $y2)]
	set _xMin $x1
	set _yMin $y1
    }
}

itcl::body Rappture::DrawingEntry::ParseBackground {} {
    foreach elem [$_owner xml children $_path.background] {
	switch -glob -- $elem {
	    "color*" {
		#  Background color of the drawing canvas (default white)
		set value [XmlGet $_path.background.$elem]
		$itk_component(drawing) configure -background $value
	    }
	    "aspect*" {
		set value [XmlGet $_path.background.$elem]
		foreach { xAspect yAspect } $value break
		AdjustDrawingArea $xAspect $yAspect
	    }
	    "coordinates*" {
		set value [XmlGet $_path.background.$elem]
		ParseScreenCoordinates $value
	    }
	    "width*" {
		set width [XmlGet $_path.background.$elem]
		$itk_component(drawing) configure -width $width
	    }
	    "height*" {
		set height [XmlGet $_path.background.$elem]
		$itk_component(drawing) configure -height $height
	    }
	}
    }
}

itcl::body Rappture::DrawingEntry::ParseSubstitutions {} {
    foreach var [$_owner xml children $_path.substitutions] {
	if { ![string match "variable*" $var] } {
	    continue
	}
	set varPath $_path.substitutions.$var
	set map ""
	set name ""
	set path ""
	foreach elem [$_owner xml children $varPath] {
	    switch -glob -- $elem {
		"name*" {
		    set name [XmlGet $varPath.$elem]
		}
		"path*" {
		    set path [XmlGet $varPath.$elem]
		}
		"map*" {
		    set from [XmlGet $varPath.$elem.from]
		    set to [XmlGet $varPath.$elem.to]
		    if { $from == "" || $to == "" } {
			puts stderr "empty translation in map table \"$varPath\""
		    }
		    lappend map $from $to
		}
	    }
	}
	if { $name == "" } {
	    puts stderr \
		"no name defined for substituion variable \"$varPath\""
	    continue
	}
	if { [info exists _name2path($name)] } {
	    puts stderr \
		"substitution variable \"$name\" already defined"
	    continue
	}		
	set _name2path($name) $path
	if { $path == "" } {
	    puts stderr \
		"no path defined for substituion variable \"$varPath\""
	    continue
	}
	set _name2map($name) $map
    }
    InitSubstitutions
}

#
# Invoke -- 
#
itcl::body Rappture::DrawingEntry::Invoke { cname x y } {
    set controls $_cname2controls($cname)
    if { [llength $controls] == 0 } {
	puts stderr "no controls defined for $cname"
	return 
    }
    # Build a popup with the designated controls
    set popup .drawingentrypopup
    if { ![winfo exists $popup] } {
	# Create a popup for the controls dialog
	Rappture::Balloon $popup -title "Change values..." \
	    -deactivatecommand [itcl::code $this Withdraw $cname]
	set inner [$popup component inner]
	Rappture::DrawingControls $inner.controls $_owner \
	    -deactivatecommand [list $popup deactivate]
	pack $inner.controls -fill both -expand yes
    } else {
	set inner [$popup component inner]
	$inner.controls delete all
    }
    set count 0
    foreach path $controls {
	if { [IsEnabled $path] } {
	    $inner.controls add $path
	    incr count
	}
    }
    if { $count == 0 } {
	return
    }
    update
    # Activate the popup and call for the output.
    incr x [winfo rootx $itk_component(drawing)]
    incr y [winfo rooty $itk_component(drawing)]
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
    $itk_component(drawing) configure -cursor left_ptr 
    $itk_component(drawing) itemconfigure $_cname2id($cname) \
	-image [Rappture::icon hotspot_normal]
}

#
# Withdraw -- 
#
itcl::body Rappture::DrawingEntry::Withdraw { cname } {
    Redraw
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
    # drawing entries have no value
    return ""
}


#
# InitSubstitutions -- 
#
itcl::body Rappture::DrawingEntry::InitSubstitutions {} {
    # Load a new parser with the variables representing the substitution
    foreach name [array names _name2path] {
	set path $_name2path($name)
	set w [$_owner widgetfor $path]
	if { $w != "" } {
	    set value [$w value]
	} else {
	    set value ""
	}
	$_parser eval [list set $name $value]
    }
}

itcl::body Rappture::DrawingEntry::XmlGet { path } {
    set value [$_owner xml get $path]
    return [string trim $value]
}

itcl::body Rappture::DrawingEntry::XmlGetSubst { path } {
    set value [$_owner xml get $path]
    if { $_parser == "" } {
	return $value
    }
    return [string trim [$_parser eval [list subst -nocommands $value]]]
}

itcl::body Rappture::DrawingEntry::IsEnabled { path } {
    set enable [string trim [$_owner xml get $path.about.enable]]
    if {"" == $enable} {
        return 1
    }
    if {![string is boolean $enable]} {
        set re {([a-zA-Z_]+[0-9]*|\([^\(\)]+\)|[a-zA-Z_]+[0-9]*\([^\(\)]+\))(\.([a-zA-Z_]+[0-9]*|\([^\(\)]+\)|[a-zA-Z_]+[0-9]*\([^\(\)]+\)))*(:[-a-zA-Z0-9/]+)?}
        set rest $enable
        set enable ""
        set deps ""
        while {1} {
            if {[regexp -indices $re $rest match]} {
                foreach {s0 s1} $match break

                if {[string index $rest [expr {$s0-1}]] == "\""
                      && [string index $rest [expr {$s1+1}]] == "\""} {
                    # string in ""'s? then leave it alone
                    append enable [string range $rest 0 $s1]
                    set rest [string range $rest [expr {$s1+1}] end]
                } else {
                    #
                    # This is a symbol which should be substituted
                    # it can be either:
                    #   input.foo.bar
                    #   input.foo.bar:units
                    #
                    set cpath [string range $rest $s0 $s1]
                    set parts [split $cpath :]
                    set ccpath [lindex $parts 0]
                    set units [lindex $parts 1]

                    # make sure we have the standard path notation
                    set stdpath [$_owner regularize $ccpath]
                    if {"" == $stdpath} {
                        puts stderr "WARNING: don't recognize parameter $cpath in <enable> expression for $path.  This may be buried in a structure that is not yet loaded."
                        set stdpath $ccpath
                    }
                    # substitute [_controlValue ...] call in place of path
                    append enable [string range $rest 0 [expr {$s0-1}]]
                    append enable [format {[ControlValue %s %s]} $stdpath $units]
                    lappend deps $stdpath
                    set rest [string range $rest [expr {$s1+1}] end]
                }
            } else {
                append enable $rest
                break
            }
        }
    }
    return [expr $enable]
}

# ----------------------------------------------------------------------
# USAGE: ControlValue <path> ?<units>?
#
# Used internally to get the value of a control with the specified
# <path>.  Returns the current value for the control.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingEntry::ControlValue {path {units ""}} {
    if {"" != $_owner} {
        set val [$_owner valuefor $path]
         if {"" != $units} {
            set val [Rappture::Units::convert $val -to $units -units off]
        }
        return $val
    }
    return ""
}

itcl::body Rappture::DrawingEntry::NumControlsEnabled { cname } {
    set controls $_cname2controls($cname)
    set count 0
    foreach path $controls {
	if { [IsEnabled $path] } {
	    incr count
	}
    }
    return $count
}