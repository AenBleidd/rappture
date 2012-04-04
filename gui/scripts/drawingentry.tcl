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

itcl::class Rappture::DrawingEntry {
    inherit itk::Widget
    itk_option define -state state State "normal"

    private variable _owner
    private variable _path
    private variable _cname2id
    private variable _worldX 0
    private variable _worldY 0
    private variable _worldWidth 0
    private variable _worldHeight 0

    constructor {owner path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    private method ParseDescription {}
    private method ParseLineDescription { cname }
    private method ParseGridDescription { cname }
    private method ParseTextDescription { cname }
    private method ParseStringDescription { cname }
    private method ParseNumberDescription { cname }
    private method GetCanvasHeight { } 
    private method GetCanvasWidth { } 
    private method ScreenX { x } 
    private method ScreenY { y } 
    private method ScreenCoords { coords } 
    private method Highlight { tag } 
    private method Unhighlight { tag } 
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
	canvas $itk_interior.canvas -background white -relief sunken -bd 1
    } {
	ignore -background
    }
    pack $itk_component(drawing) -expand yes -fill both
    ParseDescription
    eval itk_initialize $args
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
puts "value $args"
    set onlycheck 0
    set i [lsearch -exact $args -check]
    if {$i >= 0} {
        set onlycheck 1
        set args [lreplace $args $i $i]
    }

    if {[llength $args] == 1} {
        if {$onlycheck} {
            # someday we may add validation...
            return
        }
        set xmlobj [lindex $args 0]
        $itk_component(drawing) value $xmlobj
        return $xmlobj

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    return [$itk_component(drawing) value]
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

itcl::body Rappture::DrawingEntry::ParseLineDescription { cname } {
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
    set coords [$_owner xml get $_path.components.$cname.coords]
    set coords [string trim $coords]
    if { $coords == "" } {
	set coords "0 0"
    } else {
	set coords [ScreenCoords $coords]
    }
    puts stderr "ParseLineDescrption description owner=$_owner path=$_path coords=$coords"
    set list {}
    foreach attr [$_owner xml children $_path.components.$cname] {
	if { [info exists attr2option($attr)] } {
	    set option $attr2option($attr)
	    set value [$_owner xml get $_path.components.$cname.$attr]
	    set options($option) $value
	}
    }
    puts stderr "$itk_component(drawing) create line $coords"
    set id [eval $itk_component(drawing) create line $coords]
    set _cname2id($cname) $id
    eval $itk_component(drawing) itemconfigure $id [array get options]
}

#
# ParseGridDescription -- 
#
itcl::body Rappture::DrawingEntry::ParseGridDescription { cname } {
    puts stderr "ParseGridDescrption description owner=$_owner path=$_path"
    foreach attr [$_owner xml children $_path.components.$cname] {
	puts stderr attr=$attr
    }
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
    set xcoords [$_owner xml get $_path.components.$cname.xcoords]
    set xcoords [string trim $xcoords]
    set ycoords [$_owner xml get $_path.components.$cname.ycoords]
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
    puts stderr "ParseLineDescrption description owner=$_owner path=$_path xcoords=$xcoords ycoords=$ycoords"
    set list {}
    foreach attr [$_owner xml children $_path.components.$cname] {
	if { [info exists attr2option($attr)] } {
	    set option $attr2option($attr)
	    set value [$_owner xml get $_path.components.$cname.$attr]
	    set options($option) $value
	}
    }
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

itcl::body Rappture::DrawingEntry::ParseTextDescription { cname } {
    array set attr2option {
	"font"		"-font"
	"default"	"-text"
	"color"		"-fill"
	"text"		"-text"
	"anchor"	"-anchor"
    }
    puts stderr "ParseStringDescrption description owner=$_owner path=$_path"

    # Set default options first and then let tool.xml override them.
    array set options {
	-font {Arial 12}
	-text {}
	-fill black
	-anchor c
    }
    foreach attr [$_owner xml children $_path.components.$cname] {
	if { [info exists attr2option($attr)] } {
	    set option $attr2option($attr)
	    set value [$_owner xml get $_path.components.$cname.$attr]
	    set options($option) $value
	}
    }
    # Coords
    set coords {}
    set coords [$_owner xml get $_path.components.$cname.coords]
    set coords [string trim $coords]
    if { $coords == "" } {
	set coords "0 0"
    } else {
	set coords [ScreenCoords $coords]
    }
    puts stderr "$itk_component(drawing) create text $coords"
    set id [eval $itk_component(drawing) create text $coords]
    set _cname2id($cname) $id
    puts stderr "$itk_component(drawing) itemconfigure $id [array get options]"
    eval $itk_component(drawing) itemconfigure $id [array get options]
}

itcl::body Rappture::DrawingEntry::ParseStringDescription { cname } {
    array set attr2option {
	"font"		"-font"
	"default"	"-text"
	"color"		"-fill"
    }
    puts stderr "ParseStringDescrption description owner=$_owner path=$_path"

    # Set default options first and then let tool.xml override them.
    array set options {
	-font {Arial 12}
	-text {}
	-fill black
    }
    foreach attr [$_owner xml children $_path.components.$cname] {
	if { [info exists attr2option($attr)] } {
	    set option $attr2option($attr)
	    set value [$_owner xml get $_path.components.$cname.$attr]
	    set options($option) $value
	}
    }
    # Coords
    set coords {}
    set coords [$_owner xml get $_path.components.$cname.coords]
    set coords [string trim $coords]
    if { $coords == "" } {
	set coords "0 0"
    }
    # Is there a label?
    set label [$_owner xml get $_path.components.$cname.about.label]
    set labelWidth 0
    if { $label != "" } {
	set labelId [$itk_component(drawing) create text -1000 -1000 \
			 -text $label -font $options(-font)]
	set labelWidth [font measure $options(-font) $label]
    }
    set id [$itk_component(drawing) create text -1000 -1000  -tag $cname]
    set entryWidth [font measure $options(-font) $options(-text) ]

    foreach { x y } [ScreenCoords $coords] break
    set lx $x
    set ly $y
    set tx [expr $x + $labelWidth]
    set ty $y

    eval $itk_component(drawing) coords $id $tx $ty
    if { $labelWidth > 0 } {
	puts stderr "LABEL($labelWidth):$itk_component(drawing) coords $labelId $lx $ly"
	eval $itk_component(drawing) coords $labelId $lx $ly
    }
    set _cname2id($cname) $id
    puts stderr "$itk_component(drawing) itemconfigure $id [array get options]"
    eval $itk_component(drawing) itemconfigure $id [array get options]
    set bbox [$itk_component(drawing) bbox $id]
    puts stderr "cname=$cname bbox=$bbox"
    set sid [eval $itk_component(drawing) create rectangle $bbox]
    $itk_component(drawing) itemconfigure $sid -fill "" -outline "" \
	-tag $cname-bg
    set sid [eval $itk_component(drawing) create rectangle $bbox]
    $itk_component(drawing) itemconfigure $sid -fill "" -outline "" \
	-tag $cname-sensor
    $itk_component(drawing) bind $cname-sensor <Enter> \
	[itcl::code $this Highlight $cname]
    $itk_component(drawing) bind $cname-sensor <Leave> \
	[itcl::code $this Unhighlight $cname]
    $itk_component(drawing) lower $cname-bg
    $itk_component(drawing) raise $cname-sensor
}

itcl::body Rappture::DrawingEntry::ParseNumberDescription { cname } {
    puts stderr "ParseNumberDescrption description owner=$_owner path=$_path"
    foreach attr [$_owner xml children $_path.components.$cname] {
	puts stderr attr=$attr
    }
}

itcl::body Rappture::DrawingEntry::ScreenX { x } {
    set norm [expr ($x - $_worldX) / double($_worldWidth)]
    puts stderr "ScreenX x=$x, norm=$norm wx=$_worldX ww=$_worldWidth"
    set x [expr int($norm * [GetCanvasWidth])]
    puts stderr "ScreenX after x=$x cw=[GetCanvasWidth]"
    return $x
}

itcl::body Rappture::DrawingEntry::ScreenY { y } {
    set norm [expr ($y - $_worldY) / double($_worldWidth)]
    set y [expr int($norm * [GetCanvasHeight])]
    return $y
}

itcl::body Rappture::DrawingEntry::ScreenCoords { coords } {
    set list {}
    foreach {x y} $coords {
	lappend list [ScreenX $x] [ScreenY $y]
    }
    return $list
}

itcl::body Rappture::DrawingEntry::GetCanvasWidth { } {
    set w [winfo width $itk_component(drawing)] 
    if { $w < 2 } {
	set w [winfo reqwidth $itk_component(drawing)]
    }
    return $w
}

itcl::body Rappture::DrawingEntry::GetCanvasHeight { } {
    set h [winfo height $itk_component(drawing)]
    if { $h < 2 } {
	set h [winfo reqheight $itk_component(drawing)]
    }
    return $h
}

itcl::body Rappture::DrawingEntry::ParseDescription {} {
    puts stderr "ParseDescrption description owner=$_owner path=$_path"
    set bbox [$_owner xml get $_path.about.bbox] 
    puts stderr bbox=$bbox
    if { [llength $bbox] != 4 } {
	set bbox "0 0 [GetCanvasWidth] [GetCanvasHeight]"
    } 
    foreach { x1 y1 x2 y2 } $bbox break
    set _worldWidth [expr $x2 - $x1]
    set _worldHeight [expr $y2 - $y1]
    set _worldX $x1
    set _worldY $y1
    foreach cname [$_owner xml children $_path.components] {
	switch -glob -- $cname {
	    "line*" {
		ParseLineDescription $cname
	    }
	    "grid*" {
		ParseGridDescription $cname
	    }
	    "text*" {
		ParseTextDescription $cname
	    }
	    "string*" {
		ParseStringDescription $cname
	    }
	    "number*" {
		ParseNumberDescription $cname
	    }
	}
    }
}



itcl::body Rappture::DrawingEntry::Highlight { tag } {
    $itk_component(drawing) itemconfigure $tag-bg -outline black \
	-fill lightblue
}

itcl::body Rappture::DrawingEntry::Unhighlight { tag } {
    $itk_component(drawing) itemconfigure $tag-bg -outline "" \
	-fill ""
}
