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
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
    $attrParser alias clear Rappture::objects::parse_clear
    $attrParser alias compare Rappture::objects::parse_compare
    $attrParser alias export Rappture::objects::parse_export
    $attrParser alias help Rappture::objects::parse_help
    $attrParser alias import Rappture::objects::parse_import
    $attrParser alias method Rappture::objects::parse_method
    $attrParser alias palettes Rappture::objects::parse_palettes
    $attrParser alias storage Rappture::objects::parse_storage
    $attrParser alias terminal Rappture::objects::parse_terminal
    $attrParser alias unknown Rappture::objects::parse_attr_unknown
    proc ::Rappture::objects::parse_attr_unknown {args} {
        error "bad option \"[lindex $args 0]\": should be attr, check, clear, compare, export, help, import, method, palettes, storage, terminal"
    }

    # this variable will hold ObjDef object as it is being built
    variable currObjDef ""

    # this variable will hold storage/import/export for object defn
    variable currObjValDef

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
        uplevel #0 source $fname
    }

    # load supporting validation procs
    foreach fname [glob [file join $installdir validations *.tcl]] {
        uplevel #0 source $fname
    }

    # load the base class
    Rappture::objects::load [file join $installdir objects base.rp]

    # load any other classes found
    foreach dir [glob -nocomplain -types d [file join $installdir objects *]] {
        Rappture::objects::load [file join $dir *.rp]
    }

    # if anyone tries to load again, do nothing
    proc ::Rappture::objects::init {} { # already loaded }
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
                error $err "$err\n    (while loading object definition from file \"$fname\")"
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
            } else {
                lappend newilist $obj
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
    if {![info exists objDefs($name)]} {
        error "bad object type \"$name\": should be one of [join [lsort [array names objDefs]] {, }]"
    }

    set info(-image) [$objDefs($name) cget -image]
    set info(-help) [$objDefs($name) cget -help]
    set info(-palettes) [$objDefs($name) cget -palettes]
    set info(-terminal) [$objDefs($name) cget -terminal]

    set rlist ""
    foreach aname [$objDefs($name) getAttr] {
        lappend rlist [$objDefs($name) getAttr $aname]
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
# USAGE: Rappture::objects::import <xmlobj> <path>
#
# Tries to extract a value from the given <xmlobj> at the <path>.
# ----------------------------------------------------------------------
proc Rappture::objects::import {xmlobj path} {
    set type [$xmlobj element -as type $path]
    set ovclass "::Rappture::objects::[string totitle $type]Value"

    # does this object type have a value class?
    if {[catch {$ovclass ::#auto} obj]} {
        return ""
    }

    # try to load the object via its xml scheme
    if {[catch {$obj import xml $xmlobj $path} result] == 0} {
        return $obj
    }

    # can't seem to load anything -- return null
    itcl::delete object $obj
    return ""
}

# ----------------------------------------------------------------------
# USAGE: Rappture::objects::viewer <objVal>|<objDef>|<type> \
    ?-for input|output? ?-parent win?
#
# Used to find/create a viewer for the given object.  The object can
# be specified by an ObjVal object, an ObjDef object definition, or
# a string name <type>.  The -for flag indicates whether the viewer
# widget is for input or output.  The -parent indicates the parent
# containing the widget.  If the widget already exists, it is returned
# directly.  Otherwise, it is created and returned.
# ----------------------------------------------------------------------
proc Rappture::objects::viewer {what args} {
    variable objDefs

    # figure out the name of the desired object type
    if {[catch {$what isa ::Rappture::objects::ObjVal} valid] == 0 && $valid} {
        set type [[$what definition] type]
    } elseif {[catch {$what isa ::Rappture::objects::ObjDef} valid] == 0
                        && $valid} {
        set type [$what type]
    } else {
        set type [string tolower $what]  ;# doesn't matter: Number or number
        if {![info exists objDefs($type)]} {
            error "bad object type \"$type\": should be one of [join [lsort [array names objDefs]] {, }]"
        }
    }

    # process additional options
    array set opt {
        -for output
        -parent "."
    }
    foreach {key val} $args {
        if {![info exists opt($key)]} {
            error "bad option \"$key\": should be [join [array names opt] {, }]"
        }
        set opt($key) $val
    }
    if {![winfo exists $opt(-parent)]} {
        error "bad parent window \"$opt(-parent)\""
    }
    if {$opt(-parent) eq "."} {
        set opt(-parent) ""  ;# avoid ".." below when we say: $parent.foo
    }
    if {[lsearch {input output} $opt(-for)] < 0} {
        error "bad value \"$opt(-for)\": should be input, output"
    }

    # build the class name and widget name:
    #   class: Rappture::objects::CurveOutput
    #  widget: .foo.bar.curveOutput
    set which [string totitle $opt(-for)]
    set class "::Rappture::objects::[string totitle $type]$which"
    set win "$opt(-parent).v$type$opt(-for)"

    if {[winfo exists $win]} {
        return $win
    }
    if {[catch {$class $win} err] == 0} {
        return $win
    }
    return ""
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
# USAGE: Rappture::objects::check <type> <side> {<key> <val>...} <debugInfo>
#
# Checks the definition for an object of the given <type> to see if
# there are any errors in the values.  The <side> indicates whether
# it is an input or an output.  Some attributes don't apply when an
# object is an output.  The current attribute values are specified as
# a key/value list.  Returns a list of the form:
#   error "something went wrong"
#   warning "might check this"
# ----------------------------------------------------------------------
proc Rappture::objects::check {type side attrinfo debug} {
    variable objDefs

    set type [string tolower $type]  ;# doesn't matter: Tool or tool

    if {[info exists objDefs($type)]} {
        return [$objDefs($type) check $side $attrinfo $debug]
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
    variable currObjValDef
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
    catch {unset currObjValDef}
    array set currObjValDef {
        clear ""
        compare ""
        storage ""
        import ""
        export ""
        method ""
    }

    set currObjDef [Rappture::objects::ObjDef ::#auto $name -inherit $ilist]

    set cmds {
        # parse attribute definitions
        $attrParser eval $body

        # look for an image for this object
        set rootf [file rootname $currFile]
        foreach ext {png jpg gif} {
            if {[file readable $rootf.$ext] && 
		[catch {package present Tk}] == 0 && 
		[catch {package present Img}] == 0} {
                set imh [image create photo -file $rootf.$ext]
                $currObjDef configure -image $imh
                break
            }
        }

        #
        # Create a class to manage the object's value...
        #
        set ovdefn "inherit ::Rappture::objects::ObjVal\n"
        append ovdefn $currObjValDef(storage) "\n"
        append ovdefn "destructor { clear }\n"
        append ovdefn "public method clear {} [list $currObjValDef(clear)]\n"
        append ovdefn "public method definition {} {return $currObjDef}\n"

        # define extra methods added specially to this object
        foreach mn $currObjValDef(method) {
            append ovdefn [list public method $mn $currObjValDef(m-$mn-arglist) $currObjValDef(m-$mn-body)] "\n"
        }

        append ovdefn [format "private method importTypes {} { return %s }\n" [list $currObjValDef(import)]]
        append ovdefn [format "private method exportTypes {} { return %s }\n" [list $currObjValDef(export)]]

        # define methods to handle each import type
        foreach fmt $currObjValDef(import) {
            append ovdefn [list public method import_$fmt $currObjValDef(im-$fmt-arglist) $currObjValDef(im-$fmt-body)] "\n"
        }

        # define methods to handle each export type
        foreach fmt $currObjValDef(export) {
            append ovdefn [list public method export_$fmt $currObjValDef(ex-$fmt-arglist) $currObjValDef(ex-$fmt-body)] "\n"
        }

        # define the "compare" method
        set varcode ""
        foreach line [split $currObjValDef(storage) \n] {
            if {[regexp {(?:variable|common) +([a-zA-Z0-9_]+)} $line match var]} {
                append varcode "_linkvar import \$obj $var\n"
            }
        }
        if {$currObjValDef(compare) eq ""} {
            set currObjValDef(compare) "return 1"
        }
        append ovdefn [format { public method compare {obj} { %s %s }
            } $varcode $currObjValDef(compare)] "\n"

        append ovdefn {
            # utility used in "compare" method
            # this must be defined in each derived class at the most
            # specific scope, so that it has access to all of the storage
            # variables for the class.  If it's defined in the base class,
            # then it sees only the base class variables.
            protected method _linkvar {option args} {
              switch -- $option {
                export {
                    #
                    # Look for the variable in the current object scope
                    # and return a command that can be used to rebuild it.
                    #
                    set vname [lindex $args 0]
                    set suffix [lindex $args 1]
                    if {[array exists $vname]} {
                        return [list array set ${vname}${suffix} [array get $vname]]
                    } elseif {[info exists $vname]} {
                        return [list set ${vname}${suffix} [set $vname]]
                    } else {
                        return [list set ${vname}${suffix} ""]
                    }
                }
                import {
                    #
                    # The "_linkvar export" command produces a script that
                    # will replicate the variable.  Invoke this script in
                    # the calling context (uplevel) to copy the variable
                    # to the proper call stack.
                    #
                    set obj [lindex $args 0]
                    set vname [lindex $args 1]
                    uplevel [$obj _linkvar export $vname 2]
                }
                default {
                    error "bad option \"$option\": should be import, export"
                }
              }
            }
        }

        # create the object value class
        itcl::class "::Rappture::objects::[string totitle $name]Value" $ovdefn
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
# PARSER:  Rappture::objects::parse_clear
#
# Used internally to parse the definition of a clear block within a
# Rappture object definition:
#
#   clear <body>
#
# The clear block is a block of code that clears the storage variables
# before a new import operation, or whenever the object is destroyed.
# Frees any objects stored in the storage variables.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_clear {body} {
    variable currObjValDef

    if {$currObjValDef(clear) ne ""} {
        error "clear block already defined"
    }
    set currObjValDef(clear) $body
}

# ----------------------------------------------------------------------
# PARSER:  Rappture::objects::parse_compare
#
# Used internally to parse the definition of a compare block for the
# object value within a Rappture object definition:
#
#   compare <body>
#
# The compare block is a block of code that compares the value of
# the current object to another object, and returns -1/0/1, similar
# to str_cmp.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_compare {body} {
    variable currObjValDef

    if {$currObjValDef(compare) ne ""} {
        error "compare block already defined"
    }
    set currObjValDef(compare) $body
}

# ----------------------------------------------------------------------
# PARSER:  Rappture::objects::parse_export
#
# Used internally to parse the definition of an export scheme within
# a Rappture object definition:
#
#   export <name> <arglist> <body>
#
# The export <name> defines a data type that the object's value can
# be exported to.  The <arglist> arguments include the XML object, the
# file handle, etc, depending on the export type.  The <body> is a
# body of code invoked to handle the export operation.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_export {name arglist body} {
    variable currObjValDef

    set i [lsearch $currObjValDef(export) $name]
    if {$i >= 0} {
        error "export type \"$name\" already defined"
    }
    lappend currObjValDef(export) $name
    set currObjValDef(ex-$name-arglist) $arglist
    set currObjValDef(ex-$name-body) $body
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
# PARSER:  Rappture::objects::parse_import
#
# Used internally to parse the definition of an import scheme within
# a Rappture object definition:
#
#   import <name> <arglist> <body>
#
# The import <name> defines a data type that the object's value can
# be imported to.  The <arglist> arguments include the XML object, the
# file handle, etc, depending on the import type.  The <body> is a
# body of code invoked to handle the import operation.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_import {name arglist body} {
    variable currObjValDef

    set i [lsearch $currObjValDef(import) $name]
    if {$i >= 0} {
        error "import type \"$name\" already defined"
    }
    lappend currObjValDef(import) $name
    set currObjValDef(im-$name-arglist) $arglist
    set currObjValDef(im-$name-body) $body
}

# ----------------------------------------------------------------------
# PARSER:  Rappture::objects::parse_method
#
# Used internally to parse the definition of an object method within
# a Rappture object definition:
#
#   method <name> <arglist> <body>
#
# A method is an extra function supported by this object, used to
# query or modify the object value (usually by the GUI viewer).
# The <arglist> defines the arguments to the method, and the <body>
# is the body of code invoked to implement the method.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_method {name arglist body} {
    variable currObjValDef

    set i [lsearch $currObjValDef(method) $name]
    if {$i >= 0} {
        error "method \"$name\" already defined"
    }
    lappend currObjValDef(method) $name
    set currObjValDef(m-$name-arglist) $arglist
    set currObjValDef(m-$name-body) $body
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
# PARSER:  Rappture::objects::parse_storage
#
# Used internally to parse the definition of a storage block for the
# object value within a Rappture object definition:
#
#   storage {
#       private variable ...
#   }
#
# The storage block is added directly to a class defined to hold the
# object value.  Import/export code moves values into and out of the
# storage area.
# ----------------------------------------------------------------------
proc Rappture::objects::parse_storage {body} {
    variable currObjValDef

    if {$currObjValDef(storage) ne ""} {
        error "storage block already defined"
    }
    set currObjValDef(storage) $body
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

    constructor {type args} {
        set _type $type
        set _checks(num) 0
        eval configure $args
    }

    public method type {} {
        return $_type
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

    public method getAttr {args} {
        if {[llength $args] == 0} {
            set rlist ""
            foreach baseobj [cget -inherit] {
                eval lappend rlist [$baseobj getAttr]
            }
            eval lappend rlist $_attrs
            return $rlist
        } elseif {[llength $args] > 2} {
            error "wrong # args: should be \"getAttr ?name? ?-part?\""
        }

        set name [lindex $args 0]
        set part [lindex $args 1]

        # handle attributes defined right in this class
        if {[info exists _attr2def($name)]} {
            set rlist $name
            foreach opt [$_attr2def($name) configure] {
                if {[lindex $opt 0] eq $part} {
                    return [lindex $opt 2]
                }
                lappend rlist [lindex $opt 0] [lindex $opt 2]
            }
            return $rlist
        }

        # handle attributes defined in a base class
        foreach baseobj [cget -inherit] {
            set rval [eval $baseobj getAttr $name $part]
            if {$rval ne ""} {
                return $rval
            }
        }
        return ""
    }

    # call this to check the integrity of all values
    public method check {side data debug {extra ""}} {
        set rlist ""
        array set attr $data

        # code snippets sometimes use this object info
        if {$extra ne ""} {
            array set object $extra
        } else {
            array set object [list type [type] palettes $palettes help $help]
        }

        # handle checks defined in a base class
        foreach baseobj [cget -inherit] {
            eval lappend rlist [$baseobj check $side $data $debug [array get object]]
        }

        # add checks defined in the current class
        for {set n 1} {$n <= $_checks(num)} {incr n} {
            # look at the -only option and see if the check applies here
            set aname $_checks($n-attr)
            set only [getAttr $aname -only]
            if {$only ne "" && [lsearch $only $side] < 0} {
                continue
            }

            # execute the code to look for errors in the value
            set status [catch $_checks($n-code) result]
            if {$status != 0 && $status != 2} {
                puts stderr "ERROR DURING VALUE CHECK:\n$result"
            } elseif {[llength $result] > 0} {
                set class [lindex $result 0]
                set mesg [lindex $result 1]
                set dinfo $debug
                lappend dinfo -attribute $aname
                lappend rlist [list $class $mesg $dinfo]
            }
        }
        return $rlist
    }

    private variable _type ""   ;# type name (lowercase) for object type
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
    public variable only ""
    public variable expand "no"
    public variable tooltip ""

    constructor {args} {
        eval configure $args
    }
}

# ----------------------------------------------------------------------
#  CLASS: ObjVal
#  Able to import/export the value for a particular object class.
# ----------------------------------------------------------------------
itcl::class Rappture::objects::ObjVal {
    public method definition {} { # returns the ObjDef class for this value }

    public method attr {option args} {
        switch -- $option {
            get {
                if {[llength $args] == 0} {
                    return [[$this definition] getAttr]
                } elseif {[llength $args] == 1} {
                    set name [lindex $args 0]
                    if {[catch {[$this definition] getAttr $name}]} {
                        error "attribute \"$name\" not defined on $this"
                    }
                    if {[info exists attr($name)]} {
                        return $attr($name)
                    }
                    return ""
                } else {
                    error "wrong # args: should be \"attr get ?name?\""
                }
            }
            set {
                if {[llength $args] != 2} {
                    error "wrong # args: should be \"attr set name value\""
                }
                set name [lindex $args 0]
                set val [lindex $args 1]
                if {[catch {[$this definition] getAttr $name}] == 0} {
                    set attr($name) $val
                }
                return $val
            }
            info {
                if {[llength $args] == 1} {
                    set name [lindex $args 0]
                    return [[$this definition] getAttr $name]
                } else {
                    error "wrong # args: should be \"attr info name\""
                }
            }
            import {
                if {[llength $args] != 2} {
                    error "wrong # args: should be \"attr import xmlobj path\""
                }
                set xmlobj [lindex $args 0]
                set path [lindex $args 1]

                set odef [$this definition]
                foreach name [$odef getAttr] {
                    set tail [$odef getAttr $name -path]
                    set apath $path.$tail
                    if {[$xmlobj element -as type $apath] ne ""} {
                        set attr($name) [$xmlobj get $apath]
                    }
                }
            }
            export {
                if {[llength $args] != 2} {
                    error "wrong # args: should be \"attr export xmlobj path\""
                }
                set xmlobj [lindex $args 0]
                set path [lindex $args 1]

                set odef [$this definition]
                foreach name [$odef getAttr] {
                    if {[info exists attr($name)]} {
                        set tail [$odef getAttr $name -path]
                        $xmlobj put $path.$tail $attr($name)
                    }
                }
            }
            default {
                error "bad option \"$option\": should be get, set, info, import, export"
            }
        }
    }
    protected variable attr  ;# maps attribute name => value

    public method clear {} { # nothing to do for base class }

    public method import {pattern args} {
        clear
        set errs ""

        # scan through all matching types and try to import the value
        foreach type [importTypes] {
            if {[string match $pattern $type]} {
                set cmd [format {eval $this import_%s $args} $type]
                if {[catch $cmd result] == 0} {
                    return 1
                }
                lappend errs "not $type: $result"
            }
        }
        return [concat 0 $errs]
    }

    public method export {pattern args} {
        set errs ""

        # scan through all matching types and try to export the value
        foreach type [exportTypes] {
            if {[string match $pattern $type]} {
                set cmd {uplevel $this export_$type $args}
                if {[catch $cmd result] == 0} {
                    return 1
                }
                lappend errs "not $type: $result"
            }
        }
        return [concat 0 $errs]
    }

    private method importTypes {} { # derived classes override this }
    private method exportTypes {} { # derived classes override this }

    # utility used in "compare" method
    # links a variable $vname from object $obj into the current scope
    # with a similar variable name, but with $suffix on the end
    #
    # usage: _linkvar import _foo 2
    #
    # this triggers a call to "$obj _linkvar export" to produce a command
    # that can be used to rebuild the desired variable from $obj into the
    # local context.
    #
    # the _linkvar method must be defined in each derived class so that
    # it has access to variables in the most-specific object context.
    protected method _linkvar {option args} {
        error "derived classes should override this method"
    }

    # utility to compare two double-prec numbers within a tolerance
    proc cmpdbl {num1 num2 {max ""}} {
        set mag [expr {0.5*(abs($num1)+abs($num2))}]
        set diff [expr {abs($num1-$num2)}]

        if {$diff <= 1e-6*$mag} {
            # very small difference
            return 0
        } elseif {$max ne "" && $mag <= 1e-6*abs($max)} {
            # very small numbers -- treat them as zero
            return 0
        } elseif {$num1 < $num2} {
            return -1
        } else {
            return 1
        }
    }
}
