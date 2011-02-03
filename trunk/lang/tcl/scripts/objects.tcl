# ----------------------------------------------------------------------
#  COMPONENT: objects
#
#  This file contains routines to parse object data from files in
#  the "objects" directory.  Each file defines a Rappture object and
#  all information needed to specify the object in a tool.xml file.
#
#    object NAME ?-extends BASE? {
#        attr NAME -title XX -type TYPE -path PATH
#        check attr { code... }
#        palettes NAME NAME ...
#        help URL
#        terminal yes|no
#        ...
#    }
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2011  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

namespace eval Rappture { # forward declaration }
namespace eval Rappture::objects {
    # location of system object info
    variable installdir [file dirname [file normalize [info script]]]

    #
    # Set up a safe interpreter for loading object defn files...
    #
    variable objParser [interp create -safe]
    foreach cmd [$objParser eval {info commands}] {
	$objParser hide $cmd
    }
    $objParser alias object Rappture::objects::parse_object
    $objParser alias unknown Rappture::objects::parse_obj_unknown
    proc ::Rappture::objects::parse_obj_unknown {args} {
        error "bad option \"[lindex $args 0]\": should be object"
    }

    #
    # Set up a safe interpreter for loading object attributes...
    #
    variable attrParser [interp create -safe]
    foreach cmd [$attrParser eval {info commands}] {
	$attrParser hide $cmd
    }
    $attrParser alias attr Rappture::objects::parse_attr
    $attrParser alias check Rappture::objects::parse_check
    $attrParser alias help Rappture::objects::parse_help
    $attrParser alias palettes Rappture::objects::parse_palettes
    $attrParser alias terminal Rappture::objects::parse_terminal
    $attrParser alias unknown Rappture::objects::parse_attr_unknown
    proc ::Rappture::objects::parse_attr_unknown {args} {
        error "bad option \"[lindex $args 0]\": should be attr, check, help, palettes, terminal"
    }

    # this variable will hold ObjDef object as it is being built
    variable currObjDef ""

    # this variable will hold the name of the object file being parsed
    variable currFile ""
}

# ----------------------------------------------------------------------
# USAGE: Rappture::objects::init
#
# Called at the beginning of a Rappture program to initialize the
# object system.  Loads all object definitions in the "objects"
# directory found at the system location.  Object types can be
# queried by calling Rappture::objects::get.
# ----------------------------------------------------------------------
proc Rappture::objects::init {} {
    variable installdir

    # load supporting type definitions
    foreach fname [glob [file join $installdir types *.tcl]] {
        source $fname
    }

    # load supporting validation procs
    foreach fname [glob [file join $installdir validations *.tcl]] {
        source $fname
    }

    # load the base class
    Rappture::objects::load [file join $installdir objects base.rp]

    # load any other classes found
    foreach dir [glob -nocomplain -types d [file join $installdir objects *]] {
        Rappture::objects::load [file join $dir *.rp]
    }
}

# ----------------------------------------------------------------------
# USAGE: Rappture::objects::load ?<filePattern> <filePattern> ...?
#
# Clients call this to load object definitions from all files that
# match the given set of file patterns.  These may be specific
# file names or patterns of the form "dir/*.rp".  Each object is
# loaded and Rappture::ObjDef objects are created as a side effect
# to represent them.  Object types can be queried by calling
# Rappture::objects::get.
# ----------------------------------------------------------------------
proc Rappture::objects::load {args} {
    variable objParser
    variable objDefs
    variable currFile

    # scan through all matching files and load their definitions
    foreach pattern $args {
        foreach fname [glob -nocomplain $pattern] {
            set currFile $fname
            set fid [open $fname r]
            set info [read $fid]
            close $fid

            if {[catch {$objParser eval $info} err] != 0} {
                error $err "\n    (while loading object definition from file \"$fname\")"
            }
        }
    }

    # look at all inheritance relationships and make sure they're satisfied
    foreach name [array names objDefs] {
        set ilist [$objDefs($name) cget -inherit]
        set newilist ""
        foreach obj $ilist {
            if {[string index $obj 0] == "@"} {
                set tname [string range $obj 1 end]
                if {[info exists objDefs($tname)]} {
                    lappend newilist $objDefs($tname)
                } else {
                    set errs($tname) 1
                }
            }
            $objDefs($name) configure -inherit $newilist
        }
    }

    if {[array size errs] > 0} {
        error "missing definition for base class: [join [lsort [array names errs]] {, }]"
    }
}

# ----------------------------------------------------------------------
# USAGE: Rappture::objects::get ?<name>? ?-what?
#
# Returns information about the known Rappture object types.
# With no args, it returns a list of object class names.  With a
# specified <name>, it returns all information for that object in
# a key/value format:
#
#   -image xxx -palettes {xx xx xx} -attributes {{...} {...} ...}
#   -help URL -terminal bool
#
# Otherwise, the -what indicates which specific value should be
# returned.
# ----------------------------------------------------------------------
proc Rappture::objects::get {{name ""} {what ""}} {
    variable objDefs

    if {"" == $name} {
        return [array names objDefs]
    }

    set name [string tolower $name]  ;# doesn't matter: Tool or tool
    set info(-image) [$objDefs($name) cget -image]
    set info(-help) [$objDefs($name) cget -help]
    set info(-palettes) [$objDefs($name) cget -palettes]
    set info(-terminal) [$objDefs($name) cget -terminal]

    set rlist ""
    set olist $objDefs($name)
    while {[llength $olist] > 0} {
        set obj [lindex $olist 0]
        if {![info exists gotparents($obj)]} {
            set pos 0
            foreach baseobj [$obj cget -inherit] {
                set olist [linsert $olist $pos $baseobj]
                incr pos
            }
            set gotparents($obj) 1
        }

        set obj [lindex $olist 0]
        set olist [lrange $olist 1 end]

        foreach aname [$obj get] {
            lappend rlist [$obj get $aname]
        }
    }
    set info(-attributes) $rlist

    if {"" == $what} {
        return [array get info]
    } elseif {[info exists info($what)]} {
        return $info($what)
    }
    error "bad option \"$what\": should be [join [lsort [array names info]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: Rappture::objects::palettes
#
# Returns a list of unique palette names from all known types.
# ----------------------------------------------------------------------
proc Rappture::objects::palettes {} {
    variable objDefs

    foreach name [array names objDefs] {
        foreach pname [$objDefs($name) cget -palettes] {
            set unique($pname) 1
        }
    }
    return [lsort -dictionary [array names unique]]
}

# ----------------------------------------------------------------------
# USAGE: Rappture::objects::check <type> {<key> <val>...} <debugInfo>
#
# Checks the definition for an object of the given <type> to see if
# there are any errors in the values.  The attribute values are
# specified as a key/value list.  Returns a list of the form:
#   error "something went wrong"
#   warning "might check this"
# ----------------------------------------------------------------------
proc Rappture::objects::check {type attrinfo debug} {
    variable objDefs

    set type [string tolower $type]  ;# doesn't matter: Tool or tool

    if {[info exists objDefs($type)]} {
        return [$objDefs($type) check $attrinfo $debug]
    }
    return ""
}

# ----------------------------------------------------------------------
# PARSER:  Rappture::objects::parse_object
#
# Used internally to parse the definition of a Rappture object type:
#
#   object <name> ?-extends <type>? {
#     attr <name> <args>...
#     attr <name> <args>...
#     ...
#   }
#
# Builds an object in currObjDef and then registers the completed
# object in the objDefs array.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_object {args} {
    variable currObjDef
    variable currFile
    variable objDefs
    variable attrParser

    set name [lindex $args 0]
    set args [lrange $args 1 end]

    set ilist ""
    while {1} {
        set first [lindex $args 0]
        if {[string index $first 0] != "-"} {
            break
        }
        if {"-extends" == $first} {
            set base [lindex $args 1]
            set args [lrange $args 2 end]
            lappend ilist @$base
        } else {
            error "bad option \"$first\": should be -extends"
        }
    }

    if {[llength $args] != 1} {
        error "wrong # args: should be \"object name ?-extends base? {...definition...}\""
    }
    set body [lindex $args end]

    # create an object definition and add attributes to it
    set currObjDef [Rappture::objects::ObjDef ::#auto -inherit $ilist]
    set cmds {
        # parse attribute definitions
        $attrParser eval $body

        # look for an image for this object
        set rootf [file rootname $currFile]
        foreach ext {png jpg gif} {
            if {[file readable $rootf.$ext]
                  && [catch {package present Tk}] == 0} {
                set imh [image create photo -file $rootf.$ext]
                $currObjDef configure -image $imh
                break
            }
        }
    }

    if {[catch $cmds err] != 0} {
        itcl::delete object $currObjDef
        set currObjDef ""
        error $err "\n    (while loading object definition for \"$name\")"
    }

    set objDefs($name) $currObjDef
    set currObjDef ""
}

# ----------------------------------------------------------------------
# PARSER:  Rappture::objects::parse_attr
#
# Used internally to parse the definition of an attribute within a
# Rappture object definition:
#
#   attr <name> -title <string> -type <string> -path <string>
#
# Adds an attribute definition to the object in currObjDef.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_attr {args} {
    variable currObjDef

    set name [lindex $args 0]
    eval $currObjDef add attr $name [lrange $args 1 end]
}

# ----------------------------------------------------------------------
# PARSER:  Rappture::objects::parse_check
#
# Used internally to register a bit of code that's used to check the
# integrity of a value.
#
#   check attr { code... }
#
# The code assumes that attribute values are stored in an attr(...)
# array.  It checks the values and returns errors in the following
# format:
#   error "something went wrong" {-node 2 -counter 7 -attr label}
#   warning "watch out for this" {-node 8 -counter 1 -attr description}
# ----------------------------------------------------------------------
proc Rappture::objects::parse_check {attr code} {
    variable currObjDef
    $currObjDef add check $attr $code
}

# ----------------------------------------------------------------------
# PARSER:  Rappture::objects::parse_help
#
# Used internally to parse the definition of the help page URL for a
# Rappture object definition:
#
#   help <url>
#
# Keeps the <url> around so it can be displayed later in a "Help"
# button on the attribute editor.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_help {url} {
    variable currObjDef
    if {![regexp {^https?://} $url]} {
        error "bad value \"$url\": should be a URL for the help page"
    }
    $currObjDef configure -help $url
}

# ----------------------------------------------------------------------
# PARSER:  Rappture::objects::parse_palettes
#
# Used internally to parse the definition of the palettes for a
# Rappture object definition:
#
#   palettes <name> <name> ...
#
# Adds the list of palettes to the object definition.  This determines
# what palettes of controls will contain this object.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_palettes {args} {
    variable currObjDef
    $currObjDef configure -palettes $args
}

# ----------------------------------------------------------------------
# PARSER:  Rappture::objects::parse_terminal
#
# Used internally to parse the "terminal" setting within a Rappture
# object definition:
#
#   terminal yes|no
#
# Sets the Boolean value, which determines whether or not this object
# can have other objects embedded within it.  If it is "terminal",
# an object is a leaf node.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_terminal {val} {
    variable currObjDef
    if {![string is boolean -strict $val]} {
        error "bad value \"$val\": should be boolean"
    }
    $currObjDef configure -terminal $val
}

# ----------------------------------------------------------------------
#  CLASS: ObjDef
# ----------------------------------------------------------------------
itcl::class Rappture::objects::ObjDef {
    public variable inherit ""
    public variable image ""
    public variable help ""
    public variable terminal "yes"
    public variable palettes ""

    constructor {args} {
        set _checks(num) 0
        eval configure $args
    }

    public method add {what name args} {
        switch -- $what {
            attr {
                if {[info exists _attr2def($name)]} {
                    error "attribute \"$name\" already defined"
                }
                set obj [Rappture::objects::ObjAttr #auto]
                if {[catch {eval $obj configure $args} err] != 0} {
                    itcl::delete object $obj
                    error $err
                }
                lappend _attrs $name
                set _attr2def($name) $obj
            }
            check {
                set n [incr _checks(num)]
                set _checks($n-attr) $name
                set _checks($n-code) [lindex $args 0]
            }
        }
    }

    public method get {{name ""}} {
        if {"" == $name} {
            return $_attrs
        } elseif {[info exists _attr2def($name)]} {
            set rlist $name
            foreach opt [$_attr2def($name) configure] {
                lappend rlist [lindex $opt 0] [lindex $opt 2]
            }
            return $rlist
        }
    }

    # call this to check the integrity of all values
    public method check {data debug} {
        set rlist ""
        array set attr $data
        for {set n 1} {$n <= $_checks(num)} {incr n} {
            set status [catch $_checks($n-code) result]
            if {$status != 0 && $status != 2} {
                puts stderr "ERROR DURING VALUE CHECK:\n$result"
            } elseif {[llength $result] > 0} {
                set class [lindex $result 0]
                set mesg [lindex $result 1]
                set dinfo $debug
                lappend dinfo -attribute $_checks($n-attr)
                lappend rlist [list $class $mesg $dinfo]
            }
        }
        return $rlist
    }

    private variable _attrs ""  ;# list of attr names in order
    private variable _attr2def  ;# maps attr name => ObjAttr object
    private variable _checks    ;# bits of code used for checks
}

# ----------------------------------------------------------------------
#  CLASS: ObjAttr
# ----------------------------------------------------------------------
itcl::class Rappture::objects::ObjAttr {
    public variable title ""
    public variable type ""
    public variable path ""
    public variable expand "no"

    constructor {args} {
        eval configure $args
    }
}
