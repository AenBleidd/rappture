/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Library Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpLibrary.h"
#include <string.h>
#include <fstream>
#include "RpFortranCommon.h"
#include "RpLibraryFStubs.h"

#ifdef __cplusplus
    extern "C" {
#endif

int rp_lib ( const char* filePath, int filePath_len );

void rp_lib_element_comp (  int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void rp_lib_element_id (    int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void rp_lib_element_type (  int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

int rp_lib_element_obj (    int* handle,
                            char* path,
                            int path_len );

/*
 * rp_lib_child_num still needs to be written
 * keep function declaration around
int rp_lib_child_num (      int* handle,
                            char* path,
                            int* childHandle,
                            int path_len);
*/

int rp_lib_children (       int* handle, /* integer handle of library */
                            char* path, /* search path of the xml */
                            int* childHandle, /*int handle of last returned child*/
                            int path_len  /*length of the search path buffer*/
                    );


void rp_lib_get (           int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

double rp_lib_get_double (  int* handle,
                            char* path,
                            int path_len);

void rp_lib_put_str (       int* handle,
                            char* path,
                            char* value,
                            int* append,
                            int path_len,
                            int value_len );

void rp_lib_put_id_str (    int* handle,
                            char* path,
                            char* value,
                            char* id,
                            int* append,
                            int path_len,
                            int value_len,
                            int id_len );

/*
 * rp_lib_put_obj still needs to be written
 * keep function declaration around
void rp_lib_put_obj (       int* handle,
                            char* path,
                            int* valHandle,
                            int* append,
                            int path_len );
*/

/*
 * rp_lib_put_id_obj still needs to be written
 * keep function declaration around
void rp_lib_put_id_obj (    int* handle,
                            char* path,
                            int* valHandle,
                            char* id,
                            int* append,
                            int path_len,
                            int id_len );
*/

/*
 * rp_lib_remove still needs to be written
 * keep function declaration around
int rp_lib_remove (         int* handle,
                            char* path,
                            int path_len);
*/

/*
 * rp_lib_xml_len still needs to be written
 * keep function declaration around
int rp_lib_xml_len(         int* handle);
*/

void rp_lib_xml(            int* handle,
                            char* retText,
                            int retText_len);

int rp_lib_write_xml(       int* handle,
                            char* outFile,
                            int outFile_len);

void rp_lib_node_comp (     int* handle,
                            char* retText,
                            int retText_len);

void rp_lib_node_type (     int* handle,
                            char* retText,
                            int retText_len);

void rp_lib_node_id (       int* handle,
                            char* retText,
                            int retText_len);

void rp_result(             int* handle );

void rp_quit();


/**********************************************************/

#ifdef __cplusplus
    }
#endif
