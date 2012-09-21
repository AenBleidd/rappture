% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_result(lib)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_result(lib)
    err = 1;
    printf("\n\nTESTING rpLibResult\n");
    [err] = rpLibResult(lib);
    if !err
        printf ("rpLibResult Successful\n");
    else
        printf("Error within rpLibResult\n");
    end
end
