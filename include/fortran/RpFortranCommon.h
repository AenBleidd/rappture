
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <string>

#ifndef _RpFORTRAN_COMMON_H
#define _RpFORTRAN_COMMON_H


#ifdef __cplusplus 
extern "C" {
#endif

char* null_terminate(char* inStr, int len);
std::string null_terminate_str(const char* inStr, int len);
void fortranify(const char* inBuff, char* retText, int retTextLen);

#ifdef __cplusplus 
}
#endif
    
#endif
