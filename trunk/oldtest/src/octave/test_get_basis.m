% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
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
    printf("\n\nTESTING rpUnitsGetBasis\n");
    [unitHandle,err] = rpUnitsFind(unitName);
    if !err && unitHandle
        [basisHandle,err] = rpUnitsGetBasis(unitHandle);
        if !err
            if basisHandle > 0
                [retStr,err] = rpUnitsGetUnitsName(basisHandle);
                printf ("basisHandle name = %s\n", retStr);
            elseif basisHandle < 0
                printf ("%s is a basis unit\n", unitName);
            else
                printf ("rpUnitsGetBasis FAILED\n");
            end
        else
            printf("Error within rpUnitsGetBasis function\n");
        end
    else
        printf ("rpUnitsGetBasis FAILED\n");
    end
end
