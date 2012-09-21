% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_find(unitSymbol)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_find(unitSymbol)
    err = 1;
    printf("\n\nTESTING rpUnitsFind\n");
    [nodeHandle,err] = rpUnitsFind(unitSymbol);
    if !err
        if nodeHandle
            printf ("rpUnitsFind Successful\n");
        else
            printf ("rpUnitsFind FAILED\n");
        end
    else
        printf("Error within rpElement function\n");
    end
end
