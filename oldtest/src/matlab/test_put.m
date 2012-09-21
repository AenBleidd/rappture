% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
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

function [err] = test_put(libHandle,path,value,append)
    err = 1;
    disp(sprintf('\n\nTESTING rpLibPut\n'));
    [err] = rpLibPut(libHandle,path,value,append);
    if ~err
        [retStr, err] = rpLibGet(libHandle,path);
        disp(sprintf ('retStr = %s\n',retStr));
        if ~err
            if strcmp(retStr,value)
                disp(sprintf ('rpLibPut Successful\n'));
            else
                disp(sprintf ('rpLibPut FAILED\n'));
            end
        else
            disp(sprintf ('rpLibGet returned Error\n'));
        end
    else
        disp(sprintf('Error within rpLibPut\n'));
    end
return
