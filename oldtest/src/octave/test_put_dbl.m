% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_put_dbl(libHandle,path,value,append)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_put_dbl(libHandle,path,value,append)
    err = 1;
    printf("\n\nTESTING rpLibPutDouble\n");
    [err] = rpLibPutDouble(libHandle,path,value,append);
    if !err
        [retVal, err] = rpLibGetDouble(libHandle,path);
        printf ("retVal = %e\n",retVal);
        if !err
            if (retVal == value)
                printf ("rpLibPutDouble Successful\n");
            else
                printf ("rpLibPutDouble FAILED\n");
            end
        else
            printf ("rpLibGetDouble returned Error\n");
        end
    else
        printf("Error within rpLibPutDouble\n");
    end
end
