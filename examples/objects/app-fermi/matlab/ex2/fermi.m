% ----------------------------------------------------------------------
%  EXAMPLE: Fermi-Dirac function in Octave.
%
%  This script represents a newly written application with rappture 
%  bindings and interface.
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function fermi4(argv)

% declare variables to interact with Rappture
T       = 0.0;
Ef      = 0.0;
result  = 0;

% declare program variables
E       = 0.0;
dE      = 0.0;
kT      = 0.0;
Emin    = 0.0;
Emax    = 0.0;
f       = 0.0;
nPts    = 200;

% initialize the global interface
Rp_InterfaceInit(argv,@fermi_io);

% check that global interface for errors
if (Rp_InterfaceError() != 0)) {
    % there were errors while setting up the interface
    % dump the traceback
    o = Rp_InterfaceOutcome();
    fprintf(2, '%s', Rp_OutcomeContext(o));
    fprintf(2, '%s', Rp_OutcomeRemark(o));
    return(Rp_InterfaceError());
}

% connect variables to the interface
% look in the global interface for an object named
% 'temperature', convert its value to Kelvin, and
% store the value into the variable T.
% look in the global interface for an object named
% 'Ef', convert its value to electron Volts and store
% the value into the variable Ef.
% look in the global interface for an object named
% factorsTable and store a handle to it in the
% variable result.

T = Rp_InterfaceConnect('temperature','units=K');
Ef = Rp_InterfaceConnect('Ef','units=eV');
p1 = Rp_InterfaceConnect('fdfPlot');
p2 = Rp_InterfaceConnect('fdfPlot2');

% check the global interface for errors
if (Rp_InterfaceError() != 0) {
    % there were errors while retrieving input data values
    % dump the tracepack
    o = Rp_InterfaceOutcome();
    fprintf(stderr, '%s', Rp_OutcomeContext(o));
    fprintf(stderr, '%s', Rp_OutcomeRemark(o));
    return(Rp_InterfaceError());
}

% do fermi calculations (science)...
kT = 8.61734e-5 * T;
Emin = Ef - (10*kT);
Emax = Ef + (10*kT);

EArr = linspace(Emin,Emax,nPts);
fArr = 1.0 ./ (1.0 + exp((EArr - Ef)/kT));

% set up the curves for the plot by using the add command
% Rp_PlotAdd(plot,name,xdata,ydata,fmt);
%
% to group curves on the same plot, just keep adding curves
% to save space, X array values are compared between curves.
% the the X arrays contain the same values, we only store
% one version in the internal data table, otherwise a new
% column is created for the array. for big arrays this may take
% some time, we should benchmark to see if this can be done
% efficiently.

Rp_PlotAdd(p1,'fdfCurve1',fArr,EArr,'g:o');

Rp_PlotAdd(p2,'fdfCurve2',fArr,EArr,'b-o');
Rp_PlotAdd(p2,'fdfCurve3',fArr,EArr,'p--');

% close the global interface
% signal to the graphical user interface that science
% calculations are complete and to display the data
% as described in the views

Rp_InterfaceClose();

return 0;
