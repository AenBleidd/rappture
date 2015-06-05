# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: datatable - extracts data from an XML description of a field
#
#  This object represents a datatable of data in an XML description of
#  simulator output.  A datatable is similar to a field, but a field is
#  a quantity versus position in device.  A datatable is any quantity
#  versus any other quantity.  This class simplifies the process of
#  extracting data vectors that represent the datatable.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture {
    # forward declaration
}

itcl::class Rappture::DataTable {
    constructor {xmlobj path} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method components {{pattern *}}
    public method values {}
    public method columns {}
    public method hints {{key ""}}

    protected method Build {}
    private variable _xmlobj ""  ;      # ref to lib obj with datatable data
    private variable _datatable "";     # lib obj representing this datatable
    private variable _columns ""
    private variable _hints      ;      # cache of hints stored in XML
    private variable _tree ""
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DataTable::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be LibraryObj"
    }
    set _xmlobj $xmlobj
    set _datatable [$xmlobj element -as object $path]

    # build up vectors for various components of the datatable
    Build
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DataTable::destructor {} {
    itcl::delete object $_datatable
    blt::tree destroy $_tree
}

# ----------------------------------------------------------------------
# USAGE: values
#
# Returns the values for the datatable as a BLT tree.
# ----------------------------------------------------------------------
itcl::body Rappture::DataTable::values {} {
    return $_tree
}

# ----------------------------------------------------------------------
# USAGE: columns
#
# Returns the columns of the datatable in order.
# ----------------------------------------------------------------------
itcl::body Rappture::DataTable::columns {} {
    return $_columns
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this datatable.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::DataTable::hints {{keyword ""}} {
    if {![info exists _hints]} {
        foreach {key path} {
            group   about.group
            label   about.label
            color   about.color
            style   about.style
        } {
            set str [$_datatable get $path]
            if {"" != $str} {
                set _hints($key) $str
            }
        }
    }
    set _hints(xmlobj) $_xmlobj
    if { $keyword != "" } {
        if { [info exists _hints($keyword)] } {
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
# datatable when the object is first constructed, or whenever the datatable
# data changes.  Discards any existing vectors and builds everything
# from scratch.
# ----------------------------------------------------------------------
itcl::body Rappture::DataTable::Build {} {
    # Discard any existing data
    if { $_tree != "" } {
        blt::tree destroy $_tree
        set _tree ""
    }
    set _columns {}

    # Sniff for column information: label, descriptions, and style
    foreach cname [$_datatable children -type "column"] {
        set label        [$_datatable get "$cname.label"]
        set description  [$_datatable get "$cname.description"]
        set style        [$_datatable get "$cname.style"]
        lappend _columns $label $description $style
    }
    set csvdata [$_datatable get csv]
    set _tree [blt::tree create]
    if {"" != $csvdata} {
        set csvlist [blt::csv -data $csvdata]
        foreach row $csvlist {
            set child [$_tree insert 0]
            set c 0
            foreach value $row {label description style} $_columns {
                if { $label == "" } {
                    set label "$c"
                }
                $_tree set $child $label $value
                incr c
            }
        }
    }
}
