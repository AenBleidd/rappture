% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_put_str(libHandle,path,value,append)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_put_str(libHandle,path,value,append)
    err = 1;
    printf("\n\nTESTING rpLibPutString\n");
    [err] = rpLibPutString(libHandle,path,value,append);
    if !err
        [retStr, err] = rpLibGet(libHandle,path);
        printf ("retStr = %s\n",retStr);
        if !err
            if (retStr != '') && (retStr == value)
                printf ("rpLibPutString Successful\n");
            else
                printf ("rpLibPutString FAILED\n");
            end
        else
            printf ("rpLibGet returned Error\n");
        end
    else
        printf("Error within rpLibPutString\n");
    end
end
