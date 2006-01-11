% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
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
    disp(sprintf('\n\nTESTING rpLibPutStringId\n'));
    [err] = rpLibPutStringId(libHandle,path,value,id,append);
    if ~err
%        commented out because id is depricated for this function
%        if id != ''
%            pathID = [path,'(',id,')']
%        else
%            pathID = path
%        end

        pathID = path
        [retStr, err] = rpLibGet(libHandle,pathID);
        disp(sprintf ('retStr = %s\n',retStr));
        if ~err
            if strcmp(retStr,value)
                disp(sprintf ('rpLibPutStringId Successful\n'));
            else
                disp(sprintf ('rpLibPutStringId FAILED\n'));
            end
        else
            disp(sprintf ('rpLibGet returned Error\n'));
        end
    else
        disp(sprintf('Error within rpLibPutStringId\n'));
    end
return
