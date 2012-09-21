% ----------------------------------------------------------------------
%  EXAMPLE: Rappture <cloud> elements
% ======================================================================
%  AUTHOR:  Derrick S. Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

% open our xml input file.
lib = rpLib(infile);

% get the formula and make it ready for evaluation
%formula = rpLibGetString(lib,'input.string(formula).current');
%formula = 'x*y*z';

% get the number of points for the cloud
npts = rpLibGetDouble(lib,'input.integer(npts).current');

%
% Generate the 2D mesh and field values...
%
% z = 1;
append = 1;
for n=1:npts
    x = rand();
    y = rand();
    putStr = sprintf('%12g  %12g\n', x, y);
    rpLibPutString(lib,'output.cloud(m2d).points',putStr,append);

    putStr = sprintf('%12g\n', x*y);
    rpLibPutString(lib,'output.field(f2d).component.values',putStr,append);
end

%
% Generate the 3D mesh and field values...
%
for n=1:npts
    x = rand();
    y = rand();
    z = rand();
    putStr = sprintf('%12g  %12g %12g\n', x, y, z);
    rpLibPutString(lib,'output.cloud(m3d).points',putStr,append);

    putStr = sprintf('%12g\n', x*y*z);
    rpLibPutString(lib,'output.field(f3d).component.values',putStr,append);
end

%
% Save the updated XML describing the run...
%
rpLibResult(lib);
quit;
