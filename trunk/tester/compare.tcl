# ----------------------------------------------------------------------
#  COMPONENT: compare - comparison procedures for regression testing
# ======================================================================
#  AUTHOR:  Ben Rafferty, Purdue University
#  Copyright (c) 2010  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

namespace eval Rappture::Tester { #forward declaration }

# ----------------------------------------------------------------------
# USAGE: compare_elements <lib1> <lib2> <path>
#
# Compare data found in two library objects at the given path.  Returns
# 1 if match, 0 if no match.  For now, just check if ascii identical.
# Later, we can do something more sophisticated for different types of 
# elements.
# ----------------------------------------------------------------------
proc Rappture::Tester::compare_elements {lib1 lib2 path} {
    set val1 [$lib1 get $path]
    set val2 [$lib2 get $path]
    return [expr {$val1} != {$val2}]
}

# ----------------------------------------------------------------------
# USAGE: compare <lib1> <lib2> ?path?
#
# Compares two library objects and returns a list of paths that do not
# match.  Only paths which exist in both libraries are considered.
# Return value will contain all differences that occur as descendants of
# an optional starting path.  If the path argument is not given, then 
# only the output sections will be compared.
# ----------------------------------------------------------------------
proc Rappture::Tester::compare {lib1 lib2 {path output}} {
    set paths [list]
    set clist1 [$lib1 children $path]
    set clist2 [$lib2 children $path]
    foreach child $clist1 {
        # Ignore if not present in both libraries
        if {[lsearch -exact $clist2 $child] != -1} {
            foreach p [compare $lib1 $lib2 $path.$child] {
                lappend paths $p
            }
        }
    }
    if {[compare_elements $lib1 $lib2 $path]} {
        # Ignore output.time and output.user
        if {$path != "output.time" && $path != "output.user"} {
            lappend paths $path
        }
    }
    return $paths
}

# TODO: Should we report differences in input side as well?

proc Rappture::Tester::missing {lib1 lib2 {path output}} {
    set paths [list]
    foreach child [$lib1 children $path] {
        foreach p [missing $lib1 $lib2 $path.$child] {
            lappend paths $p
        }
    }
    if {[$lib1 get $path] != "" && [$lib2 get $path] == ""} {
        lappend paths $path
    }
    return $paths
}

proc Rappture::Tester::added {lib1 lib2 {path output}} {
    set paths [list]
    foreach child [$lib2 children $path] {
        foreach p [added $lib1 $lib2 $path.$child] {
            lappend paths $p
        }
    }
    if {[$lib1 get $path] == "" && [$lib2 get $path] != ""} {
        lappend paths $path
    }
    return $paths
}

