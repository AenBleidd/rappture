% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_make_metric(unitName)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_make_metric(unitName)
    err = 1;
    printf("\n\nTESTING rpUnitsMakeMetric\n");
    [unitHandle,err] = rpUnitsFind(unitName);
    if !err && unitHandle
        [err] = rpUnitsMakeMetric(unitHandle);
        if !err
            [unitHandle,err] = rpUnitsFind(['c',unitName]);
            if !err 
                [retStr,err] = rpUnitsGetUnitsName(unitHandle);
                if !err
                    printf ("centi-%s = %s\n", unitName,retStr);
                else
                    printf("Error in rpUnitsGetUnitsName testing rpUnitsMakeMetric\n");
                end
            else
                printf("Error in rpUnitsFind testing rpUnitsMakeMetric\n");
            end
        else
            printf("Error within rpUnitsMakeMetric function\n");
        end
    else
        printf ("rpUnitsFind FAILED while testing rpUnitsGetUnits\n");
    end
end
