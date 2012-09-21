% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_get_double(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_get_double(lib,path)
    err = 1;
    printf("\n\nTESTING rpLibGetDouble\n");
    [retVal,err] = rpLibGetDouble(lib,path);
    if !err
        printf ("rpLibGetDouble Successful: %e\n",retVal);
    else
        printf("Error within rpLibGetDouble function\n");
    end
end
