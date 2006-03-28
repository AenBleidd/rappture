************README*****************
By Wei Qiao <qiaow@purdue.edu>	
***********************************

Hot Keys:
"W" print interpolation of Y
"A" Add points. Press "A" then click the screen to add point
"D" Delete points. Click the point to select it, the press "D" to delete
"P" Print coordinates of all control points. (FOR DEBUGGING)


**************
float* output
**************
Interpolation of control points are stored in an array of float named "output", 
which is a member of TransferFunctionWindow.
Total of 2048 samples are stored, with values ranging from [0,1]. All samples are initialized to 0 ,when program starts.

***********
Important!
***********
Always call WriteControlPoints() before you use the values in output array. To improve performance, the values are 
not updated upon each change of control points. By calling WriteControlPoints(), interpolation is updated. 


