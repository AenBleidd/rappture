# ----------------------------------------------------------------------
#  COMPONENT: library - provides access to the XML library
#
#  These routines make it easy to load the XML description for a
#  series of tool parameters and browse through the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require tdom
package require Itcl

namespace eval Rappture { # forward declaration }
namespace eval Rappture::Library { # forward declaration }

# ----------------------------------------------------------------------
# USAGE: open ?-std? <file>
#
# Used to open a <file> containing an XML description of tool
# parameters.  Loads the file and returns the name of the LibraryObj
# file that represents it.
#
# If the -std flag is included, then the file is treated as the
# name of a standard file, which is part of the Rappture installation.
# ----------------------------------------------------------------------
proc Rappture::Library::open {args} {
    set stdfile 0
    while {[llength $args] > 1} {
        set switch [lindex $args 0]
        set args [lrange $args 1 end]
        if {$switch == "-std"} {
            set stdfile 1
        } else {
            error "bad option \"$switch\": should be -std"
        }
    }
    set fname [lindex $args 0]

    if {$stdfile && [file pathtype $fname] != "absolute"} {
        set fname [file join $Rappture::installdir lib $fname]
    }

    # otherwise, try to open the file and create its LibraryObj
    set fid [::open $fname r]
    set info [read $fid]
    close $fid

    set obj [Rappture::LibraryObj ::#auto $info]
    return $obj
}

# ----------------------------------------------------------------------
# USAGE: valid <obj>
#
# Checks to see if the given object is a valid Library.  Returns 1
# if so, and 0 otherwise.
# ----------------------------------------------------------------------
proc Rappture::Library::valid {obj} {
    if {[catch {$obj isa ::Rappture::LibraryObj} valid] == 0 && $valid} {
        return 1
    }
    return 0
}

# ----------------------------------------------------------------------
itcl::class Rappture::LibraryObj {
    constructor {info} { # defined below }
    destructor { # defined below }

    public method get {args}
    public method put {args}
    public method xml {}

    protected method find {path}
    protected method path2list {path}

    private variable _root 0       ;# non-zero => this obj owns document
    private variable _document ""  ;# XML DOM tree
    private variable _node ""      ;# node within 
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::constructor {info} {
    if {[regexp {<?[Xx][Mm][Ll]} $info]} {
        set _root 1
        set _document [dom parse $info]
        set _node [$_document documentElement]
    } elseif {[regexp {^domNode} $info]} {
        set _root 0
        set _document [$info ownerDocument]
        set _node $info
    } else {
        error "bad info: should be XML text or DOM node"
    }
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::destructor {} {
    if {$_root && $_document != ""} {
        $_document delete
    }
}

# ----------------------------------------------------------------------
# USAGE: get ?-exists|-object|-type|-info|-count|-children? ?<path>?
#
# Searches the DOM inside this object for the information on the
# requested <path>.  By default, it returns the -info associated
# with the path.  The other flags can be used to query other
# aspects of the information at the requested node.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::get {args} {
    set format -info
    while {[llength $args] > 0} {
        set first [lindex $args 0]
        if {[string index $first 0] == "-"} {
            set choices {-exists -object -type -info -count -children}
            if {[lsearch $choices $first] < 0} {
                error "bad option \"$first\": should be [join [lsort $choices] {, }]"
            }
            set format $first
            set args [lrange $args 1 end]
        } else {
            break
        }
    }
    if {[llength $args] > 1} {
        error "wrong # args: should be \"get ?-exists? ?-object? ?-type? ?-info? ?-count? ?-children? ?path?\""
    }
    set path [lindex $args 0]

    set node [find $path]

    switch -- $format {
        -exists {
            if {$node != ""} {
                return 1
            }
            return 0
        }
        -object {
            if {$node != ""} {
                return [::Rappture::LibraryObj ::#auto $node]
            }
            return ""
        }
        -info {
            if {$node != ""} {
                return [string trim [$node text]]
            }
            return ""
        }
        -type   {
            if {$node != ""} {
                return [$node nodeName]
            }
        }
        -count {
            if {$node == ""} {
                return ""
            }
            set node [$node parent]
            set type [lindex [path2list $path] end]
            set nlist [$node getElementsByTagName $type]
            return [llength $nlist]
        }
        -children {
            if {$node == ""} {
                return ""
            }
            set rlist ""
            set nlist [$node childNodes]
            foreach n $nlist {
                set type [$n nodeName]
                if {[regexp {^#} $type]} {
                    continue
                }
                if {![info exists count($type)]} {
                    set count($type) 0
                    lappend rlist $type
                } else {
                    lappend rlist "$type[incr count($type)]"
                }
            }
            return $rlist
        }
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: put <path> ?-text|-object? <string>
#
# Inserts information into the DOM represented by this object.
# The <path> is a path to the insertion point, which uses a syntax
# similar to the "get" method.  The <string> being inserted can either
# be ordinary text, or another LibraryObj object.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::put {args} {
    set what "-text"
    set path [lindex $args 0]
    set args [lrange $args 1 end]
    while {[llength $args] > 1} {
        set first [lindex $args 0]
        set args [lrange $args 1 end]
        if {$first != "-text" && $first != "-object"} {
            error "bad option \"$first\": should be -object, -text"
        }
        set what $first
    }
    if {[llength $args] != 1} {
        error "wrong # args: should be \"put path ?-text? ?-object? string\""
    }
    set str [lindex $args 0]

    switch -- $what {
        -text {
            set node [find -create $path]
            foreach n [$node childNodes] {
                if {[$n nodeType] == "TEXT_NODE"} {
                    $n delete
                }
            }
            set n [$_document createText $str]
            $node appendChild $n
        }
        -object {
            error "not yet implemented"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: xml
#
# Returns a string representing the XML information for the information
# in this library.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::xml {} {
    return [$_node asXML]
}

# ----------------------------------------------------------------------
# USAGE: find ?-create? <path>
#
# Searches from the starting node for this object according to the
# given <path>.  Returns the node found at the end of the path,
# or "" if the node was not found.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::find {args} {
    set create 0
    while {[llength $args] > 1} {
        set first [lindex $args 0]
        set args [lrange $args 1 end]
        if {$first == "-create"} {
            set create 1
        } else {
            error "bad option \"$first\": should be -create"
        }
    }
    if {[llength $args] != 1} {
        error "wrong # args: should be \"find ?-create? path\""
    }
    set path [path2list [lindex $args 0]]

    #
    # Follow the given path and look for all of the parts.
    #
    set nnum 1
    set lastnode $_node
    set node $lastnode
    foreach part $path {
        if {[regexp {^([a-zA-Z_]+)([0-9]*)$} $part match type index]
              && ($index != "" || [$node getElementsByTagName $type] != "")} {
            #
            # If the name is like "type2", then look for elements with
            # the type name and return the one with the given index.
            # If the name is like "type", then assume the index is 0.
            #
            if {$index == ""} {
                set index 0
            }
            set nlist [$node getElementsByTagName $type]
            set node [lindex $nlist $index]
        } elseif {[regexp {^([a-zA-Z_]+)\(([^\)]*)\)$} $part match type name]} {
            #
            # If the name is like "type(name)", then look for elements
            # that match the type and see if one has the requested name.
            #
            set nlist [$node getElementsByTagName $type]
            set found 0
            foreach n $nlist {
                if {[catch {$n getAttribute name} tag]} { set tag "" }
                if {$tag == $name} {
                    set found 1
                    break
                }
            }
            set node [expr {($found) ? $n : ""}]
        } else {
            #
            # Otherwise, the name might be something like "name".
            # Scan through all elements and see if any has the
            # matching name.
            #
            set nlist [$node childNodes]
            set found 0
            foreach n $nlist {
                if {[catch {$n getAttribute name} tag]} { set tag "" }
                if {$tag == $part} {
                    set found 1
                    break
                }
            }
            set node [expr {($found) ? $n : ""}]
        }

        if {$node == ""} {
            if {!$create} {
                return ""
            }
            if {![regexp {^([^\(]+)\(([^\)]+)\)$} $part match type name]} {
                set type $part
                set name ""
            }
            set node [$_document createElement $type]
            $lastnode appendChild $node

            if {"" != $name} {
                $node setAttribute name $name
            }
        }
        set lastnode $node
        incr nnum
    }
    return $node
}

# ----------------------------------------------------------------------
# USAGE: path2list <path>
#
# Converts a path of the form "foo(a).bar.baz" into a list of the
# form "foo(a) bar baz".  This is a little more complicated than
# splitting on the .'s, since the stuff in ()'s might have embedded
# .'s.  Returns a proper Tcl list for all elements of the path.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::path2list {path} {
    #
    # Normally, we just split on .'s within the path.  But there
    # might be some .'s embedded within ()'s in the path.  Change
    # any embedded .'s to an out-of-band character, then split on
    # the .'s, and change the embedded .'s back.
    #
    if {[regexp {(\([^\)]*)\.([^\)]*\))} $path]} {
        while {[regsub -all {(\([^\)]*)\.([^\)]*\))} $path "\\1\007\\2" path]} {
            # keep fixing...
        }
    }
    set path [split $path .]
    regsub -all {\007} $path {.} path

    return $path
}
