% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_convert_obj_dbl(fromVal, toUnitsName)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_convert_dbl(fromVal, toUnitsName)
    err = 1;
    printf("\n\nTESTING rpUnitsConvertDbl: convert %s to %s\n", fromVal, toUnitsName);

    [retVal,err] = rpUnitsConvertDbl(fromVal, toUnitsName);
    if !err
        printf("%s = %e\n",fromVal,retVal);
    else
        printf("Error within rpUnitsConvertDbl function\n");
    end
end
