#include "rp_defs.h"

string RpErrorStr;

void RpAppendErr(const char* str)
{
	RpErrorStr.append(str, strlen(str));
	//cout << RpErrorStr << endl;
}

void RpPrintErr()
{
    cout << RpErrorStr << endl;
}

