# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: tuples - represents a series of tuples for arbitrary data
#
#  This object represents a series of tuples.  Each tuple can contain
#  one or more elements--for example, (a) or (a,b,c).  Each column
#  in the tuple has a well-defined name and metadata.  Columns can
#  be added even after data has been stored in the tuple list.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

itcl::class Rappture::Tuples {
    constructor {args} { # defined below }

    public method column {option args}
    public method insert {pos args}
    public method delete {{from ""} {to ""}}
    public method put {args}
    public method get {args}
    public method find {args}
    public method size {}

    protected method _range {{from ""} {to ""}}

    private variable _colnames ""  ;# list of column names
    private variable _col2info     ;# maps column name => column options
    private variable _tuples       ;# maps index => tuple data

    private common _counter 0      ;# for auto-generated column names
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Tuples::constructor {args} {
    eval configure $args
}

# ----------------------------------------------------------------------
# USAGE: column insert <pos> ?-name n? ?-label l? ?-default v?
# USAGE: column delete ?<name> <name>...?
# USAGE: column names ?<pattern>?
# USAGE: column info <name>
#
# Used by clients to manipulate the columns associated with this
# list of tuples.  Each column is identified by a short name.  If
# a name is not supplied when the column is created, then one is
# generated automatically.  The column names can be queried back
# in the order they appear in a tuple by using the "column names"
# command.
# ----------------------------------------------------------------------
itcl::body Rappture::Tuples::column {option args} {
    switch -- $option {
        insert {
            # parse the incoming args
            if {[llength $args] < 1} {
                error "wrong # args: should be \"column insert pos ?-name n? ?-label l? ?-default v?\""
            }
            set pos [lindex $args 0]
            set args [lrange $args 1 end]
            Rappture::getopts args params {
                value -name #auto
                value -label ""
                value -default ""
            }

            # FIXME: This is a band-aid.  The value can be an arbitrary
            # string and therefore misinterpretered as an invalid list.
            # Try to parse the value as a list and if that fails make a
            # list out of it.  Hopefully this doesn't break run file
            # comparisons.
            if { [catch {llength $params(-default)}] != 0 } {
                set params(-default) [list $params(-default)]
            }

            if {[llength $args] != 0} {
                error "wrong # args: should be \"column insert pos ?-name n? ?-label l? ?-default v?\""
            }

            # insert the new column
            set cname $params(-name)
            if {$params(-name) == "#auto"} {
                set cname "column[incr _counter]"
            }
            if {[lsearch -exact $_colnames $cname] >= 0} {
                error "column name \"$cname\" already exists"
            }
            set _colnames [linsert $_colnames $pos $cname]
            set _col2info($cname-label) $params(-label)
            set _col2info($cname-default) $params(-default)

            # run through all existing tuples and insert the default val
            set max [array size _tuples]
            for {set i 0} {$i < $max} {incr i} {
                set oldval $_tuples($i)
                set _tuples($i) [linsert $oldval $pos $params(-default)]
            }
        }
        delete {
            foreach cname $args {
                set pos [lsearch -exact $_colnames $cname]
                if {$pos < 0} {
                    error "bad column name \"$cname\""
                }
                set _colnames [lreplace $_colnames $pos $pos]
                unset _col2info($cname-label)
                unset _col2info($cname-default)

                # run through all existing tuples and delete the column
                set max [array size _tuples]
                for {set i 0} {$i < $max} {incr i} {
                    set oldval $_tuples($i)
                    set _tuples($i) [lreplace $oldval $pos $pos]
                }
            }
        }
        names {
            if {[llength $args] == 0} {
                return $_colnames
            } elseif {[llength $args] == 1} {
                set pattern [lindex $args 0]
                set rlist ""
                foreach cname $_colnames {
                    if {[string match $pattern $cname]} {
                        lappend rlist $cname
                    }
                }
                return $rlist
            } else {
                error "wrong # args: should be \"column names ?pattern?\""
            }
        }
        info {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"column info name\""
            }
            set cname [lindex $args 0]
            set pos [lsearch -exact $_colnames $cname]
            if {$pos < 0} {
                error "bad column name \"$cname\""
            }
            return [list -label $_col2info($cname-label) -default $_col2info($cname-default)]
        }
        default {
            error "bad option \"$option\": should be delete, info, insert, names"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> ?<tuple> <tuple> ...?
#
# Used by clients to insert one or more tuples into this list at
# the given position <pos>.  Each <tuple> is a Tcl list of values
# in order corresponding to the column names.
# ----------------------------------------------------------------------
itcl::body Rappture::Tuples::insert {pos args} {
    set cols [llength $_colnames]
    set max [array size _tuples]

    if {"end" == $pos} {
        set pos $max
    } elseif {![string is integer $pos]} {
        error "bad position \"$pos\": should be integer or \"end\""
    } elseif {$pos < 0} {
        set pos 0
    } elseif {$pos > $max} {
        set pos $max
    }

    # make some room to insert these tuples
    set need [llength $args]
    for {set i [expr {$max-1}]} {$i >= $pos} {incr i -1} {
        set _tuples([expr {$i+$need}]) $_tuples($i)
    }

    # add the tuples at the specified pos
    foreach t $args {
        # make sure each tuple has enough columns
        while {[llength $t] < $cols} {
            lappend t ""
        }
        set _tuples($pos) $t
        incr pos
    }
}

# ----------------------------------------------------------------------
# USAGE: delete ?<from>? ?<to>?
#
# Used by clients to delete one or more tuples in this list.  With
# no args, it deletes all tuples.  With a single <from> arg, it deletes
# the tuple at that number.  With both args, it deletes tuples in the
# specified range.
# ----------------------------------------------------------------------
itcl::body Rappture::Tuples::delete {{from ""} {to ""}} {
    if {"" == $from && "" == $to} {
        catch {unset _tuples}
        return
    }
    if {[array size _tuples] == 0} {
        return  ;# nothing to delete
    }

    set last [expr {[array size _tuples]-1}]
    foreach {from to} [_range $from $to] break

    # delete all tuples in the specified range
    set gap [expr {$to-$from+1}]
    for {set i $from} {$i <= $last-$gap} {incr i} {
        set _tuples($i) $_tuples([expr {$i+$gap}])
    }
    for {} {$i <= $last} {incr i} {
        unset _tuples($i)
    }
}

# ----------------------------------------------------------------------
# USAGE: put ?-format <columns>? <pos> <tuple>
#
# Used by clients to store a different tuple value at the specified
# position <pos> in this list.  Normally, it stores the entire
# <tuple> at the specified slot, which must already exist.  (Use
# insert to create new slots.)  If the -format option is specified,
# then it interprets <tuple> according to the names in the -format,
# and updates only specific columns at that slot.
# ----------------------------------------------------------------------
itcl::body Rappture::Tuples::put {args} {
    Rappture::getopts args params {
        value -format ""
    }
    if {[llength $args] != 2} {
        error "wrong # args: should be \"put ?-format cols? pos tuple\""
    }
    foreach {pos tuple} $args break
    foreach {pos dummy} [_range $pos ""] break  ;# fix index

    if {![info exists _tuples($pos)]} {
        error "index $pos doesn't exist"
    }

    if {[string length $params(-format)] == 0} {
        # no format -- add tuple as-is (with proper number of columns)
        set cols [llength $_colnames]
        while {[llength $tuple] < $cols} {
            lappend tuple ""
        }
        set _tuples($pos) $tuple
    } else {
        # convert column names to indices
        set nlist ""
        foreach cname $params(-format) {
            set n [lsearch -exact $_colnames $cname]
            if {$n < 0} {
                error "bad column name \"$cname\""
            }
            lappend nlist $n
        }

        # convert data only for those indices
        set val $_tuples($pos)
        foreach n $nlist t $tuple {
            set val [lreplace $val $n $n $t]
        }
        set _tuples($pos) $val
    }
}

# ----------------------------------------------------------------------
# USAGE: get ?-format <columns>? ?<from>? ?<to>?
#
# Used by clients to query data from this list of tuples.  Returns
# a list of tuples in the specified range, or a list of all tuples
# if the range is not specified.
# ----------------------------------------------------------------------
itcl::body Rappture::Tuples::get {args} {
    Rappture::getopts args params {
        value -format ""
    }
    if {[llength $args] > 2} {
        error "wrong # args: should be \"get ?-format cols? ?from? ?to?\""
    }
    set from ""
    set to ""
    foreach {from to} $args break
    foreach {from to} [_range $from $to] break

    # empty? then return nothing
    if {[array size _tuples] == 0} {
        return ""
    }

    set rlist ""
    if {[string length $params(-format)] == 0} {
        # no format string -- return everything as-is
        for {set i $from} {$i <= $to} {incr i} {
            lappend rlist $_tuples($i)
        }
    } else {
        # convert column names to indices
        set nlist ""
        foreach cname $params(-format) {
            set n [lsearch -exact $_colnames $cname]
            if {$n < 0} {
                error "bad column name \"$cname\""
            }
            lappend nlist $n
        }
        set single [expr {[llength $nlist] == 1}]

        # convert data only for those indices
        for {set i $from} {$i <= $to} {incr i} {
            set t ""
            foreach n $nlist {
                if {$single} {
                    set t [lindex $_tuples($i) $n]
                } else {
                    lappend t [lindex $_tuples($i) $n]
                }
            }
            lappend rlist $t
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: find ?-format <columns>? ?<tuple>?
#
# Used by clients to search for all or part of a <tuple> on the
# list.  Without the extra -format option, this searches for an
# exact match of the <tuple> and returns a list of indices that
# match.  With the -format option, it checks the values for only
# the specified <columns>, and again returns a list of indices
# with matching values.
# ----------------------------------------------------------------------
itcl::body Rappture::Tuples::find {args} {
    Rappture::getopts args params {
        value -format ""
    }
    if {[llength $args] > 1} {
        error "wrong # args: should be \"find ?-format cols? ?tuple?\""
    }

    # convert column names to indices
    set nlist ""
    foreach cname $params(-format) {
        set n [lsearch -exact $_colnames $cname]
        if {$n < 0} {
            error "bad column name \"$cname\""
        }
        lappend nlist $n
    }

    # scan through all entries and find matching values
    set rlist ""
    set last [expr {[array size _tuples]-1}]
    if {[llength $args] == 0} {
        # no tuple? then all match
        for {set i 0} {$i <= $last} {incr i} {
            lappend rlist $i
        }
    } else {
        set tuple [lindex $args 0]
        if {[llength $nlist] == 0} {
            # no format? then look for an exact match
            for {set i 0} {$i <= $last} {incr i} {
                if {[string equal $tuple $_tuples($i)]} {
                    lappend rlist $i
                }
            }
        } else {
            # match only the columns in the -format
            for {set i 0} {$i <= $last} {incr i} {
                set matching 1
                foreach n $nlist t $tuple {
                    set val [lindex $_tuples($i) $n]
                    if {![string equal $t $val]} {
                        set matching 0
                        break
                    }
                }
                if {$matching} {
                    lappend rlist $i
                }
            }
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: size
#
# Used by clients to determine the number of tuples stored on this
# list.  Returns the size of the list.
# ----------------------------------------------------------------------
itcl::body Rappture::Tuples::size {} {
    return [array size _tuples]
}

# ----------------------------------------------------------------------
# USAGE: _range ?<from>? ?<to>?
#
# Used internally to convert a <from>/<to> range to a range of real
# number values.  If both are "", then the range is the entire range
# of data.  The <from> and <to> values can be integers or the keyword
# "end".
# ----------------------------------------------------------------------
itcl::body Rappture::Tuples::_range {{from ""} {to ""}} {
    set last [expr {[array size _tuples]-1}]
    if {"" == $from && "" == $to} {
        return [list 0 $last]
    }

    if {"end" == $from} {
        set from $last
    } elseif {![string is integer $from]} {
        error "bad position \"$from\": should be integer or \"end\""
    }
    if {$from < 0} {
        set from 0
    } elseif {$from > $last} {
        set from $last
    }

    if {"" == $to} {
        set to $from
    } elseif {"end" == $to} {
        set to $last
    } elseif {![string is integer $to]} {
        error "bad position \"$to\": should be integer or \"end\""
    }
    if {$to < 0} {
        set to 0
    } elseif {$to > $last} {
        set to $last
    }

    if {$from > $to} {
        # make sure to/from are in proper order
        set tmp $from
        set from $to
        set to $tmp
    }
    return [list $from $to]
}
