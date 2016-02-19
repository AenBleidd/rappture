# ----------------------------------------------------------------------
#  COMPONENT: library - provides access to the XML library
#
#  These routines make it easy to load the XML description for a
#  series of tool parameters and browse through the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require tdom
package require Itcl

namespace eval Rappture {
    variable stdlib ""
}

# load the object system along with the XML library code
Rappture::objects::init
encoding system utf-8

# ----------------------------------------------------------------------
# USAGE: library <file>
# USAGE: library standard
# USAGE: library isvalid <object>
#
# Used to open a <file> containing an XML description of tool
# parameters.  Loads the file and returns the name of the LibraryObj
# file that represents it.
#
# If you use the word "standard" in place of the file name, this
# function returns the standard Rappture library object, which
# contains material definitions.
#
# The isvalid operation checks an <object> to see if it is a valid
# library object.  Returns 1 if so, and 0 otherwise.
# ----------------------------------------------------------------------
proc Rappture::library {args} {
    # handle the isvalid operation...
    if {[llength $args] > 1 && [lindex $args 0] == "isvalid"} {
        if {[llength $args] != 2} {
            error "wrong # args: should be \"library isvalid object\""
        }
        set obj [lindex $args 1]
        #
        # BE CAREFUL with the object test:
        # The command should look like a LibraryObj formed by #auto.
        # We want to avoid things like "split" or "set", which are
        # valid Tcl commands but won't respond well to isa.
        #
        if {[regexp {libraryObj[0-9]+$} $obj]
              && [catch {$obj isa ::Rappture::LibraryObj} valid] == 0
              && $valid} {
            return 1
        }
        return 0
    }

    if {[llength $args] != 1} {
        error "wrong # args: should be \"library file\" or \"library isvalid object\""
    }
    set fname [lindex $args 0]

    if {$fname == "standard"} {
        variable stdlib
        if {$stdlib != ""} {
            return $stdlib
        }
        set fname [file join $Rappture::installdir lib library.xml]
        if { [catch {
            set fid [::open $fname r]
            set info [read $fid]
            close $fid
        } errs] != 0 } {
            puts stderr "can't open \"$fname\": errs=$errs errorInfo=$errorInfo"
        }
        set stdlib [Rappture::LibraryObj ::#auto $info]
        return $stdlib
    }

    if {[regexp {^<\?[Xx][Mm][Ll]} $fname]} {
        set info $fname
    } else {
        # otherwise, try to open the file and create its LibraryObj
        if { [catch {
            set fid [::open $fname r]
            set info [read $fid]
            close $fid
        } errs] != 0 } {
            puts stderr "can't open \"$fname\": errs=$errs errorInfo=$errorInfo"
        }
    }

    set obj [Rappture::LibraryObj ::#auto $info]
    return $obj
}

# ----------------------------------------------------------------------
# USAGE: entities ?-as <fval>? <object> <path>
#
# Used to sift through an XML <object> for "entities" within the
# Rappture description.  Entities are things like strings, numbers,
# etc., which show up in the GUI as controls.
#
# Returns a list of all entities found beneath <path>.
#
# By default, this method returns the component name "type(id)".
# This is changed by setting the -as argument to "id" (for name
# of the tail element), to "type" (for the type of the tail element),
# to "object" (for an object representing the DOM node referenced by
# the path.
# ----------------------------------------------------------------------
proc Rappture::entities {args} {
    array set params {
        -as component
    }
    while {[llength $args] > 1} {
        set first [lindex $args 0]
        if {[string index $first 0] == "-"} {
            set choices [array names params]
            if {[lsearch $choices $first] < 0} {
                error "bad option \"$first\": should be [join [lsort $choices] {, }]"
            }
            set params($first) [lindex $args 1]
            set args [lrange $args 2 end]
        } else {
            break
        }
    }
    if {[llength $args] > 2} {
        error "wrong # args: should be \"entities ?-as fval? obj ?path?\""
    }
    set xmlobj [lindex $args 0]
    set path [lindex $args 1]

    set rlist ""
    lappend queue $path
    while {[llength $queue] > 0} {
        set path [lindex $queue 0]
        set queue [lrange $queue 1 end]

        foreach cpath [$xmlobj children -as path $path] {
            set type [$xmlobj element -as type $cpath]
            switch -- $type {
                group - phase {
                    lappend queue $cpath
                }
                structure {
                    # add this to the return list with the right flavor
                    if {$params(-as) == "component"} {
                        lappend rlist $cpath
                    } else {
                        lappend rlist [$xmlobj element -as $params(-as) $cpath]
                    }

                    if {[$xmlobj element $cpath.current.parameters] != ""} {
                        lappend queue $cpath.current.parameters
                    }
                }
                drawing {
                    # add this to the return list with the right flavor
                    if {$params(-as) == "component"} {
                        lappend rlist $cpath
                    } else {
                        lappend rlist [$xmlobj element -as $params(-as) $cpath]
                    }

                    foreach child [$xmlobj children $cpath.current] {
                        if {[string match about* $child]} {
                            continue
                        }
                        if {[$xmlobj element $cpath.current.$child.parameters] != ""} {
                            lappend queue $cpath.current.$child.parameters
                        }
                    }
                }
                loader {
                    if {$params(-as) == "component"} {
                        lappend rlist $cpath
                    } else {
                        lappend rlist [$xmlobj element -as $params(-as) $cpath]
                    }
		}
                default {
                    if {[catch {Rappture::objects::get $type}] == 0} {
                        # add this to the return list with the right flavor
                        if {$params(-as) == "component"} {
                            lappend rlist $cpath
                        } else {
                            lappend rlist [$xmlobj element -as $params(-as) $cpath]
                        }
                    }

                    # if this element has embedded groups, add them to the queue
                    foreach ccpath [$xmlobj children -as path $cpath] {
                        set cctype [$xmlobj element -as type $ccpath]
                        if {$cctype == "group" || $cctype == "phase"} {
                            lappend queue $ccpath
                        }
                    }
                }
            }
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
itcl::class Rappture::LibraryObj {
    constructor {info} { # defined below }
    destructor { # defined below }

    public method element {args}
    public method parent {args}
    public method children {args}
    public method get {args}
    public method put {args}
    public method copy {path from args}
    public method remove {{path ""}}
    public method xml {{path ""}}
    public method uq_get_vars {{tfile ""}}

    public method diff {libobj}
    public proc value {libobj path}

    public proc path2list {path}
    protected method find {path}
    protected method node2name {node}
    protected method node2comp {node}
    protected method node2path {node}
    protected method childnodes {node type}

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
# USAGE: element ?-as <fval>? ?<path>?
#
# Clients use this to query a particular element within the entire
# data structure.  The path is a string of the form
# "structure.box(source).corner".  This example represents the tag
# <corner> within a tag <box id="source"> within a tag <structure>,
# which must be found at the top level within this document.
#
# By default, this method returns the component name "type(id)".
# This is changed by setting the -as argument to "id" (for name
# of the tail element), to "type" (for the type of the tail element),
# to "object" (for an object representing the DOM node referenced by
# the path).
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::element {args} {
    array set params {
        -as component
    }
    while {[llength $args] > 1} {
        set first [lindex $args 0]
        if {[string index $first 0] == "-"} {
            set choices [array names params]
            if {[lsearch $choices $first] < 0} {
                error "bad option \"$first\": should be [join [lsort $choices] {, }]"
            }
            set params($first) [lindex $args 1]
            set args [lrange $args 2 end]
        } else {
            break
        }
    }
    if {[llength $args] > 1} {
        error "wrong # args: should be \"element ?-as fval? ?path?\""
    }
    set path [lindex $args 0]

    set node [find $path]
    if {$node == ""} {
        return ""
    }

    switch -- $params(-as) {
      object {
          return [::Rappture::LibraryObj ::#auto $node]
      }
      component {
          return [node2comp $node]
      }
      id {
          return [node2name $node]
      }
      path {
          return [node2path $node]
      }
      type {
          return [$node nodeName]
      }
      default {
          error "bad flavor \"$params(-as)\": should be component, id, object, path, type"
      }
    }
}

# ----------------------------------------------------------------------
# USAGE: parent ?-as <fval>? ?<path>?
#
# Clients use this to query the parent of a particular element.
# This is just like the "element" method, but it returns the parent
# of the element instead of the element itself.
#
# By default, this method returns a list of component names "type(id)".
# This is changed by setting the -as argument to "id" (for tail
# names of all children), to "type" (for the types of all children),
# to "object" (for a list of objects representing the DOM nodes for
# all children).
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::parent {args} {
    array set params {
        -as component
    }
    while {[llength $args] > 1} {
        set first [lindex $args 0]
        if {[string index $first 0] == "-"} {
            set choices [array names params]
            if {[lsearch $choices $first] < 0} {
                error "bad option \"$first\": should be [join [lsort $choices] {, }]"
            }
            set params($first) [lindex $args 1]
            set args [lrange $args 2 end]
        } else {
            break
        }
    }
    if {[llength $args] > 1} {
        error "wrong # args: should be \"parent ?-as fval? ?path?\""
    }
    set path [lindex $args 0]

    set node [find $path]
    if {$node == ""} {
        return ""
    }
    set node [$node parentNode]
    if {$node == ""} {
        return ""
    }

    switch -- $params(-as) {
      object {
          return [::Rappture::LibraryObj ::#auto $node]
      }
      component {
          return [node2comp $node]
      }
      id {
          return [node2name $node]
      }
      path {
          return [node2path $node]
      }
      type {
          return [$node nodeName]
      }
      default {
          error "bad flavor \"$params(-as)\": should be component, id, object, path, type"
      }
    }
}

# ----------------------------------------------------------------------
# USAGE: children ?-as <fval>? ?-type <name>? ?<path>?
#
# Clients use this to query the children of a particular element
# within the entire data structure.  This is just like the "element"
# method, but it returns the children of the element instead of the
# element itself.  If the optional -type argument is specified, then
# the return list is restricted to children of the specified type.
#
# By default, this method returns a list of component names "type(id)".
# This is changed by setting the -as argument to "id" (for tail
# names of all children), to "type" (for the types of all children),
# to "object" (for a list of objects representing the DOM nodes for
# all children).
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::children {args} {
    array set params {
        -as component
        -type ""
    }
    while {[llength $args] > 1} {
        set first [lindex $args 0]
        if {[string index $first 0] == "-"} {
            set choices [array names params]
            if {[lsearch $choices $first] < 0} {
                error "bad option \"$first\": should be [join [lsort $choices] {, }]"
            }
            set params($first) [lindex $args 1]
            set args [lrange $args 2 end]
        } else {
            break
        }
    }
    if {[llength $args] > 1} {
        error "wrong # args: should be \"children ?-as fval? ?-type name? ?path?\""
    }
    set path [lindex $args 0]
    set node [find $path]
    if {$node == ""} {
        return ""
    }

    set nlist ""
    foreach n [$node childNodes] {
        set type [$n nodeName]
        if {[regexp {^#} $type]} {
            continue
        }
        if {$params(-type) != "" && $params(-type) != $type} {
            continue
        }
        lappend nlist $n
    }

    set rlist ""
    switch -- $params(-as) {
      object {
          foreach n $nlist {
              lappend rlist [::Rappture::LibraryObj ::#auto $n]
          }
      }
      component {
          foreach n $nlist {
              lappend rlist [node2comp $n]
          }
      }
      id {
          foreach n $nlist {
              lappend rlist [node2name $n]
          }
      }
      path {
          foreach n $nlist {
              lappend rlist [node2path $n]
          }
      }
      type {
          foreach n $nlist {
              lappend rlist [$n nodeName]
          }
      }
      default {
          error "bad flavor \"$params(-as)\": should be component, id, object, path, type"
      }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: get ?-decode yes? ?<path>?
#
# Clients use this to query the value of a node. Clients can specify
# if they want the data to be automatically decoded or no using the
# -decode flag. This is useful for situations where you want to keep
# the data encoded to pass to another system, like dx data in fields
# sending data to nanovis. If the path is not
# specified, it returns the value associated with the root node.
# Otherwise, it returns the value for the element specified by the
# path.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::get {args} {
    array set params {
        -decode yes
    }
    while {[llength $args] > 0} {
        set first [lindex $args 0]
        if {[string index $first 0] == "-"} {
            set choices [array names params]
            if {[lsearch $choices $first] < 0} {
                error "bad option \"$first\": should be [join [lsort $choices] {, }]"
            }
            set params($first) [lindex $args 1]
            set args [lrange $args 2 end]
        } else {
            break
        }
    }
    if {[llength $args] > 1} {
        error "wrong # args: should be \"get ?-decode yes? ?path?\""
    }
    if {[llength $args] == 1} {
        set path [lindex $args 0]
    } else {
        set path ""
    }

    set node [find $path]
    if {$node == ""} {
        return ""
    }
    set string [string trim [$node text]]
    if {$params(-decode)} {
	set string [Rappture::encoding::decode -- $string]
    }
    return $string
}

# ----------------------------------------------------------------------
# USAGE: put ?-append yes? ?-id num? ?-type string|file? ?-compress no? ?<path>? <string>
#
# Clients use this to set the value of a node.  If the path is not
# specified, it sets the value for the root node.  Otherwise, it sets
# the value for the element specified by the path.  If the value is a
# string, then it is treated as the text within the tag at the tail
# of the path.  If it is a DOM node or a library, then it is inserted
# into the tree at the specified path.
#
# If the optional id is specified, then it sets the identifier for
# the tag at the tail of the path.  If the optional append flag is
# specified, then the value is appended to the current value.
# Otherwise, the value replaces the current value.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::put {args} {
    array set params {
        -id ""
        -append no
        -type string
        -compress no
    }
    while {[llength $args] > 1} {
        set first [lindex $args 0]
        if {[string index $first 0] == "-"} {
            set choices [array names params]
            if {[lsearch $choices $first] < 0} {
                error "bad option \"$first\": should be [join [lsort $choices] {, }]"
            }
            set params($first) [lindex $args 1]
            set args [lrange $args 2 end]
        } else {
            break
        }
    }
    if {[llength $args] < 1 || [llength $args] > 2} {
        error "wrong # args: should be \"put ?-append bval? ?-id num? ?-type string|file? ?-compress bval? ?path? string\""
    }
    if {[llength $args] == 2} {
        set path [lindex $args 0]
        set str [lindex $args 1]
    } else {
        set path ""
        set str [lindex $args 0]
    }

    if {$params(-type) == "file"} {
        set fileName $str
        set fid [open $fileName r]
        fconfigure $fid -translation binary -encoding binary
        set str [read $fid]
        close $fid
    }

    if {$params(-compress) || [Rappture::encoding::is binary $str]} {
        set str [Rappture::encoding::encode -- $str]
    }

    set node [find -create $path]

    #
    # Clean up any nodes that don't belong.  If we're appending
    # the value, then clean up only child <tag> nodes.  Otherwise,
    # clean up all nodes.
    #
    set nlist ""
    if {$params(-append)} {
        foreach n [$node childNodes] {
            if {[$n nodeType] != "TEXT_NODE"} {
                lappend nlist $n
            }
        }
    } else {
        set nlist [$node childNodes]
    }
    foreach n $nlist {
        $n delete
    }

    if {[Rappture::library isvalid $str]} {
        foreach n [[$str info variable _node -value] childNodes] {
	    if { [$n nodeType] == "COMMENT_NODE" } {
		continue
	    }
            $node appendXML [$n asXML]
        }
    } else {
        set n [$_document createText $str]
        $node appendChild $n
        if {"" != $params(-id)} {
            $node setAttribute id $params(-id)
        }
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: copy <path> from ?<xmlobj>? <path>
#
# Clients use this to copy the value from one xmlobj/path to another.
# If the <xmlobj> is not specified, it is assumed to be the same as
# the current object.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::copy {path from args} {
    if {[llength $args] == 1} {
        set xmlobj $this
        set fpath [lindex $args 0]
    } elseif {[llength $args] == 2} {
        set xmlobj [lindex $args 0]
        set fpath [lindex $args 1]
    } else {
        error "wrong # args: should be \"copy path from ?xmlobj? path\""
    }
    if {$from != "from"} {
        error "bad syntax: should be \"copy path from ?xmlobj? path\""
    }

    if {[llength [$xmlobj children $fpath]] == 0} {
        set val [$xmlobj get $fpath]
        put $path $val
    } else {
        set obj [$xmlobj element -as object $fpath]
        put $path $obj
        itcl::delete object $obj
    }
}

# ----------------------------------------------------------------------
# USAGE: remove ?<path>?
#
# Clients use this to remove the specified node.  Removes the node
# from the tree.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::remove {{path ""}} {
    set node [find $path]
    if {$node != ""} {
        $node delete
    }
}

# ----------------------------------------------------------------------
# USAGE: xml ?<path>?
#
# Returns a string representing the XML information for the information
# in this library.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::xml {{path ""}} {
    if {"" != $path} {
        set n [find $path]
    } else {
        set n $_node
    }
    return [$n asXML]
}

# ----------------------------------------------------------------------
# USAGE: diff <libobj>
#
# Compares the entities in this object to those in another and
# returns a list of differences.  The result is a list of the form:
# {op1 path1 oldval1 newval1 ...} where each "op" is +/-/c for
# added/subtracted/changed, "path" is the path within the library
# that is different, and "oldval"/"newval" give the values for the
# object at the path.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::diff {libobj} {
    set rlist ""

    # query the values for all entities in both objects
    set thisv [Rappture::entities $this input]
    set otherv [Rappture::entities $libobj input]

    # add UQ checking
    lappend thisv uq.type uq.args
    lappend otherv uq.type uq.args

    # scan through values for this object, and compare against other one
    foreach path $thisv {
        set i [lsearch -exact $otherv $path]
        if {$i < 0} {
            foreach {raw norm} [value $this $path] break
            lappend rlist - $path $raw ""
        } else {
            foreach {traw tnorm} [value $this $path] break
            foreach {oraw onorm} [value $libobj $path] break
            if {![string equal $tnorm $onorm]} {
                lappend rlist c $path $traw $oraw
            }
            set otherv [lreplace $otherv $i $i]
        }
    }

    # add any values left over in the other object
    foreach path $otherv {
        foreach {oraw onorm} [value $libobj $path] break
        lappend rlist + $path "" $oraw
    }

    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: value <object> <path>
#
# Used to query the "value" associated with the <path> in an XML
# <object>.  This is a little more complicated than the object's
# "get" method.  It handles things like structures and values
# with normalized units.
#
# Returns a list of two items:  {raw norm} where "raw" is the raw
# value from the "get" method and "norm" is the normalized value
# produced by this routine.  Example:  {300K 300}
#
# Right now, it is a handy little utility used by the "diff" method.
# Eventually, it should be moved to a better object-oriented
# implementation, where each Rappture type could overload the
# various bits of processing below.  So we leave it as a "proc"
# now instead of a method, since it should be deprecated soon.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::value {libobj path} {
    switch -- [$libobj element -as type $path] {
        structure {
            set raw $path
            # try to find a label to represent the structure
            set val [$libobj get $path.about.label]
            if {"" == $val} {
                set val [$libobj get $path.current.about.label]
            }
            if {"" == $val} {
                if {[$libobj element $path.current] != ""} {
                    set comps [$libobj children $path.current.components]
                    set val "<structure> with [llength $comps] components"
                } else {
                    set val "<structure>"
                }
            }
            return [list $raw $val]
        }
        number {
            # get the usual value...
            set raw ""
            set val ""
            if {"" != [$libobj element $path.current]} {
                set raw [$libobj get $path.current]
            } elseif {"" != [$libobj element $path.default]} {
                set raw [$libobj get $path.default]
            }
            if {"" != $raw} {
                set val $raw
                # then normalize to default units
                set units [$libobj get $path.units]
                if {"" != $units} {
                    set val [Rappture::Units::mconvert $val \
                        -context $units -to $units -units off]
                }
            }
            return [list $raw $val]
        }
    }

    # for all other types, get the value (current, or maybe default)
    set raw ""
    if {"" != [$libobj element $path.current]} {
        set raw [$libobj get $path.current]
    } elseif {"" != [$libobj element $path.default]} {
        set raw [$libobj get $path.default]
    }
    return [list $raw $raw]
}

# ----------------------------------------------------------------------
# USAGE: find ?-create? <path>
#
# Used internally to find a particular element within the root node
# according to the path, which is a string of the form
# "typeNN(id).typeNN(id). ...", where each "type" is a tag <type>;
# if the optional NN is specified, it indicates an index for the
# <type> tag within its parent; if the optional (id) part is included,
# it indicates a tag of the form <type id="id">.
#
# By default, it looks for an element along the path and returns None
# if not found.  If the create flag is set, it creates various elements
# along the path as it goes.  This is useful for "put" operations.
#
# If you include "#" instead of a specific number, a node will be
# created automatically with a new number.  For example, the path
# "foo.bar#" called the first time will create "foo.bar", the second
# time "foo.bar1", the third time "foo.bar2" and so forth.
#
# Returns an object representing the element indicated by the path,
# or "" if the path is not found.
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
    set path [lindex $args 0]

    if {$path == ""} {
        return $_node
    }
    set path [path2list $path]

    #
    # Follow the given path and look for all of the parts.
    #
    set lastnode $_node
    set node $lastnode
    foreach part $path {
        if {![regexp {^(([a-zA-Z0-9_]*[a-zA-Z_]+#?)([0-9]*))?(\((.*)\))?$} $part \
               match dummy type index dummy name]} {
            error "bad path component \"$part\""
        }
        #
        # If the name is like "type2", then look for elements with
        # the type name and return the one with the given index.
        # If the name is like "type", then assume the index is 0.
        #
        if {$name == ""} {
            if {$index == ""} {
                set index 0
            }
            set nlist [childnodes $node $type]
            set node [lindex $nlist $index]
        } else {
            #
            # If the name is like "type(id)", then look for elements
            # that match the type and see if one has the requested name.
            # if the name is like "(id)", then look for any elements
            # with the requested name.
            #
            if {$type != ""} {
                set nlist [childnodes $node $type]
            } else {
                set nlist [$node childNodes]
            }
            set found 0
            foreach n $nlist {
                if {[catch {$n getAttribute id} tag]} { set tag "" }
                if {$tag == $name} {
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

            #
            # If the "create" flag is set, then create a node
            # with the specified "type(id)" and continue on.
            # If the type is "type#", then create a node with
            # an automatic number.
            #
            if {![regexp {^([^\(]+)\(([^\)]+)\)$} $part match type name]} {
                set type $part
                set name ""
            }

            if {[string match *# $type]} {
                set type [string trimright $type #]
                set node [$_document createElement $type]

                # find the last node of same type and append there
                set pos ""
                foreach n [$lastnode childNodes] {
                    if {[$n nodeName] == $type} {
                        set pos $n
                    }
                }
                if {$pos != ""} {
                    set pos [$pos nextSibling]
                }
                if {$pos != ""} {
                    $lastnode insertBefore $node $pos
                } else {
                    $lastnode appendChild $node
                }
            } else {
                set node [$_document createElement $type]
                $lastnode appendChild $node
            }
            if {"" != $name} {
                $node setAttribute id $name
            }
        }
        set lastnode $node
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

# ----------------------------------------------------------------------
# USAGE: node2name <node>
#
# Used internally to create a name for the specified node.  If the
# node doesn't have a specific name ("id" attribute) then a name of
# the form "type123" is constructed.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::node2name {node} {
    if {[catch {$node getAttribute id} name]} { set name "" }
    if {$name == ""} {
        set pnode [$node parentNode]
        if {$pnode == ""} {
            return ""
        }
        set type [$node nodeName]
        set siblings [childnodes $pnode $type]
        set index [lsearch $siblings $node]
        if {$index == 0} {
            set name $type
        } else {
            set name "$type$index"
        }
    }
    return $name
}

# ----------------------------------------------------------------------
# USAGE: node2comp <node>
#
# Used internally to create a path component name for the specified
# node.  A path component name has the form "type(id)" or just
# "type##" if the node doesn't have a name.  This name can be used
# in a path to uniquely address the component.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::node2comp {node} {
    set type [$node nodeName]
    if {[catch {$node getAttribute id} name]} { set name "" }
    if {$name == ""} {
        set pnode [$node parentNode]
        if {$pnode == ""} {
            return ""
        }
        set siblings [childnodes $pnode $type]
        set index [lsearch $siblings $node]
        if {$index == 0} {
            set name $type
        } else {
            set name "$type$index"
        }
    } else {
        set name "${type}($name)"
    }
    return $name
}

# ----------------------------------------------------------------------
# USAGE: node2path <node>
#
# Used internally to create a full path name for the specified node.
# The path is relative to the current object, so it stops when the
# parent is the root node for this object.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::node2path {node} {
    set path [node2comp $node]
    set node [$node parentNode]
    while {$node != "" && $node != $_node} {
        set path "[node2comp $node].$path"
        set node [$node parentNode]
    }
    return $path
}

# ----------------------------------------------------------------------
# USAGE: childnodes <node> <type>
#
# Used internally to return a list of children for the given <node>
# that match a specified <type>.  Similar to XML getElementsByTagName,
# but returns only direct children of the <node>.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::childnodes {node type} {
    set rlist ""
    foreach cnode [$node childNodes] {
        if {[$cnode nodeName] == $type} {
            lappend rlist $cnode
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: uq_get_vars [$tfile]
#
# Scans number nodes in the input section for UQ parameters.
#
# Returns a string in JSON so it can easily be passed to PUQ. Strips units
# because PUQ does not need or recognize them.
#
# For example, 2 parameters, one gaussian and one uniform might return:
# [["height","m",["gaussian",100,10,{"min":0}]],["velocity","m/s",["uniform",80,90]]]
#
# Returns "" if there are no UQ parameters.
#
# If tfile is specified, write a template file out.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::uq_get_vars {{tfile ""}} {
    set varout \[
    set varlist []

    if {$tfile == ""} {
        set node $_node
    } else {
        set fid [::open $tfile r]
        set doc [dom parse [read $fid]]
        set node [$doc documentElement]
        close $fid
    }

    set count 0
    set n [$node selectNodes /run/input//number]
    foreach _n $n {
        set x [$_n selectNodes current/text()]
        set val [$x nodeValue]
        if {[string equal -length 8 $val "uniform "] ||
            [string equal -length 9 $val "gaussian "]} {

            set vname [$_n getAttribute id]
            if {[lsearch $varlist $vname] >= 0} {
                continue
            } else {
                lappend varlist $vname
            }

            if {$count > 0} {
                append varout ,
            }
            incr count

            set units ""
            set unode [$_n selectNodes units/text()]
            if {"" != $unode} {
                set units [$unode nodeValue]
                set val [Rappture::Units::mconvert $val \
                -context $units -to $units -units off]
            }

            set v \[\"$vname\",\"$units\",
            set fmt "\[\"%s\",%.16g,%.16g"
            set val [format $fmt [lindex $val 0] [lindex $val 1] [lindex $val 2]]
            append v $val

            if {[string equal -length 9 [$x nodeValue] "gaussian "]} {
                set minv ""
                set min_node [$_n selectNodes min/text()]
                if {"" != $min_node} {
                    set minv [$min_node nodeValue]
                    if {$units != ""} {
                        set minv [Rappture::Units::convert $minv -context $units -units off]
                    }
                }

                set maxv ""
                set max_node [$_n selectNodes max/text()]
                if {"" != $max_node} {
                    set maxv [$max_node nodeValue]
                    if {$units != ""} {
                        set maxv [Rappture::Units::convert $maxv -context $units -units off]
                    }
                }

                if {$minv != "" || $maxv != ""} {
                    append v ",{"
                    if {$minv != ""} {
                        append v "\"min\":$minv"
                        if {$maxv != ""} {append v ,}
                    }
                    if {$maxv != ""} {
                        append v "\"max\":$maxv"
                    }
                    append v "}"
                }
            }

            if {$tfile != ""} {
                $x nodeValue @@[$_n getAttribute id]
            }
            append varout $v\]\]
        }
    }
    append varout \]

    if {$tfile != ""} {
        set fid [open $tfile w]
        puts $fid "<?xml version=\"1.0\"?>"
        puts $fid [$node asXML]
        close $fid
        $doc delete
    }
    if {$varout == "\[\]"} {set varout ""}
    #puts "uq_get_vars returning $varout"
    return [list $varout $count]
}
