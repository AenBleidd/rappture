% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_put(libHandle,path,value,id,append)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_put(libHandle,path,value,id,append)
    err = 1;
    printf("\n\nTESTING rpLibPut\n");
    [err] = rpLibPut(libHandle,path,value,id,append);
    if !err
        if id != ''
            pathID = [path,'(',id,')']
        else
            pathID = path
        end
        [retStr, err] = rpLibGet(libHandle,pathID);
        printf ("retStr = %s\n",retStr);
        if !err
            if (retStr != '') && (retStr == value)
                printf ("rpLibPut Successful\n");
            else
                printf ("rpLibPut FAILED\n");
            end
        else
            printf ("rpLibGet returned Error\n");
        end
    else
        printf("Error within rpLibPut\n");
    end
end
