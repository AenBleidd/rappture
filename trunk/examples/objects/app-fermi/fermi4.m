% ----------------------------------------------------------------------
%  EXAMPLE: Fermi-Dirac function in Octave.
%
%  This script represents a newly written application with rappture 
%  bindings and interface.
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2005-2009  Purdue Research Foundation
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
lib = Rp_LibraryInit();
Rp_LibraryLoadFile(lib, infile);

nPts = 200;

if (Rp_LibraryError(lib) != 0)) {
    % cannot open file or out of memory
    o = Rp_LibraryOutcome(lib);
    fprintf(stderr, '%s', Rp_LibraryContext(o));
    fprintf(stderr, '%s', Rp_LibraryRemark(o));
    return;
}

T = Rp_Connect(lib,'temperature');
Ef = Rp_LibraryValue(lib,'Ef','units=eV');

if (Rp_LibraryError(lib) != 0) {
    % there were errors while retrieving input data values
    % dump the tracepack
    o = Rp_LibraryOutcome(lib);
    fprintf(stderr, '%s', Rp_LibraryContext(o));
    fprintf(stderr, '%s', Rp_LibraryRemark(o));
    return;
}

% do fermi calculations (science)...
kT = 8.61734e-5 * Rp_NumberValue(T,'K');
Emin = Ef - 10*kT;
Emax = Ef + 10*kT;

EArr = linspace(Emin,Emax,nPts);
fArr = 1.0 ./ (1.0 + exp((EArr - Ef)/kT));

% do it the easy way,
% create a plot to add to the library
% plot is registered with lib upon object creation
% p1->add(nPts,xArr,yArr,format,name);

p1 = Rp_PlotInit(lib);
Rp_PlotAdd(p1,nPts,fArr,EArr,'','fdfactor');
Rp_PlotPropstr(p1,'label','Fermi-Dirac Curve');
Rp_PlotPropstr(p1,'desc','Plot of Fermi-Dirac Calculation');
Rp_PlotPropstr(p1,'xlabel','Fermi-Dirac Factor');
Rp_PlotPropstr(p1,'ylabel','Energy');
Rp_PlotPropstr(p1,'yunits','eV');

Rp_LibraryResult(lib);

return;
