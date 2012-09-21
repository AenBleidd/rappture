# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in Tcl.
#
#  This simple example shows how to use Rappture to wrap up a
#  simulator written in Matlab or some other language.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Rappture

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set Ef [$driver get input.(Ef).current]
set Ef [Rappture::Units::convert $Ef -to eV -units off]
set T [$driver get input.(temperature).current]
set T [Rappture::Units::convert $T -to K -units off]

set fid [open "indeck" w]
puts $fid $Ef
puts $fid $T
close $fid

set status [catch {Rappture::exec octave --silent fermi.m < indeck} out]
$driver put output.log $out

if {$status == 0} {
    set fid [open out.dat r]
    set info [read $fid]
    close $fid

    # Label output graph with title, x-axis label,
    # y-axis lable, and y-axis units
    $driver put -append no output.curve(f12).about.label "Fermi-Dirac Factor"
    $driver put -append no output.curve(f12).xaxis.label "Fermi-Dirac Factor"
    $driver put -append no output.curve(f12).yaxis.label "Energy"
    $driver put -append no output.curve(f12).yaxis.units "eV"

    # skip over the first 4 header lines
    foreach line [lrange [split $info \n] 5 end] {
        if {[scan $line {%g %g} f E] == 2} {
            $driver put -append yes output.curve(f12).component.xy "$f $E\n"
        }
    }
}

file delete -force indeck out.dat

# save the updated XML describing the run...
Rappture::result $driver $status
exit 0
