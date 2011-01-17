% ----------------------------------------------------------------------
%  TEST: Matlab's Rappture Library Test Functions.
%
%   [err] = test_element(lib, path)
%
%
% ======================================================================
%  AUTHOR:  Derrick Kearney, Purdue University
%  Copyright (c) 2004-2005  Purdue Research Foundation
%
%  See the file "license.terms" for information on usage and
%  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
% ======================================================================

function [err] = test_element(lib,path)
    err = 1;
    disp(sprintf('\n\nTESTING rpLibElement\n'));
    [nodeHandle,err] = rpLibElement(lib,path);
    if (nodeHandle && ~err)
        [compStr,err] = rpLibNodeComp(nodeHandle);
        disp(sprintf ('%s compStr = %s\n', path, compStr));
%        printf ("err = %i nodeHandle = %i\n",err,nodeHandle);

        [idStr, err]  = rpLibNodeId(nodeHandle);
        disp(sprintf ('%s idStr   = %s\n', path, idStr));
%        printf ("err = %i nodeHandle = %i\n",err,nodeHandle);

        [typeStr,err] = rpLibNodeType(nodeHandle);
        disp(sprintf ('%s typeStr = %s\n', path, typeStr));
%        printf ("err = %i nodeHandle = %i\n",err,nodeHandle);
    else
        disp(sprintf('Error within rpLibElement function\n'));
    end
return
