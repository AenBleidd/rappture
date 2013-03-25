# -*- mode: tcl; indent-tabs-mode: nil -*- 

# ----------------------------------------------------------------------
#  COMPONENT: field - extracts data from an XML description of a field
#
#  This object represents one field in an XML description of a device.
#  It simplifies the process of extracting data vectors that represent
#  the field.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

# TODO:
#
#  o How to describe vector values in a field?  
#       <components>3</components>
#       <values></values>
#
#    Does anything need to know the limits for each component of the vector?
#

#
# Possible field dataset types:
#
# 2D Datasets
#	vtk		(range of z-axis is zero).
#	unirect2d	(deprecated except where extents > 1)
#	cloud		(x,y point coordinates) (deprecated)
#	mesh 
# 3D Datasets
#	vtk 
#	unirect3d
#	cloud		(x,y,z coordinates) (deprecated)
#	mesh 
#	dx		(FIXME: make dx-to-vtk converter work)
#	ucd avs
#
# Viewers:
#	Format	   Dim  Description			Viewer		Server
#	vtk         2	vtk file data.			contour		vtkvis
#	vtk	    3	vtk file data.			isosurface	vtkvis
#	mesh	    2	points-on-mesh			heightmap	vtkvis
#	mesh	    3	points-on-mesh			isosurface	vtkvis
#	dx	    3	DX				volume		nanovis
#	unirect2d   2	unirect3d + extents > 1	flow	flow		nanovis
#	unirect3d   3	unirect2d + extents > 1	flow	flow		nanovis
#	
# With <views>, can specify which viewer for a specific datasets.  So it's OK
# to if the same dataset can be viewed in more than one way.
#  o Any 2D dataset can be viewed as a contour/heightmap. 
#  o Any 3D dataset can be viewed as a isosurface.  
#  o Any 2D dataset with vector data can be streamlines.  
#  o Any 3D uniform rectilinear dataset can be viewed as a volume.
#  o Any 3D dataset with vector data can be streamlines or flow.
#
# Need <views> to properly do things like qdot: volume with a polydata
# transparent shell.  The view will combine the two objects <field> and
# <drawing> (??) into a single viewer.
#
package require Itcl
package require BLT

namespace eval Rappture { 
    # forward declaration 
}

itcl::class Rappture::Field {
    private variable _dim 0;            # Dimension of the mesh
    private variable _xmlobj "";        # ref to XML obj with field data
    private variable _limits;           # maps axis name => {z0 z1} limits
    private variable _field ""
    private variable _comp2fldName ;	# cname => field names.
    private variable _fld2Components;   # field name => number of components
    private variable _fld2Label;        # field name => label
    private variable _fld2Units;        # field name => units
    private variable _hints 
    private variable _viewer "";        # Hints which viewer to use
    private variable _xv "";            # For 1D meshes only.  Holds the points
    private variable _isValid 0;        # Indicates if the field contains
                                        # valid data.
    private variable _isValidComponent
    constructor {xmlobj path} { 
	# defined below 
    }
    destructor { 
	# defined below 
    }
    public method blob { cname }
    public method components {args}
    public method controls {option args}
    public method extents {{cname -overall}}
    public method fieldlimits {}
    public method flowhints { cname }
    public method hints {{key ""}}
    public method isunirect2d {}
    public method isunirect3d {}
    public method limits {axis}
    public method mesh {{cname -overall}}
    public method style { cname }
    public method type {}
    public method values { cname }
    public method vtkdata {cname}

    public method fieldnames { cname } {
        if { ![info exists _comp2fldName($cname)] } {
            return ""
        }
        return $_comp2fldName($cname)
    }
    public method fieldinfo { fname } {
        lappend out $_fld2Label($fname)
        lappend out $_fld2Units($fname)
        lappend out $_fld2Components($fname)
        return $out
    }
    public method isvalid {} {
        return $_isValid
    }
    public method viewer {} {
	return $_viewer 
    }

    protected method Build {}
    protected method _getValue {expr}

    private variable _path "";          # Path of this object in the XML 
    private variable _units ""   ;      # system of units for this field
    private variable _zmax 0     ;# length of the device

    private variable _comp2dims  ;# maps component name => dimensionality
    private variable _comp2xy    ;# maps component name => x,y vectors
    private variable _comp2vtk   ;# maps component name => vtk file data
    private variable _comp2dx    ;# maps component name => OpenDX data
    private variable _comp2unirect2d ;# maps component name => unirect2d obj
    private variable _comp2unirect3d ;# maps component name => unirect3d obj
    private variable _comp2style ;# maps component name => style settings
    private variable _comp2cntls ;# maps component name => x,y control points
    private variable _comp2extents 
    private variable _comp2limits;	#  Array of limits per component
    private variable _type "" 
    private variable _comp2flowhints 
    private variable _comp2mesh 
    private common _counter 0    ;# counter for unique vector names

    private method BuildPointsOnMesh { cname } 
    private method ConvertToVtkData { cname } 
    private method ReadVtkDataSet { cname contents } 
    private method AvsToVtk { cname contents } 
    private variable _values ""
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Field::constructor {xmlobj path} {
    package require vtk
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
    set _path $path
    set _field [$xmlobj element -as object $path]
    set _units [$_field get units]

    set xunits [$xmlobj get units]
    if {"" == $xunits || "arbitrary" == $xunits} {
        set xunits "um"
    }

    # determine the overall size of the device
    set z0 [set z1 0]
    foreach elem [$_xmlobj children components] {
        switch -glob -- $elem {
            box* {
                if {![regexp {[0-9]$} $elem]} {
                    set elem "${elem}0"
                }
                set z0 [$_xmlobj get components.$elem.corner0]
                set z0 [Rappture::Units::convert $z0 \
                    -context $xunits -to $xunits -units off]

                set z1 [$_xmlobj get components.$elem.corner1]
                set z1 [Rappture::Units::convert $z1 \
                    -context $xunits -to $xunits -units off]

                set _limits($elem) [list $z0 $z1]
            }
        }
    }
    set _zmax $z1

    # build up vectors for various components of the field
    Build
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Field::destructor {} {
    itcl::delete object $_field
    # don't destroy the _xmlobj! we don't own it!

    foreach name [array names _comp2xy] {
        eval blt::vector destroy $_comp2xy($name)
    }
    foreach name [array names _comp2unirect2d] {
        itcl::delete object $_comp2unirect2d($name)
    }
    foreach name [array names _comp2unirect3d] {
        itcl::delete object $_comp2unirect3d($name)
    }
    foreach name [array names _comp2flowhints] {
        itcl::delete object $_comp2flowhints($name)
    }
    foreach name [array names _comp2mesh] {
	# Data is in the form of a mesh and a vector.
	foreach { mesh vector } $_comp2mesh($name) break
	# Release the mesh (may be shared)
        set class [$mesh info class]
        ${class}::release $mesh
	# Destroy the vector
        blt::vector destroy $vector
    }
}

# ----------------------------------------------------------------------
# USAGE: components ?-name|-dimensions|-style? ?<pattern>?
#
# Returns a list of names or types for the various components of
# this field.  If the optional glob-style <pattern> is specified,
# then it returns only the components with names matching the pattern.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::components {args} {
    Rappture::getopts args params {
        flag what -name default
        flag what -dimensions
        flag what -style
        flag what -particles
        flag what -flow
        flag what -box
    }

    set pattern *
    if {[llength $args] > 0} {
        set pattern [lindex $args 0]
        set args [lrange $args 1 end]
    }
    if {[llength $args] > 0} {
        error "wrong # args: should be \"components ?switches? ?pattern?\""
    }

    # There's only one dimension of the field.  Components can't have
    # different dimensions in the same field.  They would by definition be
    # using different meshes and viewers.
    if { $params(what) == "-dimensions" } {
        return "${_dim}D"
    }
    # BE CAREFUL: return component names in proper order
    set rlist ""
    set components [$_field children -type component]
    switch -- $params(what) {
        -name { 
            foreach cname $components {
                if { [info exists _isValidComponent($cname)] } {
                    lappend rlist $cname 
                }
            }
        }
        -style { 
            foreach cname $components {
                if { ![info exists _isValidComponent($cname)] } {
                    continue
                }
                if { [info exists _comp2style($cname)] } {
                    lappend rlist $_comp2style($cname) 
                }
            }
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: mesh ?<name>?
#
# Returns a list {xvec yvec} for the specified field component <name>.
# If the name is not specified, then it returns the vectors for the
# overall field (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Field::mesh {{cname -overall}} {
    if {$cname == "-overall" || $cname == "component0"} {
        set cname [lindex [components -name] 0]
    }
    if {[info exists _comp2xy($cname)]} {
        return [lindex $_comp2xy($cname) 0]  ;# return xv
    }
    if { [info exists _comp2vtk($cname)] } {
	# FIXME: extract mesh from VTK file data.
        if { $_comp2dims($cname) == "1D" } {
            return $_xv
        } 
        error "method \"mesh\" is not implemented for VTK file data"
    }
    if {[info exists _comp2dx($cname)]} {
        return ""  ;# no mesh -- it's embedded in the value data
    }
    if {[info exists _comp2mesh($cname)]} {
        return ""  ;# no mesh -- it's embedded in the value data
    }
    if {[info exists _comp2unirect2d($cname)]} {
        set mobj [lindex $_comp2unirect2d($cname) 0]
        return [$mobj mesh]
    }
    if {[info exists _comp2unirect3d($cname)]} {
        set mobj [lindex $_comp2unirect3d($cname) 0]
        return [$mobj mesh]
    }
    error "can't get field mesh: Unknown component \"$cname\": should be one of [join [lsort [array names _comp2dims]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: values ?<name>?
#
# Returns a list {xvec yvec} for the specified field component <name>.
# If the name is not specified, then it returns the vectors for the
# overall field (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Field::values {cname} {
    if {$cname == "component0"} {
        set cname "component"
    }
    if {[info exists _comp2xy($cname)]} {
        return [lindex $_comp2xy($cname) 1]  ;# return yv
    }
    # VTK file data 
    if { [info exists _comp2vtk($cname)] } {
	# FIXME: extract the values from the VTK file data
        if { $_comp2dims($cname) == "1D" } {
            return $_values
        } 
        error "method \"values\" is not implemented for vtk file data"
    }
    # Points-on-mesh
    if { [info exists _comp2mesh($cname)] } {
	set vector [lindex $_comp2mesh($cname) 1]
        return [$vector range 0 end]
    }
    if {[info exists _comp2dx($cname)]} {
        return $_comp2dx($cname)  ;# return gzipped, base64-encoded DX data
    }
    if {[info exists _comp2unirect2d($cname)]} {
        return [$_comp2unirect2d($cname) values]
    }
    if {[info exists _comp2unirect3d($cname)]} {
        return [$_comp2unirect3d($cname) blob]
    }
    error "can't get field values. Unknown component \"$cname\": should be [join [lsort [array names _comp2dims]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: blob ?<name>?
#
# Returns a string representing the blob of data for the mesh and values.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::blob {cname} {
    if {$cname == "component0"} {
        set cname "component"
    }
    if {[info exists _comp2xy($cname)]} {
        return ""
    }
    if { [info exists _comp2vtk($cname)] } {
	error "blob not implemented for VTK file data"
    }
    if {[info exists _comp2dx($cname)]} {
        return $_comp2dx($cname)  ;# return gzipped, base64-encoded DX data
    }
    if {[info exists _comp2unirect2d($cname)]} {
        set blob [$_comp2unirect2d($cname) blob]
        lappend blob "values" $_values
        return $blob
    }
    if {[info exists _comp2unirect3d($cname)]} {
        return [$_comp2unirect3d($cname) blob]
    }
    error "can't get field blob: Unknown component \"$cname\": should be one of [join [lsort [array names _comp2dims]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: limits <axis>
#
# Returns a list {min max} representing the limits for the specified
# axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::limits {which} {
    set min ""
    set max ""
    blt::vector tmp zero

    foreach cname [array names _comp2dims] {
        switch -- $_comp2dims($cname) {
            1D {
                switch -- $which {
                    x - xlin { 
			set pos 0; set log 0; set axis x 
		    }
                    xlog { 
			set pos 0; set log 1; set axis x 
		    }
                    y - ylin - v - vlin { 
			set pos 1; set log 0; set axis y 
		    }
                    ylog - vlog { 
			set pos 1; set log 1; set axis y 
		    }
                    default {
                        error "bad axis \"$which\": should be x, xlin, xlog, y, ylin, ylog, v, vlin, vlog"
                    }
                }

                set vname [lindex $_comp2xy($cname) $pos]
                $vname variable vec

                if {$log} {
                    # on a log scale, use abs value and ignore 0's
                    $vname dup tmp
                    $vname dup zero
                    zero expr {tmp == 0}            ;# find the 0's
                    tmp expr {abs(tmp)}             ;# get the abs value
                    tmp expr {tmp + zero*max(tmp)}  ;# replace 0's with abs max
                    set axisMin [blt::vector expr min(tmp)]
                    set axisMax [blt::vector expr max(tmp)]
                } else {
                    set axisMin $vec(min)
                    set axisMax $vec(max)
                }

                if {"" == $min} {
                    set min $axisMin
                } elseif {$axisMin < $min} {
                    set min $axisMin
                }
                if {"" == $max} {
                    set max $axisMax
                } elseif {$axisMax > $max} {
                    set max $axisMax
                }
            }
            2D - 3D {
                if {[info exists _comp2unirect3d($cname)]} {
                    set limits [$_comp2unirect3d($cname) limits $which]
                    foreach {axisMin axisMax} $limits break
                    set axis v
                } elseif {[info exists _comp2limits($cname)]} {
		    array set limits $_comp2limits($cname) 
		    switch -- $which {
                        x - xlin - xlog {
                            set axis x
			    foreach {axisMin axisMax} $limits(x) break
                        }
                        y - ylin - ylog {
                            set axis y
			    foreach {axisMin axisMax} $limits(y) break
                        }
                        z - zlin - zlog {
                            set axis z
			    foreach {axisMin axisMax} $limits(z) break
                        }
                        v - vlin - vlog {
                            set axis v
			    foreach {axisMin axisMax} $limits(v) break
			}
			default {
			    if { ![info exists limits($which)] } {
				error "limits: unknown axis \"$which\""
			    }
                            set axis v
			    foreach {axisMin axisMax} $limits($which) break
			}
		    }
                } else {
                    set axisMin 0  ;# HACK ALERT! must be OpenDX data
                    set axisMax 1
                    set axis v
                }
            }
        }
        if { "" == $min || $axisMin < $min } {
            set min $axisMin
        }
        if { "" == $max || $axisMax > $max } {
            set max $axisMax
        }
    }
    blt::vector destroy tmp zero
    set val [$_field get "${axis}axis.min"]
    if {"" != $val && "" != $min} {
        if {$val > $min} {
            # tool specified this min -- don't go any lower
            set min $val
        }
    }
    set val [$_field get "${axis}axis.max"]
    if {"" != $val && "" != $max} {
        if {$val < $max} {
            # tool specified this max -- don't go any higher
            set max $val
        }
    }
    return [list $min $max]
}


# ----------------------------------------------------------------------
# USAGE: fieldlimits
#
# Returns a list {min max} representing the limits for the specified
# axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::fieldlimits {} {
    foreach cname [array names _comp2limits] {
        array set limits $_comp2limits($cname) 
        foreach fname $_comp2fldName($cname) {
            if { ![info exists limits($fname)] } {
                puts stderr "field \"$fname\" unknown in \"$cname\""
                continue
            }
            foreach {min max} $limits($fname) break
            if { ![info exists overall($fname)] } {
                set overall($fname) $limits($fname)
                continue
            }
            foreach {omin omax} $overall($fname) break
            if { $min < $omin } {
                set omin $min
            }
            if { $max > $omax } {
                set omax $max
            }
            set overall($fname) [list $min $max]
        }
    }
    if { [info exists overall] } {
        return [array get overall]
    }
    return ""
}
 
# ----------------------------------------------------------------------
# USAGE: controls get ?<name>?
# USAGE: controls validate <path> <value>
# USAGE: controls put <path> <value>
#
# Returns a list {path1 x1 y1 val1  path2 x2 y2 val2 ...} representing
# control points for the specified field component <name>.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::controls {option args} {
    switch -- $option {
        get {
            set cname [lindex $args 0]
            if {[info exists _comp2cntls($cname)]} {
                return $_comp2cntls($cname)
            }
            return ""
        }
        validate {
            set path [lindex $args 0]
            set value [lindex $args 1]
            set units [$_xmlobj get $path.units]

            if {"" != $units} {
                set nv [Rappture::Units::convert \
                    $value -context $units -to $units -units off]
            } else {
                set nv $value
            }
            if {![string is double $nv]
                  || [regexp -nocase {^(inf|nan)$} $nv]} {
                error "Value out of range"
            }

            set rawmin [$_xmlobj get $path.min]
            if {"" != $rawmin} {
                set minv $rawmin
                if {"" != $units} {
                    set minv [Rappture::Units::convert \
                        $minv -context $units -to $units -units off]
                    set nv [Rappture::Units::convert \
                        $value -context $units -to $units -units off]
                }
                # fix for the case when the user tries to
                # compare values like minv=-500 nv=-0600
                set nv [format "%g" $nv]
                set minv [format "%g" $minv]

                if {$nv < $minv} {
                    error "Minimum value allowed here is $rawmin"
                }
            }

            set rawmax [$_xmlobj get $path.max]
            if {"" != $rawmax} {
                set maxv $rawmax
                if {"" != $units} {
                    set maxv [Rappture::Units::convert \
                        $maxv -context $units -to $units -units off]
                    set nv [Rappture::Units::convert \
                        $value -context $units -to $units -units off]
                }
                # fix for the case when the user tries to
                # compare values like maxv=-500 nv=-0600
                set nv [format "%g" $nv]
                set maxv [format "%g" $maxv]

                if {$nv > $maxv} {
                    error "Maximum value allowed here is $rawmax"
                }
            }

            return "ok"
        }
        put {
            set path [lindex $args 0]
            set value [lindex $args 1]
            $_xmlobj put $path.current $value
            Build
        }
        default {
            error "bad field controls option \"$option\": should be get or put"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this field.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::hints {{keyword ""}} {
    if { ![info exists _hints] } {
	foreach {key path} {
	    camera          camera.position
	    color           about.color
	    default         about.default
	    group           about.group
	    label           about.label
	    scale           about.scale
	    seeds           about.seeds
	    style           about.style
	    type            about.type
	    xlabel          about.xaxis.label
	    ylabel          about.yaxis.label
	    zlabel          about.zaxis.label
	    xunits          about.xaxis.units
	    yunits          about.yaxis.units
	    zunits          about.zaxis.units
	    units           units
	    updir           updir
	    vectors         about.vectors
	} {
	    set str [$_field get $path]
	    if { "" != $str } {
		set _hints($key) $str
	    }
	}
	foreach {key path} {
	    toolid          tool.id
	    toolname        tool.name
	    toolcommand     tool.execute
	    tooltitle       tool.title
	    toolrevision    tool.version.application.revision
	} {
	    set str [$_xmlobj get $path]
	    if { "" != $str } {
		set _hints($key) $str
	    }
	}
	# Set toolip and path hints
	set _hints(path) $_path
	if { [info exists _hints(group)] && [info exists _hints(label)] } {
	    # pop-up help for each curve
	    set _hints(tooltip) $_hints(label)
	}
    }
    if { $keyword != "" } {
        if {[info exists _hints($keyword)]} {
            return $_hints($keyword)
        }
        return ""
    }
    return [array get _hints]
}

# ----------------------------------------------------------------------
# USAGE: Build
#
# Used internally to build up the vector representation for the
# field when the object is first constructed, or whenever the field
# data changes.  Discards any existing vectors and builds everything
# from scratch.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::Build {} {

    # Discard any existing data
    foreach name [array names _comp2xy] {
        eval blt::vector destroy $_comp2xy($name)
    }
    array unset _comp2vtk
    foreach name [array names _comp2unirect2d] {
        eval itcl::delete object $_comp2unirect2d($name)
    }
    foreach name [array names _comp2unirect3d] {
        eval itcl::delete object $_comp2unirect3d($name)
    }
    catch {unset _comp2xy}
    catch {unset _comp2dx}
    catch {unset _comp2dims}
    catch {unset _comp2style}
    array unset _comp2unirect2d
    array unset _comp2unirect3d
    array unset _comp2extents
    array unset _dataobj2type
    #
    # Scan through the components of the field and create
    # vectors for each part.
    #
    array unset _isValidComponent
    foreach cname [$_field children -type component] {
        set type ""
        if { ([$_field element $cname.constant] != "" &&
	      [$_field element $cname.domain] != "") ||
	      [$_field element $cname.xy] != "" } {
            set type "1D"
        } elseif { [$_field element $cname.mesh] != "" &&
		   [$_field element $cname.values] != ""} {
            set type "points-on-mesh"
        } elseif { [$_field element $cname.vtk] != ""} {
	    set viewer [$_field get "about.view"]
	    set type "vtk"
	    if { $viewer != "" } {
		set _viewer $viewer
	    }
        } elseif {[$_field element $cname.opendx] != ""} {
            global env
            if { [info exists env(VTKVOLUME)] } {
                set type "vtkvolume"
            } else {
                set type "opendx"
            }
        } elseif {[$_field element $cname.dx] != ""} {
            global env
            if { [info exists env(VTKVOLUME)] } {
                set type "vtkvolume"
            } else {
                set type "dx"
            }
        } elseif {[$_field element $cname.ucd] != ""} {
            set type "ucd"
	}
        set _comp2style($cname) ""
        if { $type == "" } {
            puts stderr "ignoring field component \"$cname\": no data found"
            continue
        }
        # Save the extents of the component
        if { [$_field element $cname.extents] != "" } {
            set extents [$_field get $cname.extents]
        } else {
            set extents 1 
        }
        set _comp2extents($cname) $extents
        set _type $type
        if {$type == "1D"} {
            #
            # 1D data can be represented as 2 BLT vectors,
            # one for x and the other for y.
            #
            set xv ""
            set yv ""

            set val [$_field get $cname.constant]
            if {$val != ""} {
                set domain [$_field get $cname.domain]
                if {$domain == "" || ![info exists _limits($domain)]} {
                    set z0 0
                    set z1 $_zmax
                } else {
                    foreach {z0 z1} $_limits($domain) { break }
                }
                set xv [blt::vector create x$_counter]
                $xv append $z0 $z1

                foreach {val pcomp} [_getValue $val] break
                set yv [blt::vector create y$_counter]
                $yv append $val $val

                if {$pcomp != ""} {
                    set zm [expr {0.5*($z0+$z1)}]
                    set _comp2cntls($cname) \
                        [list $pcomp $zm $val "$val$_units"]
                }
            } else {
                set xydata [$_field get $cname.xy]
                if {"" != $xydata} {
                    set xv [blt::vector create x$_counter]
                    set yv [blt::vector create y$_counter]
                    set tmp [blt::vector create \#auto]
                    $tmp set $xydata
                    $tmp split $xv $yv
                    blt::vector destroy $tmp
                }
            }

            if {$xv != "" && $yv != ""} {
                # sort x-coords in increasing order
                $xv sort $yv
                set _comp2dims($cname) "1D"
                set _comp2xy($cname) [list $xv $yv]
                incr _counter
            }
        } elseif {$type == "points-on-mesh"} {
	    if { ![BuildPointsOnMesh $cname] } {
                continue;               # Ignore this component
            }
        } elseif {$type == "vtk"} {
            set contents [$_field get $cname.vtk]
            if { $contents == "" } {
                puts stderr "no data fo \"$cname.vtk\""
                continue;               # Ignore this component
            }
            ReadVtkDataSet $cname $contents
            set _comp2vtk($cname) $contents
            set _comp2style($cname) [$_field get $cname.style]
            incr _counter
        } elseif {$type == "dx" || $type == "opendx" } {
            #
            # HACK ALERT!  Extract gzipped, base64-encoded OpenDX
            # data.  Assume that it's 3D.  Pass it straight
            # off to the NanoVis visualizer.
            #
	    set _viewer "nanovis"
            set _dim 3
            set _type "dx"
            set _comp2dims($cname) "3D"
            set contents [$_field get -decode no $cname.$type]
            if { $contents == "" } {
                puts stderr "no data for \"$cname.$type\""
                continue;               # Ignore this component
            }
            set _comp2dx($cname) $contents
            if 0 {
                set hdr "@@RP-ENC:zb64\n"
                set data  [$_field get -decode no $cname.$type]
                set data "$hdr$data"
                set data  [Rappture::encoding::decode $data]
                set data  [Rappture::DxToVtk $data]
                set f [open /tmp/$_path.$cname.vtk "w"]
                puts $f $data
                close $f
            }
            set _comp2style($cname) [$_field get $cname.style]
            if {[$_field element $cname.flow] != ""} {
                set _comp2flowhints($cname) \
                    [Rappture::FlowHints ::\#auto $_field $cname $_units]
            }
            incr _counter
        } elseif { $type == "ucd"} {
            set contents [$_field get $cname.ucd]
            if { $contents == "" } {
                continue;               # Ignore this compoennt
            }
            set vtkdata [AvsToVtk $cname $contents]
            ReadVtkDataSet $cname $vtkdata
            set _comp2vtk($cname) $vtkdata
            set _comp2style($cname) [$_field get $cname.style]
            incr _counter
        }
        set _isValidComponent($cname) 1
    }
    if { [array size _isValidComponent] == 0 } {
        puts stderr "no valid components for field \"$_path\""
        return 0
    }
    # Sanity check.  Verify that all components of the field have the same 
    # dimension.
    set dim ""
    foreach cname [array names _comp2dims] {
        if { $dim == "" } {
            set dim $_comp2dims($cname)
            continue
        }
        if { $dim != $_comp2dims($cname) } {
            puts stderr "field can't have components of different dimensions: [join [array get _comp2dims] ,]"
            return 0
        }
    }
    # FIXME: about.scalars and about.vectors are temporary.  With views
    #        the label and units for each field will be specified there.
    #
    # FIXME: Test that every <field><component> has the same field names, 
    #        units, components.
    #
    # Override what we found in the VTK file with names that the user
    # selected.  We override the field label and units.
    foreach { fname label units } [$_field get about.scalars] {
        if { ![info exists _fld2Name($fname)] } {
            set _fld2Name($fname) $fname
            set _fld2Components($fname) 1
        }
        set _fld2Label($fname) $label
        set _fld2Units($fname) $units
    }
    foreach { fname label units } [$_field get about.vectors] {
        if { ![info exists _fld2Name($fname)] } {
            set _fld2Name($fname) $fname
            # We're just marking the field as vector (> 1) for now.
            set _fld2Components($fname) 3
        }
        set _fld2Label($fname) $label
        set _fld2Units($fname) $units
    }
    set _isValid 1
    return 1
}

# ----------------------------------------------------------------------
# USAGE: _getValue <expr>
#
# Used internally to get the value for an expression <expr>.  Returns
# a list of the form {val parameterPath}, where val is the numeric
# value of the expression, and parameterPath is the XML path to the
# parameter representing the value, or "" if the <expr> does not
# depend on any parameters.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::_getValue {expr} {
    #
    # First, look for the expression among the <parameter>'s
    # associated with the device.
    #
    set found 0
    foreach pcomp [$_xmlobj children parameters] {
        set id [$_xmlobj element -as id parameters.$pcomp]
        if {[string equal $id $expr]} {
            set val [$_xmlobj get parameters.$pcomp.current]
            if {"" == $val} {
                set val [$_xmlobj get parameters.$pcomp.default]
            }
            if {"" != $val} {
                set expr $val
                set found 1
                break
            }
        }
    }
    if {$found} {
        set pcomp "parameters.$pcomp"
    } else {
        set pcomp ""
    }

    if {$_units != ""} {
        set expr [Rappture::Units::convert $expr \
            -context $_units -to $_units -units off]
    }

    return [list $expr $pcomp]
}

#
# isunirect2d  --
#
# Returns if the field is a unirect2d object.  
#
itcl::body Rappture::Field::isunirect2d { } {
    return [expr [array size _comp2unirect2d] > 0]
}

#
# isunirect3d  --
#
# Returns if the field is a unirect3d object.  
#
itcl::body Rappture::Field::isunirect3d { } {
    return [expr [array size _comp2unirect3d] > 0]
}

#
# flowhints  --
#
# Returns the hints associated with a flow vector field.  
#
itcl::body Rappture::Field::flowhints { cname } {
    if { [info exists _comp2flowhints($cname)] } {
        return $_comp2flowhints($cname)
    }
    return ""
}

#
# style  --
#
# Returns the style associated with a component of the field.  
#
itcl::body Rappture::Field::style { cname } {
    if { [info exists _comp2style($cname)] } {
        return $_comp2style($cname)
    }
    return ""
}

#
# type  --
#
# Returns the style associated with a component of the field.  
#
itcl::body Rappture::Field::type {} {
    return $_type
}

#
# extents --
#
# Returns if the field is a unirect2d object.  
#
itcl::body Rappture::Field::extents {{cname -overall}} {
    if {$cname == "-overall" } {
        set max 0
        foreach cname [$_field children -type component] {
            if { ![info exists _comp2unirect3d($cname)] &&
                 ![info exists _comp2extents($cname)] } {
                continue
            }
            set value $_comp2extents($cname)
            if { $max < $value } {
                set max $value
            }
        }
        return $max
    } 
    if { $cname == "component0"} {
        set cname [lindex [components -name] 0]
    }
    return $_comp2extents($cname)
}

itcl::body Rappture::Field::ConvertToVtkData { cname } {
    set ds ""
    switch -- [typeof $cname] {
	"unirect2d" {
	    foreach { x1 x2 xN y1 y2 yN } [$dataobj mesh $cname] break
	    set spacingX [expr {double($x2 - $x1)/double($xN - 1)}]
	    set spacingY [expr {double($y2 - $y1)/double($yN - 1)}]
	    
	    set ds [vtkImageData $this-grdataTemp]
	    $ds SetDimensions $xN $yN 1
	    $ds SetOrigin $x1 $y1 0
	    $ds SetSpacing $spacingX $spacingY 0
	    set arr [vtkDoubleArray $this-arrTemp]
	    foreach {val} [$dataobj values $cname] {
		$arr InsertNextValue $val
	    }
	    [$ds GetPointData] SetScalars $arr
	}
	"unirect3d" {
	    foreach { x1 x2 xN y1 y2 yN z1 z2 zN } [$dataobj mesh $cname] break
	    set spacingX [expr {double($x2 - $x1)/double($xN - 1)}]
	    set spacingY [expr {double($y2 - $y1)/double($yN - 1)}]
	    set spacingZ [expr {double($z2 - $z1)/double($zN - 1)}]
	    
	    set ds [vtkImageData $this-grdataTemp]
	    $ds SetDimensions $xN $yN $zN
	    $ds SetOrigin $x1 $y1 $z1
	    $ds SetSpacing $spacingX $spacingY $spacingZ
	    set arr [vtkDoubleArray $this-arrTemp]
	    foreach {val} [$dataobj values $cname] {
		$arr InsertNextValue $val
	    }
	    [$ds GetPointData] SetScalars $val
	}
	"contour" {
	    return [$dataobj blob $cname]
	}
	"dx" {
            return [Rappture::DxToVtk $_comp2dx($cname)]
	}
	default {
	    set mesh [$dataobj mesh $cname]
	    switch -- [$mesh GetClassName] {
		vtkPoints {
		    # handle cloud of points
		    set ds [vtkPolyData $this-polydataTemp]
		    $ds SetPoints $mesh
		    [$ds GetPointData] SetScalars [$dataobj values $cname]
		}
		vtkPolyData {
		    set ds [vtkPolyData $this-polydataTemp]
		    $ds ShallowCopy $mesh
		    [$ds GetPointData] SetScalars [$dataobj values $cname]
		}
		vtkUnstructuredGrid {
		    # handle 3D grid with connectivity
		    set ds [vtkUnstructuredGrid $this-grdataTemp]
		    $ds ShallowCopy $mesh
		    [$ds GetPointData] SetScalars [$dataobj values $cname]
		}
		vtkRectilinearGrid {
		    # handle 3D grid with connectivity
		    set ds [vtkRectilinearGrid $this-grdataTemp]
		    $ds ShallowCopy $mesh
		    [$ds GetPointData] SetScalars [$dataobj values $cname]
		}
		default {
		    error "don't know how to handle [$mesh GetClassName] data"
		}
	    }
	}
    }

    if {"" != $ds} {
        set writer [vtkDataSetWriter $this-dsWriterTmp]
        $writer SetInput $ds
        $writer SetFileTypeToASCII
        $writer WriteToOutputStringOn
        $writer Write
        set out [$writer GetOutputString]
        $ds Delete
        $writer Delete
    } else {
        set out ""
        error "No DataSet to write"
    }

    append out "\n"
    return $out
}

itcl::body Rappture::Field::ReadVtkDataSet { cname contents } {
    package require vtk

    set reader $this-datasetreader
    vtkDataSetReader $reader

    # Write the contents to a file just in case it's binary.
    set tmpfile file[pid].vtk
    set f [open "$tmpfile" "w"]
    fconfigure $f -translation binary -encoding binary
    puts $f $contents 
    close $f

    $reader SetFileName $tmpfile
    $reader ReadAllScalarsOn
    $reader ReadAllVectorsOn
    $reader ReadAllFieldsOn
    $reader Update
    set dataset [$reader GetOutput]
    set limits {}
    foreach {xmin xmax ymin ymax zmin zmax} [$dataset GetBounds] break
    # Figure out the dimension of the mesh from the bounds.
    set _dim 0
    if { $xmax > $xmin } {
	incr _dim
    }
    if { $ymax > $ymin } {
	incr _dim
    }
    if { $zmax > $zmin } {
	incr _dim
    }
    if { $_viewer == "" } {
	if { $_dim == 2 } {
	    set _viewer contour
	} else {
	    set _viewer isosurface
	}
    }
    set _comp2dims($cname) ${_dim}D
    if { $_dim < 2 } {
        set points [$dataset GetPoints]
        set numPoints [$points GetNumberOfPoints]
        set xv [blt::vector create \#auto]
        for { set i 0 } { $i < $numPoints } { incr i } {
            set point [$points GetPoint 0]
            $xv append [lindex $point 0] 
        }
        set yv [blt::vector create \#auto]
        $yv seq 0 1 [$xv length]
        set _comp2xy($cname) [list $xv $yv]
    }
    lappend limits x [list $xmin $xmax] 
    lappend limits y [list $ymin $ymax] 
    lappend limits z [list $zmin $zmax]
    set dataAttrs [$dataset GetPointData]
    if { $_dim == 1 } {
        set numArrays [$dataAttrs GetNumberOfArrays]
    }
    if { $dataAttrs == ""} {
	puts stderr "No point data found in \"$_path\""
        return 0
    }
    set vmin 0
    set vmax 1
    set numArrays [$dataAttrs GetNumberOfArrays]
    if { $numArrays > 0 } {
	set array [$dataAttrs GetArray 0]
	foreach {vmin vmax} [$array GetRange] break

	for {set i 0} {$i < [$dataAttrs GetNumberOfArrays] } {incr i} {
	    set array [$dataAttrs GetArray $i]
	    set fname  [$dataAttrs GetArrayName $i]
	    foreach {min max} [$array GetRange] break
	    lappend limits $fname [list $min $max]
            set _fld2Units($fname) ""
	    set _fld2Label($fname) $fname
            set _fld2Components($fname) [$array GetNumberOfComponents]
            lappend _comp2fldName($cname) $fname
	}
    }
    lappend limits v [list $vmin $vmax]
    set _comp2limits($cname) $limits
    file delete $tmpfile
    rename $reader ""
}

#
# vtkdata --
#
#	Returns a string representing the mesh and field data for a specific
#	component in the legacy VTK file format.
#
itcl::body Rappture::Field::vtkdata {cname} {
    if {$cname == "component0"} {
        set cname "component"
    }
    # DX: Convert DX to VTK 
    if {[info exists _comp2dx($cname)]} {
	return [Rappture::DxToVtk $_comp2dx($cname)]
    }
    # Unirect3d: isosurface 
    if {[info exists _comp2unirect3d($cname)]} {
        return [$_comp2unirect3d($cname) vtkdata]
    }
    # VTK file data: 
    if { [info exists _comp2vtk($cname)] } {
        return $_comp2vtk($cname)
    }
    # Points on mesh:  Construct VTK file output.
    if { [info exists _comp2mesh($cname)] } {
	# Data is in the form mesh and vector
	foreach {mesh vector} $_comp2mesh($cname) break
        set label $cname
        regsub -all { } $label {_} label
	append out "# vtk DataFile Version 3.0\n"
	append out "[hints label]\n"
	append out "ASCII\n"
	append out [$mesh vtkdata]
	append out "POINT_DATA [$vector length]\n"
	append out "SCALARS $label double 1\n"
	append out "LOOKUP_TABLE default\n"
	append out "[$vector range 0 end]\n"
	return $out
    }
    error "can't find vtkdata for $cname. This method should only be called by the vtkheightmap widget"
}

#
# BuildPointsOnMesh --
#
#	Parses the field XML description to build a mesh and values vector
#	representing the field.  Right now we handle the deprecated types
#	of "cloud", "unirect2d", and "unirect3d" (mostly for flows).
#
itcl::body Rappture::Field::BuildPointsOnMesh {cname} {
    #
    # More complex 2D/3D data is represented by a mesh
    # object and an associated vector for field values.
    #
    set path [$_field get $cname.mesh]
    if {[$_xmlobj element $path] == ""} {
	# Unknown mesh designated.
	return 0
    }
    set _viewer [$_field get "about.view"]
    set element [$_xmlobj element -as type $path]
    set name $cname
    regsub -all { } $name {_} name
    set _fld2Label($name) $name
    set label [hints zlabel]
    if { $label != "" } {
        set _fld2Label($name) $label
    }
    set _fld2Units($name) [hints zunits]
    set _fld2Components($name) 1
    lappend _comp2fldName($cname) $name

    # Handle bizarre cases that hopefully will be deprecated.
    if { $element == "unirect3d" } {
	# Special case: unirect3d (should be deprecated) + flow.
        if { [$_field element $cname.extents] != "" } {
            set extents [$_field get $cname.extents]
        } else {
            set extents 1 
        }
	set _dim 3
        if { $_viewer == "" } {
            set _viewer flowvis
        }
	set _comp2dims($cname) "3D"
	set _comp2unirect3d($cname) \
	    [Rappture::Unirect3d \#auto $_xmlobj $_field $cname $extents]
	set _comp2style($cname) [$_field get $cname.style]
	if {[$_field element $cname.flow] != ""} {
	    set _comp2flowhints($cname) \
		[Rappture::FlowHints ::\#auto $_field $cname $_units]
	}
	incr _counter
	return 1
    }
    if { $element == "unirect2d" && [$_field element $cname.flow] != "" } {
	# Special case: unirect2d (normally deprecated) + flow.
        if { [$_field element $cname.extents] != "" } {
            set extents [$_field get $cname.extents]
        } else {
            set extents 1 
        }
	set _dim 2
        if { $_viewer == "" } {
            set _viewer "flowvis"
        }
	set _comp2dims($cname) "2D"
	set _comp2unirect2d($cname) \
	    [Rappture::Unirect2d \#auto $_xmlobj $path]
	set _comp2style($cname) [$_field get $cname.style]
	set _comp2flowhints($cname) \
	    [Rappture::FlowHints ::\#auto $_field $cname $_units]
	set _values [$_field get $cname.values]
        set limits {}
        foreach axis { x y } {
            lappend limits $axis [$_comp2unirect2d($cname) limits $axis]
        }
        set xv [blt::vector create \#auto]
        $xv set $_values
        lappend limits $cname [$xv limits]
        lappend limits v [$xv limits]
        blt::vector destroy $xv
        set _comp2limits($cname) $limits
	incr _counter
	return 1
    }
    switch -- $element {
	"cloud" {
	    set mesh [Rappture::Cloud::fetch $_xmlobj $path]
	}
	"mesh" {
	    set mesh [Rappture::Mesh::fetch $_xmlobj $path]
	}	    
	"unirect2d" {
            if { $_viewer == "" } {
                set _viewer "heightmap"
            }
	    set mesh [Rappture::Unirect2d::fetch $_xmlobj $path]
	}
    }
    if { ![$mesh isvalid] } {
        return 0
    }
    set _dim [$mesh dimensions]
    if {$_dim == 1} {
	# Is this used anywhere?
	#
	# OOPS!  This is 1D data
	# Forget the cloud/field -- store BLT vectors
	#
	# Is there a natural growth path in generating output from 1D to 
	# higher dimensions?  If there isn't, let's kill this in favor
	# or explicitly using a <curve> instead.  Otherwise, the features
	# (methods such as xmarkers) or the <curve> need to be added 
	# to the <field>.
	# 
	set xv [blt::vector create x$_counter]
	set yv [blt::vector create y$_counter]
	
	$yv set [$mesh points]
	$xv seq 0 1 [$yv length]
	# sort x-coords in increasing order
	$xv sort $yv
	
	set _comp2dims($cname) "1D"
	set _comp2xy($cname) [list $xv $yv]
	incr _counter
	return 1
    } 
    if {$_dim == 2} {
	set _type "heightmap"
	set v [blt::vector create \#auto]
	$v set [$_field get $cname.values]
        if { [$v length] == 0 } {
            return 0
        }
	set _comp2dims($cname) "[$mesh dimensions]D"
	set _comp2mesh($cname) [list $mesh $v]
	set _comp2style($cname) [$_field get $cname.style]
	incr _counter
	array unset _comp2limits $cname
	lappend _comp2limits($cname) x [$mesh limits x]
	lappend _comp2limits($cname) y [$mesh limits y]
	lappend _comp2limits($cname) $cname [$v limits]
	lappend _comp2limits($cname) v [$v limits]
	return 1
    } 
    if {$_dim == 3} {
	#
	# 3D data: Store cloud/field as components
	#
	set values [$_field get $cname.values]
	set farray [vtkFloatArray ::vals$_counter]
	foreach v $values {
	    if {"" != $_units} {
		set v [Rappture::Units::convert $v \
			   -context $_units -to $_units -units off]
	    }
	    $farray InsertNextValue $v
	}
        if { $_viewer == "" } {
            set _viewer "isosurface"
        }
	set _type "isosurface"
	set v [blt::vector create \#auto]
	$v set [$_field get $cname.values]
        if { [$v length] == 0 } {
            return 0
        }
        set _comp2dims($cname) "[$mesh dimensions]D"
        set _comp2mesh($cname) [list $mesh $v]
        set _comp2style($cname) [$_field get $cname.style]
        incr _counter
        lappend _comp2limits($cname) x [$mesh limits x]
        lappend _comp2limits($cname) y [$mesh limits y]
        lappend _comp2limits($cname) z [$mesh limits z]
        lappend _comp2limits($cname) $cname [$v limits]
        lappend _comp2limits($cname) v [$v limits]
	return 1
    }
    error "unhandled case in field dim=$_dim element=$element"
}

itcl::body Rappture::Field::AvsToVtk { cname contents } {
    package require vtk

    set reader $this-datasetreader
    vtkAVSucdReader $reader

    # Write the contents to a file just in case it's binary.
    set tmpfile $cname[pid].ucd
    set f [open "$tmpfile" "w"]
    fconfigure $f -translation binary -encoding binary
    puts $f $contents
    close $f
    $reader SetFileName $tmpfile
    $reader Update
    file delete $tmpfile

    set output [$reader GetOutput]
    set pointData [$output GetPointData]
    set _scalars {}
    for { set i 0 } { $i < [$pointData GetNumberOfArrays] } { incr i } {
        set name [$pointData GetArrayName $i]
	lappend _scalars $name $name "???"
    }
    set tmpfile $cname[pid].vtk
    set writer $this-datasetwriter
    vtkDataSetWriter $writer
    $writer SetInputConnection [$reader GetOutputPort]
    $writer SetFileName $tmpfile
    $writer Write
    rename $reader ""
    rename $writer ""

    set f [open "$tmpfile" "r"]
    fconfigure $f -translation binary -encoding binary
    set vtkdata [read $f]
    close $f
    file delete $tmpfile
    return $vtkdata
}
