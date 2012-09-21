% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_get(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_get(lib,path)
    err = 1;
    disp(sprintf('\n\nTESTING rpGet\n'));
    [retStr,err] = rpLibGet(lib,path);
    if ~err
        if ~strcmp(retStr,'')
            disp(sprintf ('rpGet Successful: %s\n',retStr));
        else
            disp(sprintf ('rpGet FAILED\n'));
        end
    else
        disp(sprintf('Error within rpGet function\n'));
    end
return
