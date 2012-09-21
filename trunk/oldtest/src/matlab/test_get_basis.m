% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_get_basis(unitName)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_get_basis(unitName)
    err = 1;
    disp(sprintf('\n\nTESTING rpUnitsGetBasis\n'));
    [unitHandle,err] = rpUnitsFind(unitName);
    if ~err && unitHandle
        [basisHandle,err] = rpUnitsGetBasis(unitHandle);
        if ~err
            if basisHandle > 0
                [retStr,err] = rpUnitsGetUnitsName(basisHandle);
                disp(sprintf ('basisHandle name = %s\n', retStr));
            elseif basisHandle < 0
                disp(sprintf ('%s is a basis unit\n', unitName));
            else
                disp(sprintf ('rpUnitsGetBasis FAILED\n'));
            end
        else
            disp(sprintf('Error within rpUnitsGetBasis function\n'));
        end
    else
        disp(sprintf ('rpUnitsGetBasis FAILED\n'));
    end
return
