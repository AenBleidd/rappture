% ----------------------------------------------------------------------
%  TEST: Octave's Rappture Library Test Functions.
%
%   [err] = test_element(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2012  HUBzero Foundation, LLC
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_element(lib,path)
    err = 1;
    printf("\n\nTESTING rpElement\n");
    [nodeHandle,err] = rpElement(lib,path);
    if nodeHandle && !err
        [compStr,err] = rpNodeComp(nodeHandle);
        printf ("%s compStr = %s\n", path, compStr);
%        printf ("err = %i nodeHandle = %i\n",err,nodeHandle);

        [idStr, err]  = rpNodeId(nodeHandle);
        printf ("%s idStr   = %s\n", path, idStr);
%        printf ("err = %i nodeHandle = %i\n",err,nodeHandle);

        [typeStr,err] = rpNodeType(nodeHandle);
        printf ("%s typeStr = %s\n", path, typeStr);
%        printf ("err = %i nodeHandle = %i\n",err,nodeHandle);
    else
        printf("Error within rpElement function\n");
    end
end
