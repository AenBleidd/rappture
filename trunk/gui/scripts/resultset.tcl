# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: ResultSet - set of XML objects for simulated results
#
#  This data structure collects all of the simulated results
#  produced by a series of tool runs.  It is used by the Analyzer,
#  ResultSelector, and other widgets to keep track of all known runs
#  and visualize the result that is currently selected.  Each run
#  has an index number ("#1", "#2", "#3", etc.) that can be used to
#  label the run and refer to it later.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

itcl::class Rappture::ResultSet {
    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {xmlobj}
    public method clear {{xmlobj ""}}
    public method diff {option args}
    public method find {collist vallist}
    public method get {collist xmlobj}
    public method contains {xmlobj}
    public method size {}

    public method notify {option args}
    protected method _notifyHandler {args}

    protected method _addOneResult {tuples xmlobj {simnum ""}}

    private variable _dispatcher ""  ;# dispatchers for !events
    private variable _results ""     ;# tuple of known results
    private variable _resultnum 0    ;# counter for result #1, #2, etc.
    private variable _notify         ;# info used for notify command
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::constructor {args} {
    # create a dispatcher for events
    Rappture::dispatcher _dispatcher
    $_dispatcher register !change
    $_dispatcher dispatch $this !change \
        [itcl::code $this _notifyHandler]

    # create a list of tuples for data
    set _results [Rappture::Tuples ::#auto]
    $_results column insert end -name xmlobj -label "top-level XML object"
    $_results column insert end -name simnum -label "simulation number"

    # clear notification info
    set _notify(ALL) ""

    eval configure $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::destructor {} {
    clear
    itcl::delete object $_results
}

# ----------------------------------------------------------------------
# USAGE: add <xmlobj>
#
# Adds a new result to this result set.  Scans through all existing
# results to look for a difference compared to previous results.
# Returns the simulation number (#1, #2, #3, etc.) of this new result
# to the caller.  The various data objects for this result set should
# be added to their result viewers at the same index.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::add {xmlobj} {
    set xmlobj0 [$_results get -format xmlobj end]
    if {$xmlobj0 eq ""} {
        #
        # If this is the first result, then there are no diffs.
        # Add it right in.
        #
        set simnum "#[incr _resultnum]"
        $_results insert end [list $xmlobj $simnum]
    } else {
        #
        # For all later results, find the diffs and add any new columns
        # into the results tuple.  The latest result is the most recent.
        #
        set simnum [_addOneResult $_results $xmlobj]
    }

    # make sure we fix up associated controls
    $_dispatcher event -now !change op add what $xmlobj

    return $simnum
}

# ----------------------------------------------------------------------
# USAGE: clear ?<xmlobj>?
#
# Clears one or all results in this result set.  If no specific
# result object is specified, then all results are cleared.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::clear {{xmlobj ""}} {
    if {$xmlobj ne ""} {
        #
        # Delete just one result.  Look for the result among the
        # tuples and remove it.  Then, rebuild all of the tuples
        # by scanning back through them and building them back up.
        # This will rebuild the columns/controls as they should
        # be now, removing anything that is no longer necessary.
        #
        set irun [$_results find -format xmlobj $xmlobj]
        if {[llength $irun] == 1} {
            # grab a description of what we're about to delete
            set dlist [list simnum [$_results get -format simnum $irun]]
            foreach col [lrange [$_results column names] 2 end] {
                set raw [lindex [Rappture::LibraryObj::value $xmlobj $col] 0]
                lappend dlist $col $raw  ;# use "raw" (user-readable) label
            }

            # delete this from the tuples of all results
            itcl::delete object $xmlobj
            $_results delete $irun

            set new [Rappture::Tuples ::#auto]
            $new column insert end -name xmlobj -label "top-level XML object"
            $new column insert end -name simnum -label "simulation number"

            for {set n 0} {$n < [$_results size]} {incr n} {
                set rec [lindex [$_results get -format {xmlobj simnum} $n] 0]
                foreach {obj num} $rec break
                if {$n == 0} {
                    $new insert end [list $obj $num]
                } else {
                    _addOneResult $new $obj $num
                }
            }

            # plug in the new set of rebuilt tuples
            itcl::delete object $_results
            set _results $new

            # make sure we fix up associated controls at some point
            $_dispatcher event -now !change op clear what $dlist
        }
    } else {
        #
        # Delete all results.
        #
        for {set irun 0} {$irun < [$_results size]} {incr irun} {
            set xo [$_results get -format xmlobj $irun]
            itcl::delete object $xo
        }
        $_results delete 0 end

        # make sure we fix up associated controls at some point
        $_dispatcher event -now !change op clear what all
    }

    if {[$_results size] == 0} {
        # no results left?  then reset to a clean state
        eval $_results column delete [lrange [$_results column names] 2 end]
        set _resultnum 0
    }
}

# ----------------------------------------------------------------------
# USAGE: diff names
# USAGE: diff values <column> ?<which>?
#
# Returns information about the diffs in the current set of known
# results.  The "diff names" returns a list of column names for
# parameters that have diffs.  (These are the columns in the tuples.)
#
# The "diff values" returns the various values associated with a
# particular <column> name.  If the optional <which> is specified,
# then it is treated as an index into the list of values--0 for the
# first value, 1 for the second, etc.  Each value is returned as
# a list with two words.  The first is the the label associated with
# the value.  The second is the normalized (numeric) value, which can
# be sorted to get a particular ordering.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::diff {option args} {
    switch -- $option {
        names {
            return [$_results column names]
        }
        values {
            if {[llength $args] < 1} {
                error "wrong # args: should be \"diff values col ?which?\""
            }
            set col [lindex $args 0]

            set which "all"
            if {[llength $args] > 1} {
                set which [lindex $args 1]
            }

            set rlist ""
            # build an array of normalized values and their labels
            if {$col == "simnum"} {
                set nruns [$_results size]
                for {set n 0} {$n < $nruns} {incr n} {
                    set simnum [$_results get -format simnum $n]
                    lappend rlist $simnum $n
                }
            } else {
                set havenums 1
                foreach rec [$_results get -format [list xmlobj $col]] {
                    set xo [lindex $rec 0]
                    set v [lindex $rec 1]
                    foreach {raw norm} \
                        [Rappture::LibraryObj::value $xo $col] break

                    if {![info exists unique($v)]} {
                        # keep only unique label strings
                        set unique($v) $norm
                    }
                    if {$havenums && ![string is double $norm]} {
                        set havenums 0
                    }
                }

                if {!$havenums} {
                    # don't have normalized nums? then sort and create nums
                    set rlist ""
                    set n 0
                    foreach val [lsort -dictionary [array names unique]] {
                        lappend rlist $val [incr n]
                    }
                } else {
                    set rlist [array get unique]
                }
            }

            if {$which eq "all"} {
                return $rlist
            }

            # treat the "which" parameter as an XML object
            set irun [lindex [$_results find -format xmlobj $which] 0]
            if {$irun ne ""} {
                set val [lindex [$_results get -format $col $irun] 0]
                array set val2norm $rlist
                if {[info exists val2norm($val)]} {
                    return [list $val $val2norm($val)]
                }
            }
        }
        default {
            error "bad option \"$option\": should be names or values"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: find <columnList> <valueList>
#
# Searches through the results for a set of tuple values that match
# the <valueList> for the given <columnList>.  Returns a list of
# matching xml objects or "" if there is no match.  If the <valueList>
# is *, then it returns a list of all xml objects.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::find {collist vallist} {
    if {$vallist eq "*"} {
        return [$_results get -format xmlobj]
    }

    set rlist ""
    foreach irun [$_results find -format $collist -- $vallist] {
        lappend rlist [$_results get -format xmlobj $irun]
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: get <columnList> <xmlobj>
#
# Returns values for the specified <columnList> for the given <xmlobj>.
# This is a way of querying associated data for the given object.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::get {collist xmlobj} {
    set irun [lindex [$_results find -format xmlobj $xmlobj] 0]
    if {$irun ne ""} {
        return [lindex [$_results get -format $collist $irun] 0]
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: contains <xmlobj>
#
# Checks to see if the given <xmlobj> is already represented by
# some result in this result set.  This comes in handy when checking
# to see if an input case is already covered.
#
# Returns 1 if the result set already contains this result, and
# 0 otherwise.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::contains {xmlobj} {
    # no results? then this must be new
    if {[$_results size] == 0} {
        return 0
    }

    #
    # Compare this new object against the last XML object in the
    # results set.  If it has a difference, make sure that there
    # is a column to represent the quantity with the difference.
    #
    set xmlobj0 [$_results get -format xmlobj end]
    foreach {op vpath oldval newval} [$xmlobj0 diff $xmlobj] {
        if {[$xmlobj get $vpath.about.diffs] == "ignore"} {
            continue
        }
        if {$op == "+" || $op == "-"} {
            # ignore differences where parameters come and go
            # such differences make it hard to work controls
            continue
        }
        if {[$_results column names $vpath] == ""} {
            # no column for this quantity yet
            return 0
        }
    }

    #
    # If we got this far, then look through existing results for
    # matching tuples, then check each one for diffs.
    #
    set format ""
    set tuple ""
    foreach col [lrange [$_results column names] 2 end] {
        lappend format $col
        set raw [lindex [Rappture::LibraryObj::value $xmlobj $col] 0]
        lappend tuple $raw  ;# use the "raw" (user-readable) label
    }
    if {[llength $format] > 0} {
        set ilist [$_results find -format $format -- $tuple]
    } else {
        set ilist 0  ;# no diffs -- must match first entry
    }

    foreach i $ilist {
        set xmlobj0 [$_results get -format xmlobj $i]
        set diffs [$xmlobj0 diff $xmlobj]
        if {[llength $diffs] == 0} {
            # no diffs -- already contained here
            return 1
        }
    }

    # must be some differences
    return 0
}


# ----------------------------------------------------------------------
# USAGE: size
#
# Returns the number of results currently stored in the set.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::size {} {
    return [$_results size]
}

# ----------------------------------------------------------------------
# USAGE: notify add <client> ?!event !event ...? <command>
# USAGE: notify get ?<client>? ?!event?
# USAGE: notify remove <client> ?!event !event ...?
#
# Clients use this to add/remove requests for notifications about
# various events that signal changes to the data in each ResultSet.
#
# The "notify add" operation takes a <client> name (any unique string
# identifying the client), an optional list of events, and the <command>
# that should be called for the callback.
#
# The "notify get" command returns information about clients and their
# registered callbacks.  With no args, it returns a list of <client>
# names.  If the <client> is specified, it returns a list of !events.
# If the <client> and !event is specified, it returns the <command>.
#
# The "notify remove" command removes any callback associated with
# a given <client>.  If no particular !events are specified, then it
# removes callbacks for all events.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::notify {option args} {
    set allEvents {!change}
    switch -- $option {
        add {
            if {[llength $args] < 2} {
                error "wrong # args: should be \"notify add caller ?!event !event ...? command"
            }
            set caller [lindex $args 0]
            set command [lindex $args end]
            if {[llength $args] > 2} {
                set events [lrange $args 1 end-1]
            } else {
                set events $allEvents
            }

            foreach name $events {
                if {[lsearch -exact $allEvents $name] < 0} {
                    error "bad event \"$name\": should be [join $allEvents ,]"
                }
                if {[lsearch $_notify(ALL) $caller] < 0} {
                    lappend _notify(ALL) $caller
                }
                set _notify($caller-$name) $command
            }
        }
        get {
            switch -- [llength $args] {
                0 {
                    return $_notify(ALL)
                }
                1 {
                    set caller [lindex $args 0]
                    set rlist ""
                    foreach key [array names _notify $caller-*] {
                        lappend rlist [lindex [split $key -] end]
                    }
                    return $rlist
                }
                2 {
                    set caller [lindex $args 0]
                    set name [lindex $args 1]
                    if {[info exists _notify($caller-$name)]} {
                        return $_notify($caller-$name)
                    }
                    return ""
                }
                default {
                    error "wrong # args: should be \"notify get ?caller? ?!event?\""
                }
            }
        }
        remove {
            if {[llength $args] < 1} {
                error "wrong # args: should be \"notify remove caller ?!event !event ...?"
            }
            set caller [lindex $args 0]
            if {[llength $args] > 1} {
                set events [lrange $args 1 end]
            } else {
                set events $allEvents
            }

            foreach name $events {
                catch {unset _notify($caller-$name)}
            }
            if {[llength [array names _notify $caller-*]] == 0} {
                set i [lsearch $_notify(ALL) $caller]
                if {$i >= 0} {
                    set _notify(ALL) [lreplace $_notify(ALL) $i $i]
                }
            }
        }
        default {
            error "wrong # args: should be add, get, remove"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _notifyHandler ?<eventArgs>...?
#
# Called automatically whenever a !change event is triggered in this
# object.  Scans through the list of clients that want to receive this
# event and executes each of their callbacks.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_notifyHandler {args} {
    array set data $args
    set event $data(event)

    foreach caller $_notify(ALL) {
        if {[info exists _notify($caller-$event)]} {
            if {[catch {uplevel #0 $_notify($caller-$event) $args} result]} {
                # anything go wrong? then throw a background error
                bgerror "$result\n(while dispatching $event to $caller)"
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _addOneResult <tuples> <xmlobj> ?<simNum>?
#
# Used internally to add one new <xmlobj> to the given <tuples>
# object.  If the new xmlobj contains different input parameters
# that are not already columns in the tuple, then this routine
# creates the new columns.  If the optional <simNum> is specified,
# then it is added as the simulation number #1, #2, #3, etc.  If
# not, then the new object is automatically numbered.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultSet::_addOneResult {tuples xmlobj {simnum ""}} {
    #
    # Compare this new object against the last XML object in the
    # results set.  If it has a difference, make sure that there
    # is a column to represent the quantity with the difference.
    #
    set xmlobj0 [$tuples get -format xmlobj end]
    foreach {op vpath oldval newval} [$xmlobj0 diff $xmlobj] {
        if {[$xmlobj get $vpath.about.diffs] == "ignore"} {
            continue
        }
        if {$op == "+" || $op == "-"} {
            # ignore differences where parameters come and go
            # such differences make it hard to work controls
            continue
        }

        # make sure that these values really are different
        set oldval [lindex [Rappture::LibraryObj::value $xmlobj0 $vpath] 0]
        set newval [lindex [Rappture::LibraryObj::value $xmlobj $vpath] 0]

        if {$oldval != $newval && [$tuples column names $vpath] == ""} {
            # no column for this quantity yet
            $tuples column insert end -name $vpath -default $oldval
        }
    }

    # build a tuple for this new object
    set cols ""
    set tuple ""
    foreach col [lrange [$tuples column names] 2 end] {
        lappend cols $col
        set raw [lindex [Rappture::LibraryObj::value $xmlobj $col] 0]
        lappend tuple $raw  ;# use the "raw" (user-readable) label
    }

    # find a matching tuple? then replace it -- only need one
    if {[llength $cols] > 0} {
        set ilist [$tuples find -format $cols -- $tuple]
    } else {
        set ilist 0  ;# no diffs -- must match first entry
    }

    # add all remaining columns for this new entry
    set tuple [linsert $tuple 0 $xmlobj]
    set cols [linsert $cols 0 "xmlobj"]

    if {[llength $ilist] > 0} {
        if {[llength $ilist] > 1} {
            error "why so many matching results?"
        }

        # overwrite the first matching entry
        # start by freeing the old result
        set index [lindex $ilist 0]
        set xo [$tuples get -format xmlobj $index]
        itcl::delete object $xo

        # put this new result in its place
        $tuples put -format $cols $index $tuple
        set simnum [$tuples get -format simnum $index]
    } else {
        if {$simnum eq ""} {
            set simnum "#[incr _resultnum]"
        }
        set tuple [linsert $tuple 1 $simnum]
        $tuples insert end $tuple
    }
    return $simnum
}
