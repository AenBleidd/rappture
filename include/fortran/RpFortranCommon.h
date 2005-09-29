
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef _RpFORTRAN_COMMON_H
#define _RpFORTRAN_COMMON_H

#ifdef __cplusplus 
extern "C" {
#endif

char* null_terminate(char* inStr, int len);

void fortranify(const char* inBuff, char* retText, int retTextLen);

#ifdef __cplusplus 
}
#endif
    
#endif
