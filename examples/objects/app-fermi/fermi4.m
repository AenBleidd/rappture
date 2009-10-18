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
lib = Rappture_Library(infile);

nPts = 200;

if (Rappture_Library_error(lib) == 0)) {
    % cannot open file or out of memory
    fprintf(stderr, Rappture_Library_traceback(lib));
    exit(Rappture_Library_error(lib));
}

T = Rappture_connect(lib,'temperature');
Ef = Rappture_Library_value(lib,'Ef','units=eV');

if (Rappture_Library_error(lib) == 0) {
    % there were errors while retrieving input data values
    % dump the tracepack
    fprintf(stderr,Rappture_Library_traceback(lib));
}

% do fermi calculations (science)...
kT = 8.61734e-5 * Rappture_Number_value(T,'K');
Emin = Ef - 10*kT;
Emax = Ef + 10*kT;

EArr = linspace(Emin,Emax,nPts);
fArr = 1.0 ./ (1.0 + exp((E - Ef)/kT));


curveLabel = 'Fermi-Dirac Curve';
curveDesc = 'Plot of Fermi-Dirac Calculation';

% do it the easy way,
% create a plot to add to the library
% plot is registered with lib upon object creation
% p1->add(nPts,xArr,yArr,format,curveLabel,curveDesc);

p1 = Rappture_Plot(lib);
Rappture_Plot_add(p1,nPts,fArr,EArr,"",curveLabel,curveDesc);
Rappture_Plot_propstr(p1,"xlabel","Fermi-Dirac Factor");
Rappture_Plot_propstr(p1,"ylabel","Energy");
Rappture_Plot_propstr(p1,"yunits","eV");

Rappture_Library_result(lib);

quit;
