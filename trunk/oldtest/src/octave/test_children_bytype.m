% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_children_bytype(lib, path, type)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2005  Purdue Research Foundation
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_children_bytype(lib,path,type)
    err = 1;
    prevNodeHandle = 0;
    nodeHandle = -1;

    printf("\n\nTESTING rpLibChildrenByType\n");

    while nodeHandle != 0
        [nodeHandle,err] = rpLibChildrenByType(lib,path,prevNodeHandle,type);
        prevNodeHandle = nodeHandle;

        if !err
            if nodeHandle
                [compStr,err] = rpLibNodeComp(nodeHandle);
                printf ("%s compStr = %s\n", path, compStr);
            end
        else
            printf("Error within rpLibChildrenByType function\n");
        end
    end
end
