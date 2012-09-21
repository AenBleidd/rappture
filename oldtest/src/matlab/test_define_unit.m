% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_define_unit(unitSymbol, basisHandle)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_define_unit(unitSymbol, basisHandle)
    err = 1;
    disp(sprintf('\n\nTESTING rpUnitsDefineUnit\n'));
    [unitsHandle,err] = rpUnitsDefineUnit(unitSymbol, basisHandle);
    if ~err

        if unitsHandle
            disp(sprintf ('Successfully Created %s\n',unitSymbol));
        else
            disp(sprintf ('FAILED Creating %s\n',unitSymbol));
        end

    else
        disp(sprintf('Error within rpUnitsDefineUnits function\n'));
    end
return
