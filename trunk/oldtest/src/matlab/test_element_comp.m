% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_element_comp(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_element_comp(lib,path)
    err = 1;
    disp(sprintf('\n\nTESTING rpLibElementAsComp\n'));
    [compStr,err] = rpLibElementAsComp(lib,path);
    if (~strcmp(compStr,'') && ~err)
        disp(sprintf ('%s compStr = %s\n', path, compStr));
    else
        disp(sprintf('Error within rpLibElementAsComp function\n'));
    end
return
