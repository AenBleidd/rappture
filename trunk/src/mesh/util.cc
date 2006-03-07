#include "util.h"

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

void filterTailingBlanks(char* str, int len)
{
	char* ptr = &str[len-1];

	while (ptr >= str && *ptr == ' ') {
		*ptr = '\0';
		ptr--;
	}
}
