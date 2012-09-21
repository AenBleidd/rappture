% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_xml(lib)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_xml(lib)
    err = 1;
    disp(sprintf('\n\nTESTING rpLibXml\n'));
    [retStr,err] = rpLibXml(lib);
    if (~err)
        disp(sprintf ('rpLibXml Successful: \n%s\n',retStr));
    else
        disp(sprintf('Error within rpLibXml\n'));
    end
return
