% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_result(lib)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_result(lib)
    err = 1;
    disp(sprintf('\n\nTESTING rpLibResult\n'));
    [err] = rpLibResult(lib);
    if ~err
        disp(sprintf ('rpLibResult Successful\n'));
    else
        disp(sprintf('Error within rpLibResult\n'));
    end
return
