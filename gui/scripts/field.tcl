# ----------------------------------------------------------------------
#  COMPONENT: field - extracts data from an XML description of a field
#
#  This object represents one field in an XML description of a device.
#  It simplifies the process of extracting data vectors that represent
#  the field.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Field {
    constructor {devobj libobj path} { # defined below }
    destructor { # defined below }

    public method components {{pattern *}}
    public method vectors {{what -overall}}
    public method controls {option args}
    public method hints {{key ""}}

    protected method _build {}

    private variable _device ""  ;# ref to lib obj with device data
    private variable _libobj ""  ;# ref to lib obj with field data

    private variable _units ""   ;# system of units for this field
    private variable _limits     ;# maps slab name => {z0 z1} limits
    private variable _zmax 0     ;# length of the device

    private variable _field ""   ;# lib obj representing this field
    private variable _comp2vecs  ;# maps component name => x,y vectors
    private variable _comp2cntls ;# maps component name => x,y control points

    private common _counter 0    ;# counter for unique vector names
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Field::constructor {devobj libobj path} {
    if {![Rappture::library isvalid $devobj]} {
        error "bad value \"$devobj\": should be LibraryObj"
    }
    if {![Rappture::library isvalid $libobj]} {
        error "bad value \"$libobj\": should be LibraryObj"
    }
    set _device $devobj
    set _libobj $libobj
    set _field [$libobj element -flavor object $path]
    set _units [$_field get units]

    # determine the overall size of the device
    set z0 [set z1 0]
    foreach elem [$_device children components] {
        switch -glob -- $elem {
            slab* - molecule* {
                if {![regexp {[0-9]$} $elem]} {
                    set elem "${elem}0"
                }
                set tval [$_device get components.$elem.thickness]
                set tval [Rappture::Units::convert $tval \
                    -context um -to um -units off]
                set z1 [expr {$z0+$tval}]
                set _limits($elem) [list $z0 $z1]

                set z0 $z1
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
    # don't destroy the _device! we don't own it!

    foreach name [array names _comp2vecs] {
        eval blt::vector destroy $_comp2vecs($name)
    }
}

# ----------------------------------------------------------------------
# USAGE: components ?<pattern>?
#
# Returns a list of names for the various components of this field.
# If the optional glob-style <pattern> is specified, then it returns
# only the component names matching the pattern.
# ----------------------------------------------------------------------
itcl::body Rappture::Field::components {{pattern *}} {
    set rlist ""
    foreach name [array names _comp2vecs] {
        if {[string match $pattern $name]} {
            lappend rlist $name
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: vectors ?<name>?
#
# Returns a list {xvec yvec} for the specified field component <name>.
# If the name is not specified, then it returns the vectors for the
# overall field (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Field::vectors {{what -overall}} {
    if {$what == "component0"} {
        set what "component"
    }
    if {[info exists _comp2vecs($what)]} {
        return $_comp2vecs($what)
    }
    error "bad option \"$what\": should be [join [lsort [array names _comp2vecs]] {, }]"
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
            error "bad option \"$what\": should be [join [lsort [array names _comp2cntls]] {, }]"
        }
        put {
            set path [lindex $args 0]
            set value [lindex $args 1]
            $_field put $path $value
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
    foreach key {label scale color units restrict} {
        set str [$_field get $key]
        if {"" != $str} {
            set hints($key) $str
        }
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
    foreach name [array names _comp2vecs] {
        eval blt::vector destroy $_comp2vecs($name)
    }
    catch {unset _comp2vecs}

    #
    # Scan through the components of the field and create
    # vectors for each part.
    #
    foreach cname [$_field children -type component] {
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

            if {$_units != ""} {
                set val [Rappture::Units::convert $val \
                    -context $_units -to $_units -units off]
            }
            set yv [blt::vector create y$_counter]
            $yv append $val $val

            set zm [expr {0.5*($z0+$z1)}]
            set _comp2cntls($cname) \
                [list $cname.constant $zm $val "$val$_units"]
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
            set _comp2vecs($cname) [list $xv $yv]
            incr _counter
        }
    }
}
