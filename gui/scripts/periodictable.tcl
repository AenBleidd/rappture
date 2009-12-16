
# ----------------------------------------------------------------------
#  COMPONENT: periodictable - drop-down list of items
#
#  This is a drop-down listbox, which might be used in conjunction
#  with a combobox.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *PeriodicTable.textBackground white widgetDefault
option add *PeriodicTable.outline black widgetDefault
option add *PeriodicTable.borderwidth 1 widgetDefault
option add *PeriodicTable.relief flat widgetDefault
option add *PeriodicTable.font "Arial 8"

set periodicTableData {
Hydrogen 1 H 1.0079	1 1	\#a0ffa0
Helium 2 He 4.0026	1 18	\#c0ffff
Lithium 3 Li 6.941(2) 	2 1	\#ff6666
Beryllium 4 Be 9.0122   2 2	\#ffdead
Boron 5 B 10.811(7) 	2 13	\#cccc99
Carbon 6 C 12.011       2 14 	\#a0ffa0
Nitrogen 7 N 14.007 	2 15	\#a0ffa0
Oxygen 8 O 15.999 	2 16	\#a0ffa0 
Fluorine 9 F 18.998 	2 17	\#ffff99
Neon 10 Ne 20.180       2 18 	\#c0ffff

Sodium 11 Na 22.990 	3 1	\#ff6666
Magnesium 12 Mg 24.305  3 2 	\#ffdead
Aluminium 13 Al 26.982 	3 13	\#cccccc
Silicon 14 Si 28.086 	3 14 	\#cccc99
Phosphorus 15 P 30.974 	3 15	\#a0ffa0 
Sulfur 16 S 32.066(6) 	3 16	\#a0ffa6 
Chlorine 17 Cl 35.453   3 17	\#ffff99
Argon 18 Ar 39.948(1)   3 18	\#c0ffff

Potassium 19 K 39.098 	4 1 	\#ff6666
Calcium 20 Ca 40.078(4) 4 2 	\#ffdead
Scandium 21 Sc 44.956 	4 3	\#ffc0c0
Titanium 22 Ti 47.867(1) 4 4 	\#ffc0c0
Vanadium 23 V 50.942(1) 4 5 	\#ffc0c0
Chromium 24 Cr 51.996 	4 6 	\#ffc0c0
Manganese 25 Mn 54.938 	4 7 	\#ffc0c0
Iron 26 Fe 55.845(2) 4 8	\#ffc0c0
Cobalt 27 Co 58.933 4 9 	\#ffc0c0
Nickel 28 Ni 58.693 4 10 	\#ffc0c0
Copper 29 Cu 63.546(3) 	4 11 	\#ffc0c0
Zinc 30 Zn 65.39(2) 4 12 	\#ffc0c0
Gallium 31 Ga 69.723(1) 4 13 	\#cccccc
Germanium 32 Ge 72.61(2) 4 14 	\#cccc99
Arsenic 33 As 74.922 	4 15	\#cccc99
Selenium 34 Se 78.96(3) 4 16	\#a0ffa0 
Bromine 35 Br 79.904(1) 4 17	\#ffff99
Krypton 36 Kr 83.80(1) 4 18	\#c0ffff

Rubidium 37 Rb 85.468 	5 1 	\#ff6666
Strontium 38 Sr 87.62(1) 5 2	\#ffdead
Yttrium 39 Y 88.906 	5 3	\#ffc0c0
Zirconium 40 Zr 91.224(2) 5 4	\#ffc0c0
Niobium 41 Nb 92.906 	5 5 	\#ffc0c0
Molybdenum 42 Mo 95.94(1) 5 6 	\#ffc0c0
Technetium 43 Tc [97.907] 5 7 	\#ffc0c0
Ruthenium 44 Ru 101.07(2) 5 8 	\#ffc0c0
Rhodium 45 Rh 102.906 	5 9 	\#ffc0c0
Palladium 46 Pd 106.42(1) 5 10 	\#ffc0c0
Silver 47 Ag 107.868 	5 11 	\#ffc0c0
Cadmium 48 Cd 112.411(8) 5 12 	\#ffc0c0
Indium 49 In 114.818(3) 5 13	\#cccccc
Tin 50 Sn 118.710(7) 	5 14	\#cccccc
Antimony 51 Sb 121.760(1) 5 15 	\#cccc99
Tellurium 52 Te 127.60(3) 5 16 	\#cccc99
Iodine 53 I 126.904 5 17 	\#ffff99
Xenon 54 Xe 131.29(2) 5 18 	\#c0ffff

Cesium 55 Cs 132.905 	6 1 	\#ff6666
Barium 56 Ba 137.327(7)  6 2 	\#ffdead
Lanthanides 57-71 * * 6 3	\#ffbfff
Hafnium 72 Hf 178.49(2) 6 4 	\#ffc0c0
Tantalum 73 Ta 180.948 	6 5 	\#ffc0c0
Tungsten 74 W 183.84(1) 6 6 	\#ffc0c0
Rhenium 75 Re 186.207(1) 6 7	\#ffc0c0
Osmium 76 Os 190.23(3) 	6 8 	\#ffc0c0
Iridium 77 Ir 192.217(3) 6 9	\#ffc0c0
Platinum 78 Pt 195.084(9) 6 10 	\#ffc0c0
Gold 79 Au 196.967 6 11 	\#ffc0c0
Mercury 80 Hg 200.59(2) 6 12 	\#ffc0c0
Thallium 81 Tl 204.383 	6 13	\#cccccc
Lead 82 Pb 207.2(1) 6 14	\#cccccc
Bismuth 83 Bi 208.980 	6 15	\#cccccc
Polonium 84 Po [208.982] 6 16	\#cccc99
Astatine 85 At [209.987] 6 17 	\#ffff99
Radon 86 Rn [222.018] 6 18	\#c0ffff

Francium 87 Fr [223.020] 7 1 	\#ff6666
Radium 88 Ra [226.0254] 7 2 	\#ffdead
Actinides 89-103 * * 7 3	\#ff99cc
Rutherfordium 104 Rf [263.113] 	7 4 	\#ffc0c0
Dubnium 105 Db [262.114] 7 5	\#ffc0c0
Seaborgium 106 Sg [266.122] 7 6 \#ffc0c0
Bohrium 107 Bh [264.1247] 7 7 	\#ffc0c0
Hassium 108 Hs [269.134] 7 8 	\#ffc0c0
Meitnerium 109 Mt [268.139] 7 9 \#ffc0c0
Darmstadtium 110 Ds [272.146] 7 10 	\#ffc0c0
Roentgenium 111 Rg [272.154] 7 11	\#ffc0c0
Ununbium 112 Uub [277] 	7 12 	\#ffc0c0
Ununtrium 113 Uut [284] 7 13	\#cccccc
Ununquadium 114 Uuq [289] 7 14	\#cccccc
Ununpentium 115 Uup [288] 7 15	\#cccccc
Ununhexium 116 Uuh [292]  7 16 	\#cccccc
Ununseptium 117 Uus ? 7 17	\#ffffff
Ununoctium 118 Uuo [294] 7 18 	\#ffffff

Lanthanum 57 La 138.905 8 3 	\#ffbfff
Cerium 58 Ce 140.116(1) 8 4	\#ffbfff
Praseodymium 59 Pr 140.908 8 5 		\#ffbfff
Neodymium 60 Nd 144.242(3) 8 6 	\#ffbfff
Promethium 61 Pm [144.913] 8 7 	\#ffbfff
Samarium 62 Sm 150.36(2) 8 8 	\#ffbfff
Europium 63 Eu 151.964(1) 8 9 	\#ffbfff
Gadolinium 64 Gd 157.25(3) 8 10 	\#ffbfff
Terbium 65 Tb 158.925 	8 11 	\#ffbfff
Dysprosium 66 Dy 162.500(1) 8 12 	\#ffbfff
Holmium 67 Ho 164.930 8 13	\#ffbfff
Erbium 68 Er 167.259(3) 8 14	\#ffbfff
Thulium 69 Tm 168.934 	8 15	\#ffbfff
Ytterbium 70 Yb 173.04(3) 8 16 	\#ffbfff
Lutetium 71 Lu 174.967(1) 8 17	\#ffbfff

Actinium 89 Ac [227.027] 9 3	\#ff99cc
Thorium 90 Th 232.038 	9 4 	\#ff99cc
Protactinium 91 Pa 231.036 9 5 	\#ff99cc
Uranium 92 U 238.029 9 6	\#ff99cc
Neptunium 93 Np [237.048] 9 7 	\#ff99cc
Plutonium 94 Pu [244.064] 9 8 	\#ff99cc
Americium 95 Am [243.061] 9 9 	\#ff99cc
Curium 96 Cm [247.070] 9 10 	\#ff99cc
Berkelium 97 Bk [247.070] 9 11 	\#ff99cc
Californium 98 Cf [251.080] 9 12 	\#ff99cc
Einsteinium 99 Es [252.083] 9 13	\#ff99cc
Fermium 100 Fm [257.095] 9 14	\#ff99cc
Mendelevium 101 Md [258.098] 9 15 		\#ff99cc
Nobelium 102 No [259.101] 9 16 	\#ff99cc
Lawrencium 103 Lr [262.110] 9 17 	\#ff99cc
}

itcl::class Rappture::PeriodicTable {
    inherit Rappture::Dropdown

    constructor {args} { # defined below }

    public method include {args}
    public method exclude {args}
    public method get {args}
    public method select {name}
    public method value { {name ""} }

    private variable _table
    private variable _dispatcher ""
    private variable _current ""

    protected method _adjust {{widget ""}}

    protected method _react {}
    protected method Redraw {}
    protected method Activate { widget id x y }
    protected method Deactivate { widget id }
}

itk::usual PeriodicTable {
    keep -background -outline -cursor 
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicTable::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Redraw]; list"
    global periodicTableData
    foreach { name number symbol weight row column color } $periodicTableData {
	set _table($name) [list name $name number $number symbol $symbol \
		weight $weight row $row column $column color $color]
    }
    itk_component add scroller {
	Rappture::Scroller $itk_interior.sc \
	    -xscrollmode off -yscrollmode auto
    }
    pack $itk_component(scroller) -expand yes -fill both

    itk_component add table {
	canvas $itk_component(scroller).table \
	    -highlightthickness 0
    } {
	usual
	rename -background -textbackground textBackground Background
	ignore -highlightthickness -highlightbackground -highlightcolor
	keep -relief
    }
    $itk_component(scroller) contents $itk_component(table)

    # add bindings so the table can react to selections
    bind RappturePeriodicTable-$this <ButtonRelease-1> [itcl::code $this _react]
    bind RappturePeriodicTable-$this <KeyPress-Return> [itcl::code $this _react]
    bind RappturePeriodicTable-$this <KeyPress-space> [itcl::code $this _react]
    bind RappturePeriodicTable-$this <KeyPress-Escape> [itcl::code $this unpost]

    set btags [bindtags $itk_component(table)]
    set i [lsearch $btags [winfo class $itk_component(table)]]
    if {$i < 0} {
	set i end
    }
    set btags [linsert $btags [expr {$i+1}] RappturePeriodicTable-$this]
    bindtags $itk_component(table) $btags
    
    eval itk_initialize $args
    Redraw
}

# ----------------------------------------------------------------------
# USAGE: include <list of elements> 
#
# Inserts one or more values into this drop-down list.  Each value
# has a keyword (computer-friendly value) and a label (human-friendly
# value).  The labels appear in the listbox.  If the label is "--",
# then the value is used as the label.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicTable::include {args} {
    array unset _includes
    foreach name [array names _args] { 
	if { ![info exists _table($name)] } {
	    puts stderr "unknown element \"$name\""
	} else {
	    set _includes($name) 1
	}
    }
    $_dispatcher event -idle !rebuild
}

# ----------------------------------------------------------------------
# USAGE: exclude <list of elements> 
#
# Inserts one or more values into this drop-down list.  Each value
# has a keyword (computer-friendly value) and a label (human-friendly
# value).  The labels appear in the listbox.  If the label is "--",
# then the value is used as the label.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicTable::exclude {args} {
    foreach name [array names _table] { 
	set _includes($name) 1
    }
    foreach name [array names $args] { 
	if { ![info exists _table($name)] } {
	    puts stderr "unknown element \"$name\""
	} else {
	    array unset _includes $name
	}
    }
    $_dispatcher event -idle !rebuild
}

# ----------------------------------------------------------------------
# USAGE: get ?-symbol|-name|-all? ?name?
#
# Queries one or more values from the drop-down list.  With no args,
# it returns a list of all values and labels in the list.  If the
# index is specified, then it returns the value or label (or both)
# for the specified index.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicTable::get { args } {
    array set options { 
	"-symbol" "0"
	"-weight" "0"
	"-number" "0"
	"-name" "0"
	"-all" 0
    }
    set first [lindex $args 0]
    if {[string index $first 0] == "-"} {
	set choices {-symbol -weight -number -name -all}
	if {[lsearch $choices $first] < 0} {
	    error "bad option \"$first\": should be [join [lsort $choices] {, }]"
	}
	set format $first
	set args [lrange $args 1 end]
    }
    # return the whole list or just a single value
    if {[llength $args] > 1} {
	error "wrong # args: should be [join [lsort $choices] {, }]"
    }
    if {[llength $args] == 0} {
	set name $_current
    } else {
	set name [lindex $args 0]
    }
    if { ![info exists _table($name)] } {
	return ""
    }
    array set info $_table($name)
    # scan through and build up the return list
    switch -- $format {
	-name   { set value $info(name)   }
	-symbol { set value $info(symbol) }
	-weight { set value $info(weight) }
	-number { set value $info(number) }
	-all { 
	    foreach key { name weight number name } { 
		lappend value name $info($key) 
	    }
	}
    }
    return $value
}

# ----------------------------------------------------------------------
# USAGE: select <name> 
#
# Used to manipulate the selection in the table.
#
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicTable::select { what } {
    foreach name [array names _table] { 
	array set info $_table($name)
	if { $what == $info(name) || $what == $info(number) || 
	     $what == $info(symbol) || $what == $info(weight) } {
	    break
	}
	array unset info
    }
    if { ![info exists info] } {
	error "unknown element \"$what\""
    }
    set _current $info(name)
    $_dispatcher event -idle !rebuild
}

# ----------------------------------------------------------------------
# USAGE: _adjust ?<widget>?
#
# This method is invoked each time the dropdown is posted to adjust
# its size and contents.  If the <widget> is specified, it is the
# controlling widget.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicTable::_adjust {{widget ""}} {
    chain $widget

    if 0 {
    set fnt [$itk_component(list) cget -font]
    set maxw 0
    foreach str $_labels {
	set w [font measure $fnt $str]
	if {$w > $maxw} { set maxw $w }
    }
    if {$widget != ""} {
	if {$maxw < [winfo width $widget]} { set maxw [winfo width $widget] }
    }
    set avg [font measure $fnt "n"]
    $itk_component(list) configure -width [expr {round($maxw/double($avg))+1}]

    if {$widget != ""} {
	set y [expr {[winfo rooty $widget]+[winfo height $widget]}]
	set h [font metrics $fnt -linespace]
	set lines [expr {double([winfo screenheight $widget]-$y)/$h}]
	if {[llength $_labels] < $lines} {
	    $itk_component(list) configure -height [llength $_labels]
	} else {
	    $itk_component(list) configure -height 10
	}
    }
    }
    focus $itk_component(table)
}

# ----------------------------------------------------------------------
# USAGE: _react
#
# Invoked automatically when the user has selected something from
# the listbox.  Unposts the drop-down and generates an event letting
# the client know that the selection has changed.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicTable::_react {} {
    unpost
    event generate $itk_component(hull) <<PeriodicTableSelect>>
}

itcl::body Rappture::PeriodicTable::Activate { canvas id x y } {
    set c $itk_component(table)
    array set info $_table($id)
    scan $info(color) "\#%2x%2x%2x" r g b
    incr r -50
    incr g -50
    incr b -50
    set color [format "\#%0.2x%0.2x%0.2x" $r $g $b]
    $c itemconfigure $id-rect -outline black -width 1 -fill $color
    $c itemconfigure $id-number -fill white
    $c itemconfigure $id-name -fill white
    $c itemconfigure $id-symbol -fill white
    $c itemconfigure $id-weight -fill white
    ::Rappture::Tooltip::text $c \
	"$info(name) ($info(symbol))\nNumber: $info(number)\nWeight: $info(weight)"
    ::Rappture::Tooltip::tooltip pending $c @$x,$y
}

itcl::body Rappture::PeriodicTable::Deactivate { canvas id } {
    set c $itk_component(table)
    array set info $_table($id)
    $c itemconfigure $id-rect -outline black -width 1 -fill $info(color)
    $c itemconfigure $id-number -fill black
    $c itemconfigure $id-name -fill black
    $c itemconfigure $id-symbol -fill black
    $c itemconfigure $id-weight -fill black
    ::Rappture::Tooltip::tooltip cancel
}

itcl::body Rappture::PeriodicTable::value {{value "" }} {
    if { $value != "" } {
	set _current $value
    }
}

# ----------------------------------------------------------------------
# USAGE: Redraw
#
# Invoked automatically when the user has selected something from
# the canvas.  Unposts the drop-down and generates an event letting
# the client know that the selection has changed.
# ----------------------------------------------------------------------
itcl::body Rappture::PeriodicTable::Redraw {} {
set sqwidth 30
set sqheight 30
set xoffset 4
set yoffset 4
set last ""
    set c $itk_component(table)
    $c delete all
    foreach name [array names _table] {
	array set info $_table($name)
	set x1 [expr ($info(column)-1)*$sqwidth+$xoffset]
	set y1 [expr ($info(row)-1)*$sqheight+$yoffset]
	set x2 [expr ($info(column)*$sqwidth)-2+$xoffset]
	set y2 [expr ($info(row)*$sqheight)-2+$yoffset]
	#puts stderr symbol=$info(symbol)
	$c create rectangle $x1 $y1 $x2 $y2 -outline black -fill $info(color) \
	    -tags $info(name)-rect
	if { $info(symbol) != "*" } {
	    $c create text [expr ($x2+$x1)/2+1] [expr ($y2+$y1)/2+4] \
		-anchor c \
		-text [string range $info(symbol) 0 4] \
		-font "Arial 6 bold" -tags $info(name)-symbol
	    $c create text [expr $x2-2] [expr $y1+2] -anchor ne \
		-text $info(number) \
		-font "math1 5" -tags $info(name)-number
	}
	$c create rectangle $x1 $y1 $x2 $y2 -outline "" -fill "" \
	    -tags $info(name) 
	$c bind $info(name) <Enter> \
	    [itcl::code $this Activate %W $info(name) %X %Y]
	$c bind $info(name) <Leave> [itcl::code $this Deactivate %W $info(name)]
	$c bind $info(name) <ButtonRelease-1> [itcl::code $this value $info(name)]
    }
    update
    foreach { x1 y1 x2 y2 } [$c bbox all] break
    puts stderr [$c bbox all]
    set width [expr $x2-$x1+$xoffset*2]
    set height [expr $y2-$y1+$yoffset*2]
    $c configure -height $height -width $width -background white
}