% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
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
    disp(sprintf('\n\nTESTING rpLibPutDoubleId\n'));
    [err] = rpLibPutDoubleId(libHandle,path,value,id,append);
    if ~err
%        commented out because id field is depricated for this function
%        if ~strcmp(id,'')
%            pathID = [path,'(',id,')'];
%        else
%            pathID = path;
%        end

        pathID = path;

        [retVal, err] = rpLibGetDouble(libHandle,pathID);
        disp(sprintf ('retVal = %e\n',retVal));
        if ~err
            if (retVal == value)
                disp(sprintf ('rpLibPutDoubleId Successful\n'));
            else
                disp(sprintf ('rpLibPutDoubleId FAILED\n'));
            end
        else
            disp(sprintf ('rpLibGetDoubleId returned Error\n'));
        end
    else
        disp(sprintf('Error within rpLibPutDoubleId\n'));
    end
return
