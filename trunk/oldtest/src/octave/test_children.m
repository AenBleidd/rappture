% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_children(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_children(lib,path)
    err = 1;
    prevNodeHandle = 0;
    nodeHandle = -1;

    printf("\n\nTESTING rpLibChildren\n");

    while nodeHandle != 0
        [nodeHandle,err] = rpLibChildren(lib,path,prevNodeHandle);
        prevNodeHandle = nodeHandle;

        if !err
            if nodeHandle
                [compStr,err] = rpLibNodeComp(nodeHandle);
                printf ("%s compStr = %s\n", path, compStr);
            end
        else
            printf("Error within rpLibChildren function\n");
        end
    end
end
