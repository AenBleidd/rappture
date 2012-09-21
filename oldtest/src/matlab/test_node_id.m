% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_node_id(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_node_id(lib,path)
    err = 1;
    disp(sprintf('\n\nTESTING rpLibNodeId\n'));
    [nodeHandle,err] = rpLibElement(lib,path);
    if ~err
        if nodeHandle
            [idStr,err] = rpLibNodeId(nodeHandle);
            if ~err
                disp(sprintf ('%s idStr = %s\n', path, idStr));
            else
                disp(sprintf('Error within rpLibNodeId function\n'));
            end
        else
            disp(sprintf('rpLibElement returned invalid nodeHandle\n'));
        end
    else
        disp(sprintf('Error within rpLibElement function\n'));
    end
return
