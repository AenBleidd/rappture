% ----------------------------------------------------------------------
%  EXAMPLE: Fermi-Dirac function in Octave.
%
%  This script represents a newly written application with rappture 
%  bindings and interface.
%
% ======================================================================
%  AUTHOR:  Michael McLennan, Purdue University
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2007  Purdue Research Foundation
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

% get out input file from the command line
% invoke this script with the following command:
% matlab -nodisplay -r infile=\'driver1234.xml\',fermi
% the above command sets variable infile to the name 'driver1234.xml'

% infile = 'driver31619.xml'

% open our xml input file.
lib = rpLib(infile);

% retrieve user specified data out of the input file
% convert values to correct units.
Ef = rpLibGetString(lib,'input.number(Ef).current');
[Ef,err] = rpUnitsConvertDbl(Ef,'eV');
T = rpLibGetString(lib,'input.number(temperature).current');
[T,err] = rpUnitsConvertDbl(T,'K');

% do fermi calculations (science)...
kT = 8.61734e-5 * T;
Emin = Ef - 10*kT;
Emax = Ef + 10*kT;

E = linspace(Emin,Emax,200);
f = 1.0 ./ (1.0 + exp((E - Ef)/kT));

% prepare out output section
% label graphs
rpLibPutString(lib,'output.curve(f12).about.label','Fermi-Dirac Factor',0);
rpLibPutString(lib,'output.curve(f12).xaxis.label','Fermi-Dirac Factor',0);
rpLibPutString(lib,'output.curve(f12).yaxis.label','Energy',0);
rpLibPutString(lib,'output.curve(f12).yaxis.units','eV',0);

for j=1:200
  rpUtilsProgress((j/200*100),'Iterating');
  putStr = sprintf('%12g  %12g\n', f(j), E(j));
  % put the data into the xml file
  rpLibPutString(lib,'output.curve(f12).component.xy',putStr,1);
end

% signal the end of processing
rpLibResult(lib);

quit;
