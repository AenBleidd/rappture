% ----------------------------------------------------------------------
%  EXAMPLE: Fermi-Dirac function in Octave.
%
%  This script represents a newly written application with rappture 
%  bindings and interface.
%
% ======================================================================
%  AUTHOR:  Michael McLennan, Purdue University
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

% get input file from the command line
% open our xml input file.
infile = sprintf("%s",argv(1,:));
lib = rpLib(infile);

% retrieve user specified data out of the input file 
% convert values to correct units.
Ef = rpLibGetString(lib,"input.number(Ef).current");
[Ef,err] = rpUnitsConvertDbl(Ef,"eV");
T = rpLibGetString(lib,"input.number(temperature).current");
[T,err] = rpUnitsConvertDbl(T,"K");

% do fermi calculations (science)...
kT = 8.61734e-5 * T;
Emin = Ef - 10*kT;
Emax = Ef + 10*kT;

E = linspace(Emin,Emax,200);
f = 1.0 ./ (1.0 + exp((E - Ef)/kT));

% prepare out output section
% label graphs
rpLibPutString(lib,"output.curve(f12).about.label","Fermi-Dirac Factor",0);
rpLibPutString(lib,"output.curve(f12).xaxis.label","Fermi-Dirac Factor",0);
rpLibPutString(lib,"output.curve(f12).yaxis.label","Energy",0);
rpLibPutString(lib,"output.curve(f12).yaxis.units","eV",0);

% this is a slow and inefficient method of putting
% large amounts of data back in to the rappture library object
%for j=1:200
%  rpUtilsProgress((j/200*100),'Iterating');
%  putStr = sprintf('%12g  %12g\n', f(j), E(j));
%  % put the data into the xml file
%  rpLibPutString(lib,"output.curve(f12).component.xy",putStr,1);
%end

% a better way is to take advantage of octave's vector operations.
outData = [f;E];
putStr = sprintf('%12g  %12g\n', outData);
rpLibPutString(lib,'output.curve(f12).component.xy',putStr,0);

% signal the end of processing
rpLibResult(lib);
