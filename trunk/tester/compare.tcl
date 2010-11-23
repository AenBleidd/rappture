package require Rappture

namespace eval Rappture::Regression { #forward declaration }

# For now, just check if equal
proc Rappture::Regression::compare_elements {lib1 lib2 path} {
    set val1 [$lib1 get $path]
    set val2 [$lib2 get $path]
    return [expr {$val1} != {$val2}]
}

# return a list of paths that differ
proc Rappture::Regression::compare {lib1 lib2 {path output}} {
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

