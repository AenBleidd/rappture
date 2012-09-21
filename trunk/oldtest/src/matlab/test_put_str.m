% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
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
    disp(sprintf('\n\nTESTING rpLibPutString\n'));
    [err] = rpLibPutString(libHandle,path,value,append);
    if ~err
        [retStr, err] = rpLibGet(libHandle,path);
        disp(sprintf ('retStr = %s\n',retStr));
        if ~err
            if strcmp(retStr,value)
                disp(sprintf ('rpLibPutString Successful\n'));
            else
                disp(sprintf ('rpLibPutString FAILED\n'));
            end
        else
            disp(sprintf ('rpLibGet returned Error\n'));
        end
    else
        disp(sprintf('Error within rpLibPutString\n'));
    end
return
