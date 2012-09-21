% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
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
    disp(sprintf('\n\nTESTING rpLibPutDouble\n'));
    [err] = rpLibPutDouble(libHandle,path,value,append);
    if ~err
        [retVal, err] = rpLibGetDouble(libHandle,path);
        disp(sprintf ('retVal = %e\n',retVal));
        if ~err
            if (retVal == value)
                disp(sprintf ('rpLibPutDouble Successful\n'));
            else
                disp(sprintf ('rpLibPutDouble FAILED\n'));
            end
        else
            disp(sprintf ('rpLibGetDouble returned Error\n'));
        end
    else
        disp(sprintf('Error within rpLibPutDouble\n'));
    end
return
