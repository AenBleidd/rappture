% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_put_dbl_id(libHandle,path,value,id,append)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2005  Purdue Research Foundation
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_put_dbl_id(libHandle,path,value,id,append)
    err = 1;
    printf("\n\nTESTING rpLibPutDoubleId\n");
    [err] = rpLibPutDoubleId(libHandle,path,value,id,append);
    if !err
        if id != ''
            pathID = [path,'(',id,')'];
        else
            pathID = path;
        end

        [retVal, err] = rpLibGetDouble(libHandle,pathID);
        printf ("retVal = %e\n",retVal);
        if !err
            if (retVal == value)
                printf ("rpLibPutDoubleId Successful\n");
            else
                printf ("rpLibPutDoubleId FAILED\n");
            end
        else
            printf ("rpLibGetDoubleId returned Error\n");
        end
    else
        printf("Error within rpLibPutDoubleId\n");
    end
end
