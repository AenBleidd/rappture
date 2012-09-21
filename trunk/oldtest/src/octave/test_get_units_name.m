% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
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
    printf("\n\nTESTING rpUnitsGetUnitsName\n");
    [unitHandle,err] = rpUnitsFind(unitName);
    if !err && unitHandle
        [retStr,err] = rpUnitsGetUnitsName(unitHandle);
        if !err
            if retStr != ''
                printf ("units name of %s = %s\n", unitName,retStr);
            else
                printf ("rpUnitsGetUnitsName FAILED\n");
            end
        else
            printf("Error within rpUnitsGetUnitsName function\n");
        end
    else
        printf ("rpUnitsFind FAILED while testing rpUnitsGetUnitsName\n");
    end
end
