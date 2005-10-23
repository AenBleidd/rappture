# ----------------------------------------------------------------------
#  USAGE: tclsh install.tcl
#
#  Use this script to install the Rappture toolkit into an existing
#  Tcl installation.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

# run this script from directory containing it
cd [file dirname [info script]]

set package "Rappture"
set version "0.9"

proc find {dir} {
    set flist ""
    foreach name [glob [file join $dir *]] {
        lappend flist $name
        if {[file isdirectory $name]} {
            eval lappend flist [find $name]
        }
    }
    return $flist
}

proc mkindex {dir} {
    package require Itcl  ;# index Itcl files too!
    auto_mkindex $dir *.tcl
}

proc fixperms {target perms} {
    global tcl_platform
    if {$tcl_platform(platform) == "unix"} {
        file attributes $target -permissions $perms
    }
}


set dir [file dirname [info library]]
set targetdir [file join $dir $package$version]

if {![file exists $targetdir]} {
    puts "making directory $targetdir..."
    file mkdir $targetdir
}

set origdir [pwd]
foreach context {. ../gui} {
    cd $context

    foreach file [find .] {
        set target [file join $targetdir $file]
        if {[file isdirectory $file]} {
            puts "making directory $target..."
            catch {file mkdir $target}
            fixperms $target ugo+rx
        } else {
            puts "installing $target..."
            file copy -force $file $target
            fixperms $target ugo+r
        }
    }
}

cd ..
catch {file mkdir [file join $targetdir lib]}
foreach file [find ./lib] {
    set target [file join $targetdir $file]
    if {[file isdirectory $file]} {
        puts "making directory $target..."
        catch {file mkdir $target}
        fixperms $target ugo+rx
    } else {
        puts "installing $target..."
        file copy -force $file $target
        fixperms $target ugo+r
    }
}

cd $origdir

set fid [open [file join $targetdir pkgIndex.tcl] w]
puts $fid "# Tcl package index file"
puts $fid "package ifneeded $package $version \""
puts $fid "  \[list lappend auto_path \[file join \$dir scripts\]\]"
puts $fid "  namespace eval \[list Rappture \[list variable installdir \$dir\]\]"
puts $fid "  package provide $package $version"
puts $fid "\""
close $fid

mkindex [file join $targetdir scripts]

if {[string match wish* [file tail [info nameofexecutable]]]} {
    package require Tk
    wm withdraw .
    tk_messageBox -icon info -message "$package-$version INSTALLED"
} else {
    puts "== $package-$version INSTALLED"
}
exit 0
