% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_node_type(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_node_type(lib,path)
    err = 1;
    disp(sprintf('\n\nTESTING rpLibNodeType\n'));
    [nodeHandle,err] = rpLibElement(lib,path);
    if ~err
        if nodeHandle
            [typeStr,err] = rpLibNodeType(nodeHandle);
            if ~err
                disp(sprintf ('%s typeStr = %s\n', path, typeStr));
            else
                disp(sprintf('Error within rpLibNodeType function\n'));
            end
        else
            disp(sprintf('rpLibElement returned invalid nodeHandle\n'));
        end
    else
        disp(sprintf('Error within rpLibElement function\n'));
    end
return
