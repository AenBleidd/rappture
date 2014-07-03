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

$kT = 8.61734e-5 * $T;
$Emin = $Ef - 10 * $kT;
$Emax = $Ef + 10 * $kT;

$E = $Emin;
$dE = 0.005*($Emax - $Emin);

# Label the output graph with a title, x-axis label,
# y-axis label, y-axis units.

$driver->put("output.curve(f12).about.label","Fermi-Dirac Factor",0);
$driver->put("output.curve(f12).xaxis.label","Fermi-Dirac Factor",0);
$driver->put("output.curve(f12).yaxis.label","Energy",0);
$driver->put("output.curve(f12).yaxis.units","eV",0);

while( $E < $Emax ) {
    $f = 1.0 / ( 1.0 + exp(($E - $Ef) / $kT));
    Rappture::Utils::progress((($E-$Emin)/($Emax-$Emin)*100),"Iterating");
    $driver->put("output.curve(f12).component.xy", "$f $E\n", 1);
    $E = $E + $dE;
}

$driver->result();
