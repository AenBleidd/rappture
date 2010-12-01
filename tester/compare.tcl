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
# USAGE: compare_elements lib1 lib2 path
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
# USAGE: compare lib1 lib2 ?path?
#
# Compares two library objects and returns a list of paths that do not
# match.  Paths are relative to lib1 (i.e. if a path exists in lib2 but
# not lib1, it will not be included in the result.  Result will contain
# all differences that occur as descendants of an optional starting
# path.  If the path argument is not given, then only the output
# sections will be compared.
# ----------------------------------------------------------------------
proc Rappture::Tester::compare {lib1 lib2 {path output}} {
    set diffs [list]
    foreach child [$lib1 children $path] {
        foreach diff [compare $lib1 $lib2 $path.$child] {
            lappend diffs $diff
        }
    }
    if {[compare_elements $lib1 $lib2 $path]} {
        # Ignore output.time and output.user
        if {$path != "output.time" && $path != "output.user"} {
            lappend diffs $path
        }
    }
    return $diffs
}

# ----------------------------------------------------------------------
# USAGE: makeDriver tool.xml test.xml
#
# Builds and returns a driver library object to be used for running the 
# test specified by testxml.  Copy current values from test xml into the
# newly created driver.  If any inputs are present in the new tool.xml 
# which do not exist in the test xml, use the default value.
# ----------------------------------------------------------------------
proc Rappture::Tester::makeDriver {toolxml testxml} {
    # TODO: Test with various cases, especially with missing input elements
    # TODO: Any way to copy an object rather than creating a duplicate?
    # TODO: Sensible way of combining this proc with "merge" below?
    set toolobj [Rappture::library $toolxml]
    set golden [Rappture::library $testxml]
    set driver [Rappture::library $toolxml]
    return [Rappture::Tester::merge $toolobj $golden $driver]
}

# ----------------------------------------------------------------------
# USAGE: merge toolobj golden driver ?path?
#
# Used to recursively build up a driver library object for running a
# test.  Should not be called directly - see makeDriver.
# ----------------------------------------------------------------------
proc Rappture::Tester::merge {toolobj golden driver {path input}} {
    foreach child [$toolobj children $path] {
        set val [$golden get $path.$child.current]
        if {$val != ""} {
            $driver put $path.$child.current $val
        } else {
            set def [$toolobj get $path.$child.default]
            if {$def != ""} {
                $driver put $path.$child.current $def
            }
        }
        Rappture::Tester::merge $toolobj $golden $driver $path.$child
    }
    return $driver
}


