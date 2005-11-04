% ----------------------------------------------------------------------
%  EXAMPLE: Fermi-Dirac function in Matlab.
%
%  This script represents a legacy application that will be left
%  intact, but wrapped with a Rappture interface.
%
% ======================================================================
%  AUTHOR:  Michael McLennan, Purdue University
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2005  Purdue Research Foundation
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

% get out input file from the command line
% open our xml input file.
infile = sprintf("%s",argv(1,:));
lib = rpLib(infile);

% retrieve user specified data out of the input file 
% convert values to correct units.
Ef = rpGetString(lib,"input.number(Ef).current");
[Ef,err] = rpConvertDbl(Ef,"eV");
T = rpGetString(lib,"input.number(temperature).current");
[T,err] = rpConvertDbl(T,"K");

% do fermi calculations (science)...
kT = 8.61734e-5 * T;
Emin = Ef - 10*kT;
Emax = Ef + 10*kT;

E = linspace(Emin,Emax,200);
f = 1.0 ./ (1.0 + exp((E - Ef)/kT));

% prepare out output section
% label graphs
rpPutString(lib,"output.curve(f12).about.label","Fermi-Dirac Factor",0);
rpPutString(lib,"output.curve(f12).xaxis.label","Fermi-Dirac Factor",0);
rpPutString(lib,"output.curve(f12).yaxis.label","Energy",0);
rpPutString(lib,"output.curve(f12).yaxis.units","eV",0);

for j=1:200
  putStr = sprintf('%12g  %12g\n', f(j), E(j));
  % put the data into the xml file
  rpPutString(lib,"output.curve(f12).component.xy",putStr,1);
end

% signal the end of processing
rpResult(lib);
