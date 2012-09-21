% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_get_units_name(unitName)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_get_units_name(unitName)
    err = 1;
    disp(sprintf('\n\nTESTING rpUnitsGetUnitsName\n'));
    [unitHandle,err] = rpUnitsFind(unitName);
    if ~err && unitHandle
        [retStr,err] = rpUnitsGetUnitsName(unitHandle);
        if ~err
            if ~strcmp(retStr,'')
                disp(sprintf ('units name of %s = %s\n', unitName,retStr));
            else
                disp(sprintf ('rpUnitsGetUnitsName FAILED\n'));
            end
        else
            disp(sprintf('Error within rpUnitsGetUnitsName function\n'));
        end
    else
        disp(sprintf ('rpUnitsFind FAILED while testing rpUnitsGetUnitsName\n'));
    end
return
