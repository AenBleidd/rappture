#include "RpFortranCommon.h"

char* null_terminate(char* inStr, int len) {
    int retVal = 0;
    char* newStr = NULL;
    char* current = NULL;

    if (inStr && (len > 0) ) {

        current = inStr+len-1;

        while ((len > 0) && (isspace(*(current)))) {
            // dont strip off newlines

            if ( (*(current) == '\f')
              || (*(current) == '\n')
              || (*(current) == '\r')
              || (*(current) == '\t')
              || (*(current) == '\v') )
            {
                break;
            }

            if (--len) {
                current--;
            }
        }

        newStr = (char*) calloc(len+1,(sizeof(char)));
        strncpy(newStr,inStr,len);
        *(newStr+len) = '\0';

        retVal++;
    }

    return newStr;
}

void fortranify(const char* inBuff, char* retText, int retTextLen) {

    int inBuffLen = 0;
    int i = 0;

    inBuffLen = strlen(inBuff);

    strncpy(retText, inBuff, retTextLen);

    // fortran-ify the string
    if (inBuffLen < retTextLen) {
        for (i = inBuffLen; i < retTextLen; i++) {
            retText[i] = ' ';
        }
    }
}
