% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_convert(fromVal, toUnitsName, showUnits)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_convert_str(fromVal, toUnitsName, showUnits)
    err = 1;
    printf("\n\nTESTING rpUnitsConvertStr: convert %s to %s, showUnits: %i\n", fromVal, toUnitsName, showUnits);

    [retStr,err] = rpUnitsConvertStr(fromVal, toUnitsName, showUnits);
    if !err
        printf("%s = %s\n",fromVal,retStr);
    else
        printf("Error within rpUnitsConvert function\n");
    end
end
