% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_node_comp(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_node_comp(lib,path)
    err = 1;
    disp(sprintf('\n\nTESTING rpLibNodeComp\n'));
    [nodeHandle,err] = rpLibElement(lib,path);
    if ~err
        if nodeHandle
            [compStr,err] = rpLibNodeComp(nodeHandle);
            if ~err
                disp(sprintf ('%s compStr = %s\n', path, compStr));
            else
                disp(sprintf('Error within rpLibNodeComp function\n'));
            end
        else
            disp(sprintf('rpLibElement returned invalid nodeHandle\n'));
        end
    else
        disp(sprintf('Error within rpLibElement function\n'));
    end
return
