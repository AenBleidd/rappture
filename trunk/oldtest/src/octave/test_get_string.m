% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_get_string(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_get_string(lib,path)
    err = 1;
    printf("\n\nTESTING rpLibGetString\n");
    [retStr,err] = rpLibGetString(lib,path);
    if !err
        if retStr != ''
            printf ("rpLibGetString Successful: %s\n",retStr);
        else
            printf ("rpLibGetString FAILED\n");
        end
    else
        printf("Error within rpLibGetString function\n");
    end
end
