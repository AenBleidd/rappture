# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: sequence - represents a sequence of output results
#
#  This object represents a sequence of other output results.  Each
#  element in the sequence has an index and a value.  All values in
#  the sequence must have the same type, but they can all be curves,
#  images, or other results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Sequence {
    constructor {xmlobj path} { # defined below }
    destructor { # defined below }

    public method value {pos}
    public method label {pos}
    public method index {pos}
    public method size {}
    public method hints {{keyword ""}}

    private variable _xmlobj ""  ;# ref to lib obj with sequence data
    private variable _dataobjs   ;# maps index => data object
    private variable _labels     ;# maps index => labels
    private variable _indices    ;# list of sorted index values
    private variable _hints      ;# cache of hints stored in XML
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Sequence::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be LibraryObj"
    }
    set _xmlobj [$xmlobj element -as object $path]

    #
    # Extract data values from the element definitions.
    #
    foreach name [$_xmlobj children -type element] {
        set index [$xmlobj get $path.$name.index]
        if {"" == $index} {
            continue
        }

        # check for an element about.label stanza
        set elelabel [$xmlobj get $path.$name.about.label]

        set ctype ""
        set _dataobjs($index) ""
        set _labels($index) ""
        foreach cname [$_xmlobj children $name] {
            set type [$xmlobj element -as type $path.$name.$cname]
            switch -- $type {
                index {
                    # ignore this
                    continue
                }
                about {
                    # ignore this
                    continue
                }
                curve {
                    set obj [Rappture::Curve ::\#auto $xmlobj $path.$name.$cname]
                }
                drawing {
                    set obj [Rappture::Drawing ::\#auto $xmlobj $path.$name.$cname]
                }
                datatable {
                    set obj [Rappture::DataTable ::\#auto $xmlobj $path.$name.$cname]
                }
                histogram {
                    set obj [Rappture::Histogram ::\#auto $xmlobj $path.$name.$cname]
                }
                field {
                    set obj [Rappture::Field ::\#auto $xmlobj $path.$name.$cname]
                }
                image {
                    set obj [Rappture::Image ::\#auto $xmlobj $path.$name.$cname]
                }
                map {
                    set obj [Rappture::Map ::\#auto $xmlobj $path.$name.$cname]
                }
                mesh {
                    set obj [Rappture::Mesh ::\#auto $xmlobj $path.$name.$cname]
                }
                structure {
                    # extract unique result set prefix
                    scan $xmlobj "::libraryObj%d" rset

                    # object rooted at x.sequence(y).element(z).structure
                    set obj [$xmlobj element -as object $path.$name.$cname]

                    # scene id (sequence id)
                    set sceneid [$xmlobj element -as id $path]-$rset

                    # sequence/element/frame number starting at 1
                    set frameid [expr [$xmlobj element -as id $path.$name] + 1]

                    # only supporting one molecule per structure at the moment
                    # otherwise should go through all children that are molecules
                    # and insert scene/frame data.
                    $obj put "components.molecule.state" $frameid
                    $obj put "components.molecule.model" $sceneid
                }
                default {
                    error "don't know how to handle sequences of $type"
                }
            }
            if {"" == $ctype} {
                set ctype $type
            }
            if {$type == $ctype} {
                lappend _dataobjs($index) $obj
                set _labels($index) $elelabel
            } else {
                itcl::delete object $obj
            }
        }
    }
    #
    # Generate a list of sorted index values.
    #
    set units [$xmlobj get $path.index.units]
    if {"" != $units} {
        # build up a list:  {10m 10} {10cm 0.1} ...
        set vals ""
        foreach key [array names _dataobjs] {
            lappend vals [list $key [Rappture::Units::convert $key \
                -context $units -to $units -units off]]
        }

        # sort according to raw values; store both values
        set _indices [lsort -real -index 1 $vals]
    } else {
        # are the indices integers, reals, or strings?
        set how -integer
        foreach key [array names _dataobjs] {
            if {[regexp {^[0-9]+[eE][-+]?[0-9]+|([0-9]+)?\.[0-9]+([eE][-+]?[0-9]+)?$} $key]} {
                set how -real
                break
            } elseif {![regexp {^[0-9]+$} $key]} {
                set how -dictionary
                break
            }
        }

        # keep a list of indices sorted in order
        set _indices ""
        if {[string equal $how -dictionary]} {
            set n 0
            foreach val [lsort $how [array names _dataobjs]] {
                lappend _indices [list $val $n]
                incr n
            }
        } else {
            foreach val [lsort $how [array names _dataobjs]] {
                lappend _indices [list $val $val]
            }
        }
    }
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Sequence::destructor {} {
    foreach key [array names _dataobjs] {
        eval itcl::delete object $_dataobjs($key)
    }
    itcl::delete object $_xmlobj
}

# ----------------------------------------------------------------------
# USAGE: value <pos>
#
# Returns the value for the element as position <pos> in the
# list of all elements.  Here, <pos> runs from 0 to size-1.
# ----------------------------------------------------------------------
itcl::body Rappture::Sequence::value {pos} {
    set i [lindex [lindex $_indices $pos] 0]

    # FIXME:  This is a bandaid on what appears to be a timing problem.
    # This "dataobjs" method is now called before any sequence frames
    # have been added.
    if { ![info exists _dataobjs($i)] } {
        return ""
    }

    return $_dataobjs($i)
}

# ----------------------------------------------------------------------
# USAGE: label <pos>
#
# Returns the label for the element as position <pos> in the
# list of all elements.  Here, <pos> runs from 0 to size-1.
# ----------------------------------------------------------------------
itcl::body Rappture::Sequence::label {pos} {
    set i [lindex [lindex $_indices $pos] 0]

    # FIXME:  This is a bandaid on what appears to be a timing problem.
    # This "label" method is now called before any sequence frames
    # have been added.
    if { ![info exists _labels($i)] } {
        return ""
    }

    return $_labels($i)
}

# ----------------------------------------------------------------------
# USAGE: index <pos>
#
# Returns information about the index value for the element at
# position <pos> in the list of all elements.  The return value is
# a list of two elements:  {string rawNumberValue}.  Here, <pos>
# runs from 0 to size-1.
# ----------------------------------------------------------------------
itcl::body Rappture::Sequence::index {pos} {
    return [lindex $_indices $pos]
}

# ----------------------------------------------------------------------
# USAGE: size
#
# Returns the number of elements in this sequence.
# ----------------------------------------------------------------------
itcl::body Rappture::Sequence::size {} {
    return [llength $_indices]
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about showing
# this image.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Sequence::hints {{keyword ""}} {
    if {![info exists _hints]} {
        foreach {key path} {
            label        about.label
            indexlabel   index.label
            indexdesc    index.description
        } {
            set str [$_xmlobj get $path]
            if {"" != $str} {
                set _hints($key) $str
            }
        }
    }

    if {$keyword != ""} {
        if {[info exists _hints($keyword)]} {
            return $_hints($keyword)
        }
        return ""
    }
    return [array get _hints]
}
