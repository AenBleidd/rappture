# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: table - extracts data from an XML description of a table
#
#  This object represents one table in an XML description of a table.
#  It simplifies the process of extracting data representing columns
#  in the table.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Table {
    constructor {xmlobj path} { # defined below }
    destructor { # defined below }

    public method rows {}
    public method columns {args}
    public method values {args}
    public method limits {col}
    public method hints {{key ""}}

    private variable _xmlobj ""  ;# ref to lib obj with curve data
    private variable _table ""   ;# lib obj representing this table
    private variable _tuples ""  ;# list of tuples with table data
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Table::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _table [$xmlobj element -as object $path]

    #
    # Load data from the table and store in the tuples.
    #
    set _tuples [Rappture::Tuples ::#auto]
    foreach cname [$_table children -type column] {
        set label [$_table get $cname.label]
        $_tuples column insert end -name $cname -label $label
    }

    set cols [llength [$_tuples column names]]
    set nline 1
    foreach line [split [$_table get data] \n] {
        if {[llength $line] == 0} {
            continue
        }
        if {[llength $line] != $cols} {
            error "bad data at line $nline: expected $cols columns but got \"[string trim $line]\""
        }
        $_tuples insert end $line
        incr nline
    }
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Table::destructor {} {
    itcl::delete object $_tuples
    itcl::delete object $_table
    # don't destroy the _xmlobj! we don't own it!
}

# ----------------------------------------------------------------------
# USAGE: rows
#
# Returns the number of rows of information in this table.
# ----------------------------------------------------------------------
itcl::body Rappture::Table::rows {} {
    return [$_tuples size]
}

# ----------------------------------------------------------------------
# USAGE: columns ?-component|-label|-units? ?<pos>?
#
# Returns information about the columns associated with this table.
# ----------------------------------------------------------------------
itcl::body Rappture::Table::columns {args} {
    Rappture::getopts args params {
        flag switch -component
        flag switch -label default
        flag switch -units
    }
    if {[llength $args] == 0} {
        set cols [llength [$_tuples column names]]
        set plist ""
        for {set i 0} {$i < $cols} {incr i} {
            lappend plist $i
        }
    } elseif {[llength $args] == 1} {
        set p [lindex $args 0]
        if {[string is integer $p]} {
            lappend plist $p
        } else {
            set pos [lsearch -exact [$_tuples column names] $p]
            if {$pos < 0} {
                error "bad column \"$p\": should be column name or integer index"
            }
            lappend plist $pos
        }
    } else {
        error "wrong # args: should be \"columns ?-component|-label|-units? ?pos?\""
    }

    set rlist ""
    switch -- $params(switch) {
        -component {
            set names [$_tuples column names]
            foreach p $plist {
                lappend rlist [lindex $names $p]
            }
        }
        -label {
            set names [$_tuples column names]
            foreach p $plist {
                set name [lindex $names $p]
                catch {unset opts}
                array set opts [$_tuples column info $name]
                lappend rlist $opts(-label)
            }
        }
        -units {
            set names [$_tuples column names]
            foreach p $plist {
                set comp [lindex $names $p]
                lappend rlist [$_table get $comp.units]
            }
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: values ?-row <index>? ?-column <index>?
#
# Returns a single value or a list of values for data in this table.
# If a particular -row and -column is specified, then it returns
# a single value for that row/column.  If either the -row or the
# -column is specified, then it returns a list of values in that
# row or column.  With no args, it returns all values in the table.
# ----------------------------------------------------------------------
itcl::body Rappture::Table::values {args} {
    Rappture::getopts args params {
        value -row ""
        value -column ""
    }
    if {[llength $args] > 0} {
        error "wrong # args: should be \"values ?-row r? ?-column c?\""
    }
    if {"" == $params(-row) && "" == $params(-column)} {
        return [$_tuples get]
    } elseif {"" == $params(-column)} {
        return [lindex [$_tuples get $params(-row)] 0]
    }

    if {[string is integer $params(-column)]} {
        set col [lindex [$_tuples column names] $params(-column)]
    } else {
        set col $params(-column)
        if {"" == [$_tuples column names $col]} {
            error "bad column name \"$col\": should be [join [$_tuples column names] {, }]"
        }
    }

    if {"" == $params(-row)} {
        # return entire column
        return [$_tuples get -format $col]
    }
    # return a particular cell
    return [$_tuples get -format $col $params(-row)]
}

# ----------------------------------------------------------------------
# USAGE: limits <column>
#
# Returns the {min max} limits of the numerical values in the
# specified <column>, which can be either an integer index to
# a column or a column name.
# ----------------------------------------------------------------------
itcl::body Rappture::Table::limits {column} {
    set min ""
    set max ""
    foreach v [values -column $column] {
        if {"" == $min} {
            set min $v
            set max $v
        } else {
            if {$v < $min} { set min $v }
            if {$v > $max} { set max $v }
        }
    }
    return [list $min $max]
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this table.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Table::hints {{keyword ""}} {
    foreach {key path} {
        label   about.label
        color   about.color
        style   about.style
    } {
        set str [$_table get $path]
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
