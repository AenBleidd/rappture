% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_get(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2005  Purdue Research Foundation
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_get(lib,path)
    err = 1;
    printf("\n\nTESTING rpGet\n");
    [retStr,err] = rpLibGet(lib,path);
    if !err
        if retStr != ''
            printf ("rpGet Successful: %s\n",retStr);
        else
            printf ("rpGet FAILED\n");
        end
    else
        printf("Error within rpGet function\n");
    end
end
