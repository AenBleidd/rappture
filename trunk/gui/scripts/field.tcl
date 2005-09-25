# ----------------------------------------------------------------------
#  COMPONENT: field - extracts data from an XML description of a field
#
#  This object represents one field in an XML description of a device.
#  It simplifies the process of extracting data vectors that represent
#  the field.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Field {
    constructor {xmlobj path} { # defined below }
    destructor { # defined below }

    public method components {args}
    public method mesh {{what -overall}}
    public method values {{what -overall}}
    public method limits {axis}
    public method controls {option args}
    public method hints {{key ""}}

    protected method _build {}
    protected method _getValue {expr}

    private variable _xmlobj ""  ;# ref to XML obj with device data

    private variable _units ""   ;# system of units for this field
    private variable _limits     ;# maps box name => {z0 z1} limits
    private variable _zmax 0     ;# length of the device

    private variable _field ""   ;# lib obj representing this field
    private variable _comp2dims  ;# maps component name => dimensionality
    private variable _comp2xy    ;# maps component name => x,y vectors
    private variable _comp2vtk   ;# maps component name => vtkFloatArray
    private variable _comp2cntls ;# maps component name => x,y control points

    private common _counter 0    ;# counter for unique vector names
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Field::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
    set _field [$xmlobj element -as object $path]
    set _units [$_field get units]

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
                    -context um -to um -units off]

                set z1 [$_xmlobj get components.$elem.corner1]
                set z1 [Rappture::Units::convert $z1 \
                    -context um -to um -units off]

                set _limits($elem) [list $z0 $z1]
            }
        }
    }
    set _zmax $z1

    # build up vectors for various components of the field
    _build
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
    foreach name [array names _comp2vtk] {
        set mobj [lindex $_comp2vtk($name) 0]
        set class [$mobj info class]
        ${class}::release $mobj

        set fobj [lindex $_comp2vtk($name) 1]
        rename $fobj ""
    }
}

# ----------------------------------------------------------------------
# USAGE: components ?-name|-dimensions? ?<pattern>?
#
# Returns a list of names or types for the various components of
# this field.  If the optional glob-style <pattern> is specified,
# then it returns only the components with names matching the pattern.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::components {args} {
    Rappture::getopts args params {
        flag what -name default
        flag what -dimensions
    }

    set pattern *
    if {[llength $args] > 0} {
        set pattern [lindex $args 0]
        set args [lrange $args 1 end]
    }
    if {[llength $args] > 0} {
        error "wrong # args: should be \"components ?switches? ?pattern?\""
    }

    set rlist ""
    foreach name [array names _comp2dims $pattern] {
        switch -- $params(what) {
            -name { lappend rlist $name }
            -dimensions { lappend rlist $_comp2dims($name) }
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
itcl::body Rappture::Field::mesh {{what -overall}} {
    if {$what == "component0"} {
        set what "component"
    }
    if {[info exists _comp2xy($what)]} {
        return [lindex $_comp2xy($what) 0]  ;# return xv
    }
    if {[info exists _comp2vtk($what)]} {
        set mobj [lindex $_comp2vtk($what) 0]
        return [$mobj mesh]
    }
    error "bad option \"$what\": should be [join [lsort [array names _comp2dims]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: values ?<name>?
#
# Returns a list {xvec yvec} for the specified field component <name>.
# If the name is not specified, then it returns the vectors for the
# overall field (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Field::values {{what -overall}} {
    if {$what == "component0"} {
        set what "component"
    }
    if {[info exists _comp2xy($what)]} {
        return [lindex $_comp2xy($what) 1]  ;# return yv
    }
    if {[info exists _comp2vtk($what)]} {
        return [lindex $_comp2vtk($what) 1]  ;# return vtkFloatArray
    }
    error "bad option \"$what\": should be [join [lsort [array names _comp2dims]] {, }]"
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

    blt::vector create tmp zero
    foreach comp [array names _comp2dims] {
        switch -- $_comp2dims($comp) {
            1D {
                switch -- $which {
                    x - xlin { set pos 0; set log 0; set axis xaxis }
                    xlog { set pos 0; set log 1; set axis xaxis }
                    y - ylin - v - vlin { set pos 1; set log 0; set axis yaxis }
                    ylog - vlog { set pos 1; set log 1; set axis yaxis }
                    default {
                        error "bad option \"$which\": should be x, xlin, xlog, y, ylin, ylog, v, vlin, vlog"
                    }
                }

                set vname [lindex $_comp2xy($comp) $pos]
                $vname variable vec

                if {$log} {
                    # on a log scale, use abs value and ignore 0's
                    $vname dup tmp
                    $vname dup zero
                    zero expr {tmp == 0}            ;# find the 0's
                    tmp expr {abs(tmp)}             ;# get the abs value
                    tmp expr {tmp + zero*max(tmp)}  ;# replace 0's with abs max
                    set vmin [blt::vector expr min(tmp)]
                    set vmax [blt::vector expr max(tmp)]
                } else {
                    set vmin $vec(min)
                    set vmax $vec(max)
                }

                if {"" == $min} {
                    set min $vmin
                } elseif {$vmin < $min} {
                    set min $vmin
                }
                if {"" == $max} {
                    set max $vmax
                } elseif {$vmax > $max} {
                    set max $vmax
                }
            }
            2D - 3D {
                foreach {xv yv} $_comp2vtk($comp) break
                switch -- $which {
                    x - xlin - xlog {
                        foreach {vmin vmax} [$xv limits x] break
                        set axis xaxis
                    }
                    y - ylin - ylog {
                        foreach {vmin vmax} [$xv limits y] break
                        set axis yaxis
                    }
                    z - zlin - zlog {
                        foreach {vmin vmax} [$xv limits z] break
                        set axis zaxis
                    }
                    v - vlin - vlog {
                        foreach {vmin vmax} [$yv GetRange] break
                        set axis vaxis
                    }
                    default {
                        error "bad option \"$which\": should be x, xlin, xlog, y, ylin, ylog, v, vlin, vlog"
                    }
                }
            }
        }
        if {"" == $min} {
            set min $vmin
        } elseif {$vmin < $min} {
            set min $vmin
        }
        if {"" == $max} {
            set max $vmax
        } elseif {$vmax > $max} {
            set max $vmax
        }
    }
    blt::vector destroy tmp zero

    set val [$_field get $axis.min]
    if {"" != $val && "" != $min} {
        if {$val > $min} {
            # tool specified this min -- don't go any lower
            set min $val
        }
    }

    set val [$_field get $axis.max]
    if {"" != $val && "" != $max} {
        if {$val < $max} {
            # tool specified this max -- don't go any higher
            set max $val
        }
    }

    return [list $min $max]
}

# ----------------------------------------------------------------------
# USAGE: controls get ?<name>?
# USAGE: controls put <path> <value>
#
# Returns a list {path1 x1 y1 val1  path2 x2 y2 val2 ...} representing
# control points for the specified field component <name>.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::controls {option args} {
    switch -- $option {
        get {
            set what [lindex $args 0]
            if {[info exists _comp2cntls($what)]} {
                return $_comp2cntls($what)
            }
            return ""
        }
        put {
            set path [lindex $args 0]
            set value [lindex $args 1]
            $_xmlobj put $path.current $value
            _build
        }
        default {
            error "bad option \"$option\": should be get or put"
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
    foreach {key path} {
        group   about.group
        label   about.label
        color   about.color
        style   about.style
        scale   about.scale
        units   units
    } {
        set str [$_field get $path]
        if {"" != $str} {
            set hints($key) $str
        }
    }

    # to be compatible with curve objects
    set hints(xlabel) "Position"

    if {[info exists hints(group)] && [info exists hints(label)]} {
        # pop-up help for each curve
        set hints(tooltip) $hints(label)
    }

    if {$keyword != ""} {
        if {[info exists hints($keyword)]} {
            return $hints($keyword)
        }
        return ""
    }
    return [array get hints]
}

# ----------------------------------------------------------------------
# USAGE: _build
#
# Used internally to build up the vector representation for the
# field when the object is first constructed, or whenever the field
# data changes.  Discards any existing vectors and builds everything
# from scratch.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::_build {} {
    # discard any existing data
    foreach name [array names _comp2xy] {
        eval blt::vector destroy $_comp2xy($name)
    }
    foreach name [array names _comp2vtk] {
        set mobj [lindex $_comp2vtk($name) 0]
        set class [$mobj info class]
        ${class}::release $mobj

        set fobj [lindex $_comp2vtk($name) 1]
        rename $fobj ""
    }
    catch {unset _comp2xy}
    catch {unset _comp2vtk}
    catch {unset _comp2dims}

    #
    # Scan through the components of the field and create
    # vectors for each part.
    #
    foreach cname [$_field children -type component] {
        set type ""
        if {( [$_field element $cname.constant] != ""
                && [$_field element $cname.domain] != "" )
              || [$_field element $cname.xy] != ""} {
            set type "1D"
        } elseif {[$_field element $cname.mesh] != ""
                    && [$_field element $cname.values] != ""} {
            set type "points-on-mesh"
        }

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

                    foreach line [split $xydata \n] {
                        if {[scan $line {%g %g} xval yval] == 2} {
                            $xv append $xval
                            $yv append $yval
                        }
                    }
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
            #
            # More complex 2D/3D data is represented by a mesh
            # object and an associated vtkFloatArray for field
            # values.
            #
            set path [$_field get $cname.mesh]
            if {[$_xmlobj element $path] != ""} {
                switch -- [$_xmlobj element -as type $path] {
                    cloud {
                        set mobj [Rappture::Cloud::fetch $_xmlobj $path]
                    }
                    mesh {
                        set mobj [Rappture::Mesh::fetch $_xmlobj $path]
                    }
                }

                if {[$mobj dimensions] > 1} {
                    #
                    # 2D/3D data
                    # Store cloud/field as components
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

                    set _comp2dims($cname) "[$mobj dimensions]D"
                    set _comp2vtk($cname) [list $mobj $farray]
                    incr _counter
                } else {
                    #
                    # OOPS!  This is 1D data
                    # Forget the cloud/field -- store BLT vectors
                    #
                    set xv [blt::vector create x$_counter]
                    set yv [blt::vector create y$_counter]

                    set vtkpts [$mobj points]
                    set max [$vtkpts GetNumberOfPoints]
                    for {set i 0} {$i < $max} {incr i} {
                        set xval [lindex [$vtkpts GetPoint $i] 0]
                        $xv append $xval
                    }
                    set class [$mobj info class]
                    ${class}::release $mobj

                    set values [$_field get $cname.values]
                    foreach yval $values {
                        if {"" != $_units} {
                            set yval [Rappture::Units::convert $yval \
                                -context $_units -to $_units -units off]
                        }
                        $yv append $yval
                    }

                    # sort x-coords in increasing order
                    $xv sort $yv

                    set _comp2dims($cname) "1D"
                    set _comp2xy($cname) [list $xv $yv]
                    incr _counter
                }
            } else {
                puts "WARNING: can't find mesh $path for field component"
            }
        }
    }
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
