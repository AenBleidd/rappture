% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
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
    printf("\n\nTESTING rpUnitsDefineUnit\n");
    [unitsHandle,err] = rpUnitsDefineUnit(unitSymbol, basisHandle);
    if !err

        if unitsHandle
            printf ("Successfully Created %s\n",unitSymbol);
        else
            printf ("FAILED Creating %s\n",unitSymbol);
        end

    else
        printf("Error within rpUnitsDefineUnits function\n");
    end
end
