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

# ----------------------------------------------------------------------
# USAGE: library ?-std? <file>
# USAGE: library isvalid <object>
#
# Used to open a <file> containing an XML description of tool
# parameters.  Loads the file and returns the name of the LibraryObj
# file that represents it.
#
# If the -std flag is included, then the file is treated as the
# name of a standard file, which is part of the Rappture installation.
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
        if {[catch {$obj isa ::Rappture::LibraryObj} valid] == 0 && $valid} {
            return 1
        }
        return 0
    }

    # handle the open operation...
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
itcl::class Rappture::LibraryObj {
    constructor {info} { # defined below }
    destructor { # defined below }

    public method element {args}
    public method children {args}
    public method get {{path ""}}
    public method put {args}
    public method remove {{path ""}}
    public method xml {}

    protected method find {path}
    protected method path2list {path}
    protected method node2name {node}
    protected method node2comp {node}

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
# USAGE: element ?-flavor <fval>? ?<path>?
#
# Clients use this to query a particular element within the entire
# data structure.  The path is a string of the form
# "structure.box(source).corner".  This example represents the tag
# <corner> within a tag <box id="source"> within a tag <structure>,
# which must be found at the top level within this document.
#
# By default, this method returns the component name "type(id)".
# This is changed by setting the -flavor argument to "id" (for name
# of the tail element), to "type" (for the type of the tail element),
# to "object" (for an object representing the DOM node referenced by
# the path.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::element {args} {
    array set params {
        -flavor component
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
        error "wrong # args: should be \"element ?-flavor fval? ?path?\""
    }
    set path [lindex $args 0]

    set node [find $path]
    if {$node == ""} {
        return ""
    }

    switch -- $params(-flavor) {
      object {
          return [::Rappture::LibraryObj ::#auto $node]
      }
      component {
          return [node2comp $node]
      }
      id {
          return [node2name $node]
      }
      type {
          return [$node nodeName]
      }
      default {
          error "bad flavor \"$params(-flavor)\": should be object, id, type, component"
      }
    }
}

# ----------------------------------------------------------------------
# USAGE: children ?-flavor <fval>? ?-type <name>? ?<path>?
#
# Clients use this to query the children of a particular element
# within the entire data structure.  This is just like the "element"
# method, but it returns the children of the element instead of the
# element itself.  If the optional -type argument is specified, then
# the return list is restricted to children of the specified type.
#
# By default, this method returns a list of component names "type(id)".
# This is changed by setting the -flavor argument to "id" (for tail
# names of all children), to "type" (for the types of all children),
# to "object" (for a list of objects representing the DOM nodes for
# all children).
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::children {args} {
    array set params {
        -flavor component
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
        error "wrong # args: should be \"children ?-flavor fval? ?-type name? ?path?\""
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
    switch -- $params(-flavor) {
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
      type {
          foreach n $nlist {
              lappend rlist [$n nodeName]
          }
      }
      default {
          error "bad flavor \"$params(-flavor)\": should be object, id, type, component"
      }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: get ?<path>?
#
# Clients use this to query the value of a node.  If the path is not
# specified, it returns the value associated with the root node.
# Otherwise, it returns the value for the element specified by the
# path.
# ----------------------------------------------------------------------
itcl::body Rappture::LibraryObj::get {{path ""}} {
    set node [find $path]
    if {$node == ""} {
        return ""
    }
    return [string trim [$node text]]
}

# ----------------------------------------------------------------------
# USAGE: put ?-append yes? ?-id num? ?<path>? <string>
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
        error "wrong # args: should be \"put ?-append bval? ?-id num? ?path? string\""
    }
    if {[llength $args] == 2} {
        set path [lindex $args 0]
        set str [lindex $args 1]
    } else {
        set path ""
        set str [lindex $args 0]
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
        error "not yet implemented"
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
        if {![regexp {^(([a-zA-Z_]+#?)([0-9]*))?(\(([^\)]+)\))?$} $part \
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
            set nlist [$node getElementsByTagName $type]
            set node [lindex $nlist $index]
        } else {
            #
            # If the name is like "type(id)", then look for elements
            # that match the type and see if one has the requested name.
            # if the name is like "(id)", then look for any elements
            # with the requested name.
            #
            if {$type != ""} {
                set nlist [$node getElementsByTagName $type]
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
        set siblings [$pnode getElementsByTagName $type]
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
        set siblings [$pnode getElementsByTagName $type]
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
