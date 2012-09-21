% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_get_exponent(unitName)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_get_exponent(unitName)
    err = 1;
    disp(sprintf('\n\nTESTING rpUnitsGetExponent\n'));
    [unitHandle,err] = rpUnitsFind(unitName);
    if ~err && unitHandle
        [retVal,err] = rpUnitsGetExponent(unitHandle);
        if ~err
            disp(sprintf ('Exponent of %s = %e\n', unitName,retVal));
        else
            disp(sprintf('Error within rpUnitsGetExponent function\n'));
        end
    else
        disp(sprintf ('rpUnitsGetExponent FAILED\n'));
    end
return
