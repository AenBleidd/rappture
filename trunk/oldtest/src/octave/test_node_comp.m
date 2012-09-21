% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
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
    printf("\n\nTESTING rpLibNodeComp\n");
    [nodeHandle,err] = rpLibElement(lib,path);
    if !err
        if nodeHandle
            [compStr,err] = rpLibNodeComp(nodeHandle);
            if !err
                printf ("%s compStr = %s\n", path, compStr);
            else
                printf("Error within rpLibNodeComp function\n");
            end
        else
            printf("rpLibElement returned invalid nodeHandle\n");
        end
    else
        printf("Error within rpLibElement function\n");
    end
end
