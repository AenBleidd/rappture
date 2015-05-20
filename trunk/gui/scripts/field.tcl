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
#    Vector field limits are wrong: need to compute magnitude limits and
#    component-wise limits.

#
# Possible field dataset types:
#
# 1D Datasets
#	1D		A field contained in a structure
# 2D Datasets
#	vtk		(range of z-axis is zero).
#	unirect2d	(deprecated)
#	cloud		(x,y point coordinates) (deprecated)
#	mesh
# 3D Datasets
#	vtk
#	unirect3d       (deprecated)
#	cloud		(x,y,z coordinates) (deprecated)
#	mesh
#	dx		(FIXME: make dx-to-vtk converter work)
#	ucd		(AVS ucd format)
#
# Viewers:
#	Format	   Dim  Description			Viewer		Server
#	1D          1   structure field                 DeviceViewer1D  N/A
#	vtk         2	vtk file data.			contour		vtkvis
#	vtk	    3	vtk file data.			isosurface	vtkvis
#	mesh	    2	points-on-mesh			heightmap	vtkvis
#	mesh	    3	points-on-mesh			isosurface	vtkvis
#	dx	    3	DX				volume		nanovis
#	unirect2d   2	unirect2d + extents > 1	flow	flow		nanovis
#	unirect3d   3	unirect3d + extents > 1	flow	flow		nanovis
#
# With <views>, can specify which viewer for specific datasets.  So it's OK
# for the same dataset to be viewed in more than one way.
#  o Any 2D dataset can be viewed as a contour/heightmap.
#  o Any 3D dataset can be viewed as a isosurface.
#  o Any 2D dataset with vector data can be streamlines or flow.
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
    constructor {xmlobj path} {
        # defined below
    }
    destructor {
        # defined below
    }
    public method blob { cname }
    public method components {args}
    public method controls {option args}
    public method fieldinfo { fname } {
        lappend out $_fld2Label($fname)
        lappend out $_fld2Units($fname)
        lappend out $_fld2Components($fname)
        return $out
    }
    public method fieldlimits {}
    public method fieldnames { cname } {
        if { ![info exists _comp2fldName($cname)] } {
            return ""
        }
        return $_comp2fldName($cname)
    }
    public method flowhints { cname }
    public method hints {{key ""}}
    public method isunirect2d {}
    public method isunirect3d {}
    public method isvalid {} {
        return $_isValid
    }
    public method limits {axis}
    public method mesh {{cname -overall}}
    public method numComponents {cname}
    public method style { cname }
    public method type {}
    public method valueLimits { cname }
    public method values { cname }
    public method viewer {} {
        return $_viewer
    }
    public method vtkdata {cname}
    public method xErrorValues { cname } {
    }
    public method yErrorValues { cname } {
    }

    protected method Build {}
    protected method _getValue {expr}
    protected method GetAssociation { cname }
    protected method GetTypeAndSize { cname }
    protected method ReadVtkDataSet { cname contents }

    private method AvsToVtk { cname contents }
    private method DicomToVtk { cname contents }
    private method BuildPointsOnMesh { cname }
    private method InitHints {}
    private method VerifyVtkDataSet { contents }
    private method VectorLimits { vector vectorsize {comp -1} }
    private method VtkDataSetToXy { dataset }

    protected variable _dim 0;          # Dimension of the mesh

    private variable _xmlobj "";        # ref to XML obj with field data
    private variable _path "";          # Path of this object in the XML
    private variable _field "";         # This field element as XML obj

    private variable _type "";          # Field type: e.g. file type
    private variable _hints;            # Hints array
    private variable _limits;           # maps axis name => {z0 z1} limits
    private variable _units "";         # system of units for this field
    private variable _viewer "";        # Hints which viewer to use
    private variable _isValid 0;        # Indicates if the field contains
                                        # valid data.
    private variable _isValidComponent; # Array of valid components found
    private variable _zmax 0;           # length of the device (1D only)

    private variable _fld2Components;   # field name => number of components
    private variable _fld2Label;        # field name => label
    private variable _fld2Units;        # field name => units

    private variable _comp2fldName;     # cname => field names.
    private variable _comp2type;        # cname => type (e.g. "vectors")
    private variable _comp2size;        # cname => # of components in element
    private variable _comp2assoc;       # cname => association (e.g. pointdata)
    private variable _comp2dims;        # cname => dimensionality
    private variable _comp2xy;          # cname => x,y vectors
    private variable _comp2vtk;         # cname => vtk file data
    private variable _comp2dx;          # cname => OpenDX data
    private variable _comp2unirect2d;   # cname => unirect2d obj
    private variable _comp2unirect3d;   # cname => unirect3d obj
    private variable _comp2style;       # cname => style settings
    private variable _comp2cntls;       # cname => x,y control points (1D only)
    private variable _comp2limits;      # cname => List of axis, min/max list
    private variable _comp2flowhints;   # cname => Rappture::FlowHints obj
    private variable _comp2mesh;        # list: mesh obj, BLT vector of values
                                        # valid for cloud,mesh,unirect2d
    private variable _values "";        # Only for unirect2d - list of values

    private common _alwaysConvertDX 0;  # If set, convert DX and store as VTK,
                                        # even if viewer is nanovis/flowvis
    private common _counter 0;          # counter for unique vector names
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
    InitHints
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
    set components {}
    # First compile a list of valid components that match the pattern
    foreach cname [$_field children -type component] {
        if { ![info exists _isValidComponent($cname)] } {
            continue
        }
        if { [string match $pattern $cname] } {
            lappend components $cname
        }
    }
    # Now handle the tests.
    switch -- $params(what) {
        -name {
            set rlist $components
        }
        -style {
            foreach cname $components {
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
# For 1D data (curve), returns a BLT vector of x values for the field
# component <name>.  Otherwise, this method is unused
# ----------------------------------------------------------------------
itcl::body Rappture::Field::mesh {{cname -overall}} {
    if {$cname == "-overall" || $cname == "component0"} {
        set cname [lindex [components -name] 0]
    }
    if {[info exists _comp2xy($cname)]} {
        return [lindex $_comp2xy($cname) 0]  ;# return xv
    }
    if {[info exists _comp2vtk($cname)]} {
        # FIXME: extract mesh from VTK file data.
        error "method \"mesh\" is not implemented for VTK file data"
    }
    if {[info exists _comp2dx($cname)]} {
        error "method \"mesh\" is not implemented for DX file data"
    }
    if {[info exists _comp2mesh($cname)]} {
        # FIXME: This only works for cloud
        set mesh [lindex $_comp2mesh($cname) 0]
        return [$mesh points]
    }
    if {[info exists _comp2unirect2d($cname)]} {
        # FIXME: unirect2d mesh is a list: xMin xMax xNum yMin yMax yNum
        return [$_comp2unirect2d($cname) mesh]
    }
    if {[info exists _comp2unirect3d($cname)]} {
        # This returns a list of x,y,z points
        return [$_comp2unirect3d($cname) mesh]
    }
    error "can't get field mesh: Unknown component \"$cname\": should be one of [join [lsort [array names _comp2dims]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: values ?<name>?
#
# For 1D data (curve), returns a BLT vector of field values (y coords)
# for the field component <name>.  Otherwise, this method is unused
# ----------------------------------------------------------------------
itcl::body Rappture::Field::values {cname} {
    if {$cname == "component0"} {
        set cname "component"
    }
    if {[info exists _comp2xy($cname)]} {
        return [lindex $_comp2xy($cname) 1]  ;# return yv
    }
    if { [info exists _comp2vtk($cname)] } {
        # FIXME: extract the values from the VTK file data
        error "method \"values\" is not implemented for VTK file data"
    }
    if {[info exists _comp2dx($cname)]} {
        error "method \"values\" is not implemented for DX file data"
    }
    if { [info exists _comp2mesh($cname)] } {
        set vector [lindex $_comp2mesh($cname) 1]
        return [$vector range 0 end]
    }
    if {[info exists _comp2unirect2d($cname)]} {
        return $_values
    }
    if {[info exists _comp2unirect3d($cname)]} {
        return [$_comp2unirect3d($cname) values]
    }
    error "can't get field values. Unknown component \"$cname\": should be one of [join [lsort [array names _comp2dims]] {, }]"
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
    if { [info exists _comp2vtk($cname)] } {
        error "blob not implemented for VTK file data"
    }
    if {[info exists _comp2xy($cname)]} {
        error "blob not implemented for XY data"
    }
    error "can't get field blob: Unknown component \"$cname\": should be one of [join [lsort [array names _comp2dims]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: valueLimits <cname>
#
# Returns an array for the requested component with a list {min max}
# representing the limits for each axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::valueLimits { cname } {
    if { [info exists _comp2limits($cname)] } {
        return $_comp2limits($cname)
    }
    return ""
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

                if {$log} {
                    blt::vector tmp zero
                    # on a log scale, use abs value and ignore zeros
                    $vname dup tmp
                    $vname dup zero
                    zero expr {tmp == 0}            ;# find the zeros
                    tmp expr {abs(tmp)}             ;# get the abs value
                    tmp expr {tmp + zero*max(tmp)}  ;# replace 0s with abs max
                    set axisMin [blt::vector expr min(tmp)]
                    set axisMax [blt::vector expr max(tmp)]
                    blt::vector destroy tmp zero
                } else {
                    set axisMin [$vname min]
                    set axisMax [$vname max]
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
            default {
                if {[info exists _comp2limits($cname)]} {
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
        foreach fname [fieldnames $cname] {
            if { ![info exists limits($fname)] } {
                puts stderr "ERROR: field \"$fname\" unknown in \"$cname\""
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
    if { $keyword != "" } {
        if {[info exists _hints($keyword)]} {
            return $_hints($keyword)
        }
        return ""
    }
    return [array get _hints]
}


# ----------------------------------------------------------------------
# USAGE: InitHints
#
# Returns a list of key/value pairs for various hints about plotting
# this field.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::InitHints {} {
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
    foreach cname [components] {
        if { ![info exists _comp2mesh($cname)] } {
            continue
        }
        set mesh [lindex $_comp2mesh($cname) 0]
        foreach axis {x y z} {
            if { ![info exists _hints(${axis}units)] } {
                set _hints(${axis}units) [$mesh units $axis]
            }
            if { ![info exists _hints(${axis}label)] } {
                set _hints(${axis}label) [$mesh label $axis]
            }
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
            set type "vtk"
        } elseif {[$_field element $cname.opendx] != ""} {
            set type "opendx"
        } elseif {[$_field element $cname.dx] != ""} {
            set type "dx"
        } elseif {[$_field element $cname.ucd] != ""} {
            set type "ucd"
        } elseif {[$_field element $cname.dicom] != ""} {
            set type "dicom"
        }
        set _comp2style($cname) ""
        if { $type == "" } {
            puts stderr "WARNING: Ignoring field component \"$_path.$cname\": no data found."
            continue
        }
        set _type $type

        GetTypeAndSize $cname
        GetAssociation $cname

        if { $_comp2size($cname) < 1 ||
             $_comp2size($cname) > 9 } {
            puts stderr "ERROR: Invalid <elemsize>: $_comp2size($cname)"
            continue
        }
        # Some types have restrictions on number of components
        if { $_comp2type($cname) == "vectors" &&
             $_comp2size($cname) != 3 } {
            puts stderr "ERROR: vectors <elemtype> must have <elemsize> of 3"
            continue
        }
        if { $_comp2type($cname) == "normals" &&
             $_comp2size($cname) != 3 } {
            puts stderr "ERROR: normals <elemtype> must have <elemsize> of 3"
            continue
        }
        if { $_comp2type($cname) == "tcoords" &&
             $_comp2size($cname) > 3 } {
            puts stderr "ERROR: tcoords <elemtype> must have <elemsize> <= 3"
            continue
        }
        if { $_comp2type($cname) == "scalars" &&
             $_comp2size($cname) > 4 } {
            puts stderr "ERROR: scalars <elemtype> must have <elemsize> <= 4"
            continue
        }
        if { $_comp2type($cname) == "colorscalars" &&
             $_comp2size($cname) > 4 } {
            puts stderr "ERROR: colorscalars <elemtype> must have <elemsize> <= 4"
            continue
        }
        if { $_comp2type($cname) == "tensors" &&
             $_comp2size($cname) != 9 } {
            puts stderr "ERROR: tensors <elemtype> must have <elemsize> of 9"
            continue
        }

        if { [$_field element $cname.flow] != "" } {
            set haveFlow 1
        } else {
            set haveFlow 0
        }
        set viewer [$_field get "about.view"]
        if { $viewer != "" } {
            set _viewer $viewer
        }
        if { $_viewer == "" } { 
            if { $_comp2size($cname) > 1 && ! $haveFlow } {
                set _viewer "glyphs"
            } elseif { $_comp2size($cname) > 1 && $haveFlow } {
                set _viewer "flowvis"
            }
        }

        if {$type == "1D"} {
            #
            # 1D data can be represented as 2 BLT vectors,
            # one for x and the other for y.
            #
            set xv ""
            set yv ""
            set _dim 1
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
                set _dim 1
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
                puts stderr "WARNING: No data for \"$_path.$cname.vtk\""
                continue;               # Ignore this component
            }
            ReadVtkDataSet $cname $contents
            set _comp2vtk($cname) $contents
            set _comp2style($cname) [$_field get $cname.style]
            incr _counter
        } elseif {$type == "dx" || $type == "opendx" } {
            #
            # Extract gzipped, base64-encoded OpenDX data
            #
            if { $_viewer == "" } {
                if {$haveFlow} {
                    set _viewer "flowvis"
                } else {
                    global env
                    if { [info exists env(VTKVOLUME)] } {
                        set _viewer "vtkvolume"
                    }
                    set _viewer "nanovis"
                }
            }
            set data [$_field get -decode no $cname.$type]
            set contents [Rappture::encoding::decode -as zb64 $data]
            if { $contents == "" } {
                puts stderr "WARNING: No data for \"$_path.$cname.$type\""
                continue;               # Ignore this component
            }
            if 0 {
                set f [open /tmp/$_path.$cname.dx "w"]
                puts -nonewline $f $contents
                close $f
            }
            # Convert to VTK
            if { [catch { Rappture::DxToVtk $contents } vtkdata] == 0 } {
                unset contents
                # Read back VTK: this will set the field limits and the mesh
                # dimensions based on the bounds (sets _dim).  We rely on this
                # conversion for limits even if we send DX data to nanovis.
                ReadVtkDataSet $cname $vtkdata
                if 0 {
                    set f [open /tmp/$_path.$cname.vtk "w"]
                    puts -nonewline $f $vtkdata
                    close $f
                }
            } else {
                unset contents
                # vtkdata variable holds error message
                puts stderr "Can't parse DX data\n$vtkdata"
                continue;               # Ignore this component
            }
            if { $_alwaysConvertDX ||
                 ($_viewer != "nanovis" && $_viewer != "flowvis") } {
                set _type "vtk"
                set _comp2vtk($cname) $vtkdata
            } else {
                set _type "dx"
                set _comp2dx($cname) $data
            }
            unset data
            unset vtkdata
            set _comp2style($cname) [$_field get $cname.style]
            if {$haveFlow} {
                set _comp2flowhints($cname) \
                    [Rappture::FlowHints ::\#auto $_field $cname $_units]
            }
            incr _counter
        } elseif { $type == "dicom"} {
            set contents [$_field get $cname.dicom]
            if { $contents == "" } {
                continue;               # Ignore this component
            }
            set vtkdata [DicomToVtk $cname $contents]
            if { $_viewer == "" } {
                set _viewer [expr {($_dim == 3) ? "vtkvolume" : "vtkimage"}]
            }
            set _comp2vtk($cname) $vtkdata
            set _comp2style($cname) [$_field get $cname.style]
            incr _counter
        } elseif { $type == "ucd"} {
            set contents [$_field get $cname.ucd]
            if { $contents == "" } {
                continue;               # Ignore this component
            }
            set vtkdata [AvsToVtk $cname $contents]
            ReadVtkDataSet $cname $vtkdata
            set _comp2vtk($cname) $vtkdata
            set _comp2style($cname) [$_field get $cname.style]
            incr _counter
        }
        set _isValidComponent($cname) 1
        #puts stderr "Field $cname type is: $_type"
    }
    if { [array size _isValidComponent] == 0 } {
        puts stderr "ERROR: All components of field \"$_path\" are invalid."
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
            puts stderr "WARNING: A field can't have components of different dimensions: [join [array get _comp2dims] ,]"
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
# Returns the data storage type of the field.
#
# FIXME: What are the valid types?
#
itcl::body Rappture::Field::type {} {
    return $_type
}

#
# numComponents --
#
# Returns the number of components in the field component.
#
itcl::body Rappture::Field::numComponents {cname} {
    set name $cname
    regsub -all { } $name {_} name
    if {[info exists _fld2Components($name)] } {
        return $_fld2Components($name)
    }
    return 1;                           # Default to scalar.
}

itcl::body Rappture::Field::VerifyVtkDataSet { contents } {
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
    $reader ReadAllNormalsOn
    $reader ReadAllTCoordsOn
    $reader ReadAllScalarsOn
    $reader ReadAllColorScalarsOn
    $reader ReadAllVectorsOn
    $reader ReadAllTensorsOn
    $reader ReadAllFieldsOn
    $reader Update
    file delete $tmpfile

    set dataset [$reader GetOutput]
    set dataAttrs [$dataset GetPointData]
    if { $dataAttrs == ""} {
        puts stderr "WARNING: No point data found in \"$_path\""
        rename $reader ""
        return 0
    }
    rename $reader ""
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
    $reader ReadAllNormalsOn
    $reader ReadAllTCoordsOn
    $reader ReadAllScalarsOn
    $reader ReadAllColorScalarsOn
    $reader ReadAllVectorsOn
    $reader ReadAllTensorsOn
    $reader ReadAllFieldsOn
    $reader Update
    file delete $tmpfile

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
    lappend limits x [list $xmin $xmax]
    lappend limits y [list $ymin $ymax]
    lappend limits z [list $zmin $zmax]
    set dataAttrs [$dataset GetPointData]
    if { $dataAttrs == ""} {
        puts stderr "WARNING: No point data found in \"$_path\""
        rename $reader ""
        return 0
    }
    set vmin 0
    set vmax 1
    set numArrays [$dataAttrs GetNumberOfArrays]
    if { $numArrays > 0 } {
        for {set i 0} {$i < [$dataAttrs GetNumberOfArrays] } {incr i} {
            set array [$dataAttrs GetArray $i]
            set fname  [$dataAttrs GetArrayName $i]
            foreach {min max} [$array GetRange -1] break
            if {$i == 0} {
                set vmin $min
                set vmax $max
            }
            lappend limits $fname [list $min $max]
            set _fld2Units($fname) ""
            set _fld2Label($fname) $fname
            # Let the VTK file override the <type> designated.
            set _fld2Components($fname) [$array GetNumberOfComponents]
            lappend _comp2fldName($cname) $fname
        }
    }

    lappend limits v [list $vmin $vmax]
    set _comp2limits($cname) $limits

    rename $reader ""
}

#
# VtkDataSetToXy --
#
#        Attempt to convert 0 or 1 dimensional VTK DataSet to XY data (curve).
#
itcl::body Rappture::Field::VtkDataSetToXy { dataset } {
    foreach {xmin xmax ymin ymax zmin zmax} [$dataset GetBounds] break
    # Only X can have non-zero range.  X can have zero range if there is
    # only one point
    if { $ymax > $ymin } {
        return 0
    }
    if { $zmax > $zmin } {
        return 0
    }
    set numPoints [$dataset GetNumberOfPoints]
    set dataAttrs [$dataset GetPointData]
    if { $dataAttrs == ""} {
        puts stderr "WARNING: No point data found"
        return 0
    }
    set array [$dataAttrs GetScalars]
    if { $array == ""} {
        puts stderr "WARNING: No scalar point data found"
        return 0
    }
    # Multi-component scalars (e.g. color scalars) are not supported
    if { [$array GetNumberOfComponents] != 1 } {
        return 0
    }
    set numTuples [$array GetNumberOfTuples]
    set xv [blt::vector create \#auto]
    for { set i 0 } { $i < $numPoints } { incr i } {
        set point [$dataset GetPoint $i]
        $xv append [lindex $point 0]
    }
    set yv [blt::vector create \#auto]
    for { set i 0 } { $i < $numTuples } { incr i } {
        $yv append [$array GetComponent $i 0]
    }
    $xv sort $yv
    set _comp2xy($cname) [list $xv $yv]
    return 1
}

#
# vtkdata --
#
#        Returns a string representing the mesh and field data for a specific
#        component in the legacy VTK file format.
#
itcl::body Rappture::Field::vtkdata {cname} {
    if {$cname == "component0"} {
        set cname "component"
    }
    # VTK file data:
    if { [info exists _comp2vtk($cname)] } {
        return $_comp2vtk($cname)
    }
    # DX: Convert DX to VTK
    if {[info exists _comp2dx($cname)]} {
        set data $_comp2dx($cname)
        set data [Rappture::encoding::decode -as zb64 $data]
        return [Rappture::DxToVtk $data]
    }
    # unirect2d (deprecated)
    # This can be removed when the nanovis server with support for loading VTK
    # vector data is released
    if {[info exists _comp2unirect2d($cname)]} {
        set label $cname
        regsub -all { } $label {_} label
        set elemSize [numComponents $cname]
        set numValues [$_comp2unirect2d($cname) numpoints]
        append out "# vtk DataFile Version 3.0\n"
        append out "[hints label]\n"
        append out "ASCII\n"
        append out [$_comp2unirect2d($cname) vtkdata]
        append out "POINT_DATA $numValues\n"
        if {$elemSize == 3} {
            append out "VECTORS $label double\n"
        } else {
            append out "SCALARS $label double $elemSize\n"
            append out "LOOKUP_TABLE default\n"
        }
        # values for VTK are x-fastest
        append out $_values
        append out "\n"
        return $out
    }
    # unirect3d (deprecated)
    if {[info exists _comp2unirect3d($cname)]} {
        set vector [$_comp2unirect3d($cname) values]
        set label $cname
        regsub -all { } $label {_} label
        set elemSize [numComponents $cname]
        set numValues [expr [$vector length] / $elemSize]
        append out "# vtk DataFile Version 3.0\n"
        append out "[hints label]\n"
        append out "ASCII\n"
        append out [$_comp2unirect3d($cname) vtkdata]
        append out "POINT_DATA $numValues\n"
        if {$elemSize == 3} {
            append out "VECTORS $label double\n"
        } else {
            append out "SCALARS $label double $elemSize\n"
            append out "LOOKUP_TABLE default\n"
        }
        # values for VTK are x-fastest
        append out [$vector range 0 end]
        append out "\n"
        return $out
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

        if { $_comp2assoc($cname) == "pointdata" } {
            set vtkassoc "POINT_DATA"
        } elseif { $_comp2assoc($cname) == "celldata" } {
            set vtkassoc "CELL_DATA"
        } elseif { $_comp2assoc($cname) == "fielddata" } {
            set vtkassoc "FIELD"
        } else {
            error "unknown association \"$_comp2assoc($cname)\""
        }
        set elemSize [numComponents $cname]
        set numValues [expr [$vector length] / $elemSize]
        if { $_comp2assoc($cname) == "fielddata" } {
            append out "$vtkassoc FieldData 1\n"
            append out "$label $elemSize $numValues double\n"
        } else {
            append out "$vtkassoc $numValues\n"
            if { $_comp2type($cname) == "colorscalars" } {
                # Must be float for ASCII, unsigned char for BINARY
                append out "COLOR_SCALARS $label $elemSize\n"
            } elseif { $_comp2type($cname) == "normals" } {
                # elemSize must equal 3
                append out "NORMALS $label double\n"
            } elseif { $_comp2type($cname) == "scalars" } {
                # elemSize can be 1, 2, 3 or 4
                append out "SCALARS $label double $elemSize\n"
                append out "LOOKUP_TABLE default\n"
            } elseif { $_comp2type($cname) == "tcoords" } {
                # elemSize must be 1, 2, or 3
                append out "TEXTURE_COORDINATES $label $elemSize double\n"
            } elseif { $_comp2type($cname) == "tensors" } {
                # elemSize must equal 9
                append out "TENSORS $label double\n"
            } elseif { $_comp2type($cname) == "vectors" } {
                # elemSize must equal 3
                append out "VECTORS $label double\n"
            } else {
                error "unknown element type \"$_comp2type($cname)\""
            }
        }
        append out [$vector range 0 end]
        append out "\n"
        if 0 {
            VerifyVtkDataSet $out
        }
        return $out
    }
    error "can't find vtkdata for $cname"
}

#
# BuildPointsOnMesh --
#
#        Parses the field XML description to build a mesh and values vector
#        representing the field.  Right now we handle the deprecated types
#        of "cloud", "unirect2d", and "unirect3d" (mostly for flows).
#
itcl::body Rappture::Field::BuildPointsOnMesh {cname} {
    #
    # More complex 2D/3D data is represented by a mesh
    # object and an associated vector for field values.
    #
    set path [$_field get $cname.mesh]
    if {[$_xmlobj element $path] == ""} {
        # Unknown mesh designated.
        puts stderr "ERROR: Unknown mesh \"$path\""
        return 0
    }
    set element [$_xmlobj element -as type $path]
    set name $cname
    regsub -all { } $name {_} name
    set _fld2Label($name) $name
    set label [hints zlabel]
    if { $label != "" } {
        set _fld2Label($name) $label
    }
    set _fld2Units($name) [hints zunits]
    set _fld2Components($name) $_comp2size($cname)
    lappend _comp2fldName($cname) $name

    # Handle bizarre cases that are due to be removed.
    if { $element == "unirect3d" } {
        # Special case: unirect3d (deprecated) + flow.
        set vectorsize [numComponents $cname]
        set _type unirect3d
        set _dim 3
        if { $_viewer == "" } {
            set _viewer flowvis
        }
        set _comp2dims($cname) "3D"
        set _comp2unirect3d($cname) \
            [Rappture::Unirect3d \#auto $_xmlobj $_field $cname $vectorsize]
        set _comp2style($cname) [$_field get $cname.style]
        set limits {}
        foreach axis { x y z } {
            lappend limits $axis [$_comp2unirect3d($cname) limits $axis]
        }
        # Get the data limits
        set vector [$_comp2unirect3d($cname) values]
        set minmax [VectorLimits $vector $vectorsize]
        lappend limits $cname $minmax
        lappend limits v      $minmax
        set _comp2limits($cname) $limits
        if {[$_field element $cname.flow] != ""} {
            set _comp2flowhints($cname) \
                [Rappture::FlowHints ::\#auto $_field $cname $_units]
        }
        incr _counter
        return 1
    }
    if { $element == "unirect2d" && [$_field element $cname.flow] != "" } {
        # Special case: unirect2d (deprecated) + flow.
        set vectorsize [numComponents $cname]
        set _type unirect2d
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
        foreach axis { x y z } {
            lappend limits $axis [$_comp2unirect2d($cname) limits $axis]
        }
        set xv [blt::vector create \#auto]
        $xv set $_values
        set minmax [VectorLimits $xv $vectorsize]
        lappend limits $cname $minmax
        lappend limits v $minmax
        blt::vector destroy $xv
        set _comp2limits($cname) $limits
        incr _counter
        return 1
    }
    switch -- $element {
        "cloud" {
            set mesh [Rappture::Cloud::fetch $_xmlobj $path]
            set _type cloud
        }
        "mesh" {
            set mesh [Rappture::Mesh::fetch $_xmlobj $path]
            set _type mesh
        }
        "unirect2d" {
            if { $_viewer == "" } {
                set _viewer "heightmap"
            }
            set mesh [Rappture::Unirect2d::fetch $_xmlobj $path]
            set _type unirect2d
        }
    }
    if { ![$mesh isvalid] } {
        puts stderr "Mesh is invalid"
        return 0
    }
    set _dim [$mesh dimensions]
    if { $_dim == 3 } {
        set dim 0
        foreach axis {x y z} {
            foreach {min max} [$mesh limits $axis] {
                if { $min < $max } {
                    incr dim
                }
            }
        }
        if { $dim != 3 } {
            set _dim $dim
        }
    }

    if {$_dim < 2} {
        puts stderr "ERROR: Can't convert 1D cloud/mesh to curve.  Please use curve output for 1D meshes."
        return 0

        # 1D data: Create vectors for graph widget.
        # The prophet tool currently outputs 1D clouds with fields
        # Band Structure Lab used to (see isosurface1 test in rappture-bat)
        #
        # Is there a natural growth path in generating output from 1D to
        # higher dimensions?  If there isn't, let's kill this in favor
        # or explicitly using a <curve> instead.  Otherwise, the features
        # (methods such as xmarkers) of the <curve> need to be added
        # to the <field>.
        #
        #set xv [blt::vector create x$_counter]
        #set yv [blt::vector create y$_counter]

        # This only works with a Cloud mesh type, since the points method
        # is not implemented for the Mesh object
        #$xv set [$mesh points]
        # TODO: Put field values in yv
        #set _comp2dims($cname) "1D"
        #set _comp2xy($cname) [list $xv $yv]
        #incr _counter
        #return 1
    }
    if {$_dim == 2} {
        # 2D data: By default surface or contour plot using heightmap widget.
        set v [blt::vector create \#auto]
        $v set [$_field get $cname.values]
        if { [$v length] == 0 } {
            return 0
        }
        if { $_viewer == "" } {
            set _viewer "contour"
        }
        set numFieldValues [$v length]
        set numComponentsPerTuple [numComponents $cname]
        if { [expr $numFieldValues % $numComponentsPerTuple] != 0 } {
            puts stderr "ERROR: Number of field values ($numFieldValues) not divisble by elemsize ($numComponentsPerTuple)"
            return 0
        }
        set numFieldTuples [expr $numFieldValues / $numComponentsPerTuple]
        if { $_comp2assoc($cname) == "pointdata" } {
            set numPoints [$mesh numpoints]
            if { $numPoints != $numFieldTuples } {
                puts stderr "ERROR: Number of points in mesh ($numPoints) and number of field tuples ($numFieldTuples) don't agree"
                return 0
            }
        } elseif { $_comp2assoc($cname) == "celldata" } {
            set numCells [$mesh numcells]
            if { $numCells != $numFieldTuples } {
                puts stderr "ERROR: Number of cells in mesh ($numCells) and number of field tuples ($numFieldTuples) don't agree"
                return 0
            }
        }
        set _comp2dims($cname) "[$mesh dimensions]D"
        set _comp2mesh($cname) [list $mesh $v]
        set _comp2style($cname) [$_field get $cname.style]
        if {[$_field element $cname.flow] != ""} {
            set _comp2flowhints($cname) \
                [Rappture::FlowHints ::\#auto $_field $cname $_units]
        }
        incr _counter
        array unset _comp2limits $cname
        foreach axis { x y z } {
            lappend _comp2limits($cname) $axis [$mesh limits $axis]
        }
        set minmax [VectorLimits $v $_comp2size($cname)]
        lappend _comp2limits($cname) $cname $minmax
        lappend _comp2limits($cname) v $minmax
        return 1
    }
    if {$_dim == 3} {
        # 3D data: By default isosurfaces plot using isosurface widget.
        if { $_viewer == "" } {
            set _viewer "isosurface"
        }
        set v [blt::vector create \#auto]
        $v set [$_field get $cname.values]
        if { [$v length] == 0 } {
            puts stderr "ERROR: No field values"
            return 0
        }
        set numFieldValues [$v length]
        set numComponentsPerTuple [numComponents $cname]
        if { [expr $numFieldValues % $numComponentsPerTuple] != 0 } {
            puts stderr "ERROR: Number of field values ($numFieldValues) not divisble by elemsize ($numComponentsPerTuple)"
            return 0
        }
        set numFieldTuples [expr $numFieldValues / $numComponentsPerTuple]
        if { $_comp2assoc($cname) == "pointdata" } {
            set numPoints [$mesh numpoints]
            if { $numPoints != $numFieldTuples } {
                puts stderr "ERROR: Number of points in mesh ($numPoints) and number of field tuples ($numFieldTuples) don't agree"
                return 0
            }
        } elseif { $_comp2assoc($cname) == "celldata" } {
            set numCells [$mesh numcells]
            if { $numCells != $numFieldTuples } {
                puts stderr "ERROR: Number of cells in mesh ($numCells) and number of field tuples ($numFieldTuples) don't agree"
                return 0
            }
        }
        set _comp2dims($cname) "[$mesh dimensions]D"
        set _comp2mesh($cname) [list $mesh $v]
        set _comp2style($cname) [$_field get $cname.style]
        if {[$_field element $cname.flow] != ""} {
            set _comp2flowhints($cname) \
                [Rappture::FlowHints ::\#auto $_field $cname $_units]
        }
        incr _counter
        foreach axis { x y z } {
            lappend _comp2limits($cname) $axis [$mesh limits $axis]
        }
        set minmax [VectorLimits $v $_comp2size($cname)]
        lappend _comp2limits($cname) $cname $minmax
        lappend _comp2limits($cname) v $minmax
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

    set tmpfile $cname[pid].vtk
    set writer $this-datasetwriter
    vtkDataSetWriter $writer
    $writer SetInputConnection [$reader GetOutputPort]
    $writer SetFileName $tmpfile
    $writer SetFileTypeToBinary
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

itcl::body Rappture::Field::DicomToVtk { cname path } {
    package require vtk

    if { ![file exists $path] } {
        puts stderr "path \"$path\" doesn't exist."
        return 0
    }

    if { [file isdir $path] } {
        set files [glob -nocomplain $path/*.dcm]
        if { [llength $files] == 0 } {
            set files [glob -nocomplain $path/*]
            if { [llength $files] == 0 } {
                puts stderr "no dicom files found in \"$path\""
                return 0
            }
        }

        #array set data [Rappture::DicomToVtk files $files]
        array set data [Rappture::DicomToVtk dir $path]
    } else {
        array set data [Rappture::DicomToVtk files [list $path]]
    }

    if 0 {
        foreach key [array names data] {
            if {$key == "vtkdata"} {
                if 0 {
                    set f [open /tmp/$cname.vtk "w"]
                    fconfigure $f -translation binary -encoding binary
                    puts -nonewline $f $data(vtkdata)
                    close $f
                }
            } else {
                puts stderr "$key = \"$data($key)\""
            }
        }
    }

    ReadVtkDataSet $cname $data(vtkdata)
    return $data(vtkdata)
}

itcl::body Rappture::Field::GetTypeAndSize { cname } {
    array set type2components {
        "colorscalars"         4
        "normals"              3
        "scalars"              1
        "tcoords"              2
        "tensors"              9
        "vectors"              3
    }
    set type [$_field get $cname.elemtype]
    # <extents> is a deprecated synonym for <elemsize>
    set extents  [$_field get $cname.extents]
    if { $type == "" } {
        if { $extents != "" && $extents > 1 } {
            set type "vectors"
        } else {
            set type "scalars"
        }
    }
    if { ![info exists type2components($type)] } {
        error "unknown <elemtype> \"$type\" in field"
    }
    set size [$_field get $cname.elemsize]
    if { $size == "" } {
        if { $extents != "" } {
            set size $extents
        } else {
            set size $type2components($type)
        }
    }
    set _comp2type($cname) $type
    set _comp2size($cname) $size
}

itcl::body Rappture::Field::GetAssociation { cname } {
    set assoc [$_field get $cname.association]
    if { $assoc == "" } {
        set _comp2assoc($cname) "pointdata"
        return
    }
    switch -- $assoc {
        "pointdata" - "celldata" - "fielddata" {
            set _comp2assoc($cname) $assoc
            return
        }
        default {
            error "unknown field association \"$assoc\""
        }
    }
}

#
# Compute the per-component limits or limits of vector magnitudes
#
itcl::body Rappture::Field::VectorLimits {vector vectorsize {comp -1}} {
    if {$vectorsize == 1} {
        set minmax [$vector limits]
    } else {
        set len [$vector length]
        if {[expr $len % $vectorsize] != 0} {
            error "Invalid vectorsize: $vectorsize"
        }
        if {$comp > $vectorsize-1} {
            error "Invalid vector component: $comp"
        }
        set numTuples [expr ($len/$vectorsize)]
        for {set i 0} {$i < $numTuples} {incr i} {
            if {$comp >= 0} {
                set idx [expr ($i * $vectorsize + $comp)]
                set val [$vector index $idx]
            } else {
                set idx [expr ($i * $vectorsize)]
                set mag 0
                for {set j 0} {$j < $vectorsize} {incr j} {
                    set val [$vector index $idx]
                    set mag [expr ($mag + $val * $val)]
                    incr idx
                }
                set val [expr (sqrt($mag))]
            }
            if (![info exists minmax]) {
                set minmax [list $val $val]
            } else {
                if {$val < [lindex $minmax 0]} {
                    lset minmax 0 $val
                }
                if {$val > [lindex $minmax 1]} {
                    lset minmax 1 $val
                }
            }
        }
    }
    return $minmax
}
