# ----------------------------------------------------------------------
#  EXAMPLE: Fermi-Dirac function in Perl.
#         
#  This simple example shows how to use Rappture within a simulator
#  written in Perl.
# ======================================================================
#  AUTHOR:  Nicholas J. Kisseberth, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

use Rappture;

# open the XML file containing the run parameters

$driver = Rappture::RpLibrary->new($ARGV[0]);

$Tstr = $driver->get("input.(temperature).current");
$T = Rappture::RpUnits::convert($Tstr, "K", "off");

$Efstr = $driver->get("input.(Ef).current");
$Ef = Rappture::RpUnits::convert($Efstr, "eV", "off");

open FILE, ">indeck" or die $!;
print FILE "$Ef\n";
print FILE "$T\n";
close FILE;

# $result = `octave --silent fermi.m < indeck 2>&1`;
$result = `octave --silent fermi.m < indeck`;

$driver->put("output.log",$result,0);

if (length($result) > 0) {
    open FILE, "<out.dat" or die $!;
    my @lines = <FILE>;
    close FILE;


    # Label the output graph with a title, x-axis label,
    # y-axis label, y-axis units.

    $driver->put("output.curve(f12).about.label","Fermi-Dirac Factor",0);
    $driver->put("output.curve(f12).xaxis.label","Fermi-Dirac Factor",0);
    $driver->put("output.curve(f12).yaxis.label","Energy",0);
    $driver->put("output.curve(f12).yaxis.units","eV",0);

    # skip over the first 4 header lines
    split(@lines,0,4);
    # search each line for 2 floating point numbers
    foreach $line (@lines) {
        if ($line =~ /(?<f>[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)\s+(?<E>[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)/) {
            $driver->put("output.curve(f12).component.xy", "$+{f} $+{E}\n", 1);
        }
    }

}

unlink("indeck");
unlink("out.dat");

$driver->result();
