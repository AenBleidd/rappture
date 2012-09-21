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
% look in the global interface for the columns to
% store data. retrieve them for later use.
T = Rp_InterfaceConnect('temperature','units=K');
Ef = Rp_InterfaceConnect('Ef','units=eV');

x1 = Rp_InterfaceConnect('Fermi-Dirac Factor');
y1 = Rp_InterfaceConnect('Energy');
x2 = Rp_InterfaceConnect('Fermi-Dirac Factor * 2');
y2 = Rp_InterfaceConnect('Energy * 2');

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

% store results in the results table
% add data to the table pointed to by the variable result.
% put the fArr data in the column named "Fermi-Dirac Factor"
% put the EArr data in the column named "Energy"

Rp_TableColumnStore(x1,fArr);
Rp_TableColumnStore(y1,EArr);
Rp_TableColumnStore(x2,fArr2);
Rp_TableColumnStore(y2,EArr2);

% close the global interface
% signal to the graphical user interface that science
% calculations are complete and to display the data
% as described in the views

Rp_InterfaceClose();

return 0;
