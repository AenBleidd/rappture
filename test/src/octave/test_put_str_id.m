% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_put_str_id(libHandle,path,value,id,append)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2005  Purdue Research Foundation
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_put_str_id(libHandle,path,value,id,append)
    err = 1;
    printf("\n\nTESTING rpLibPutStringId\n");
    [err] = rpLibPutStringId(libHandle,path,value,id,append);
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
                printf ("rpLibPutStringId Successful\n");
            else
                printf ("rpLibPutStringId FAILED\n");
            end
        else
            printf ("rpLibGet returned Error\n");
        end
    else
        printf("Error within rpLibPutStringId\n");
    end
end
