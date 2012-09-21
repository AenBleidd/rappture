/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Library Stub Headers
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifdef __cplusplus
    extern "C" {
#endif


/**********************************************************/
// INTERFACE FUNCTIONS - ONE UNDERSCORE

void rp_init_();

int rp_lib_ ( const char* filePath, int filePath_len );

void rp_lib_element_comp_ ( int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void rp_lib_element_id_ (   int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void rp_lib_element_type_ ( int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

int rp_lib_element_obj_ (   int* handle,
                            char* path,
                            int path_len );

int rp_lib_child_num_ (     int* handle,
                            char* path,
                            int* childHandle,
                            int path_len);

int rp_lib_children_ (      int* handle, /* integer handle of library */
                            char* path, /* search path of the xml */
                            int* childHandle, /*int handle of last returned child*/
                            int path_len  /*length of the search path buffer*/
                    );


void rp_lib_get_ (          int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void rp_lib_get_str_ (      int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

double rp_lib_get_double_ ( int* handle,
                            char* path,
                            int path_len);

int rp_lib_get_integer_ (   int* handle,
                            char* path,
                            int path_len);

int rp_lib_get_boolean_ (   int* handle,
                            char* path,
                            int path_len);

int rp_lib_get_file_ (      int* handle,
                            char* path,
                            char* fileName,
                            int path_len,
                            int fileName_len);

void rp_lib_put_str_ (      int* handle,
                            char* path,
                            char* value,
                            int* append,
                            int path_len,
                            int value_len );

void rp_lib_put_id_str_ (   int* handle,
                            char* path,
                            char* value,
                            char* id,
                            int* append,
                            int path_len,
                            int value_len,
                            int id_len );

void rp_lib_put_data_ (     int* handle,
                            char* path,
                            char* bytes,
                            int* nbytes,
                            int* append,
                            int path_len,
                            int bytes_len );

void rp_lib_put_file_ (     int* handle,
                            char* path,
                            char* fileName,
                            int* compress,
                            int* append,
                            int path_len,
                            int fileName_len );

/*
void rp_lib_put_obj_ (      int* handle,
                            char* path,
                            int* valHandle,
                            int* append,
                            int path_len );

void rp_lib_put_id_obj_ (   int* handle,
                            char* path,
                            int* valHandle,
                            char* id,
                            int* append,
                            int path_len,
                            int id_len );
*/

int rp_lib_remove_ (        int* handle,
                            char* path,
                            int path_len);

int rp_lib_xml_len_(        int* handle);

void rp_lib_xml_(           int* handle,
                            char* retText,
                            int retText_len);

int rp_lib_write_xml_(      int* handle,
                            char* outFile,
                            int outFile_len);

void rp_lib_node_comp_ (    int* handle,
                            char* retText,
                            int retText_len);

void rp_lib_node_type_ (    int* handle,
                            char* retText,
                            int retText_len);

void rp_lib_node_id_ (      int* handle,
                            char* retText,
                            int retText_len);

void rp_result_(            int* handle );


/**********************************************************/

void rp_init__();

int rp_lib__ ( const char* filePath, int filePath_len );

void rp_lib_element_comp__ (int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void rp_lib_element_id__ (  int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void rp_lib_element_type__ (int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

int rp_lib_element_obj__ (  int* handle,
                            char* path,
                            int path_len );

int rp_lib_child_num__ (    int* handle,
                            char* path,
                            int* childHandle,
                            int path_len);

int rp_lib_children__ (     int* handle, /* integer handle of library */
                            char* path, /* search path of the xml */
                            int* childHandle, /*int handle of last returned child*/
                            int path_len  /*length of the search path buffer*/
                    );


void rp_lib_get__ (         int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void rp_lib_get_str__ (     int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

double rp_lib_get_double__ (int* handle,
                            char* path,
                            int path_len);

int rp_lib_get_integer__ (  int* handle,
                            char* path,
                            int path_len);

int rp_lib_get_boolean__ (  int* handle,
                            char* path,
                            int path_len);

int rp_lib_get_file__ (     int* handle,
                            char* path,
                            char* fileName,
                            int path_len,
                            int fileName_len);

void rp_lib_put_str__ (     int* handle,
                            char* path,
                            char* value,
                            int* append,
                            int path_len,
                            int value_len );

void rp_lib_put_id_str__ (  int* handle,
                            char* path,
                            char* value,
                            char* id,
                            int* append,
                            int path_len,
                            int value_len,
                            int id_len );

void rp_lib_put_data__ (    int* handle,
                            char* path,
                            char* bytes,
                            int* nbytes,
                            int* append,
                            int path_len,
                            int bytes_len );

void rp_lib_put_file__ (    int* handle,
                            char* path,
                            char* fileName,
                            int* compress,
                            int* append,
                            int path_len,
                            int fileName_len );

/*
void rp_lib_put_obj__ (     int* handle,
                            char* path,
                            int* valHandle,
                            int* append,
                            int path_len );

void rp_lib_put_id_obj__ (  int* handle,
                            char* path,
                            int* valHandle,
                            char* id,
                            int* append,
                            int path_len,
                            int id_len );
*/

int rp_lib_remove__ (       int* handle,
                            char* path,
                            int path_len);

int rp_lib_xml_len__(       int* handle);

void rp_lib_xml__(          int* handle,
                            char* retText,
                            int retText_len);

int rp_lib_write_xml__(     int* handle,
                            char* outFile,
                            int outFile_len);

void rp_lib_node_comp__ (   int* handle,
                            char* retText,
                            int retText_len);

void rp_lib_node_type__ (   int* handle,
                            char* retText,
                            int retText_len);

void rp_lib_node_id__ (     int* handle,
                            char* retText,
                            int retText_len);

void rp_result__(           int* handle );


/**********************************************************/
// INTERFACE STUB FUNCTIONS - ALL CAPS

void RP_INIT();

int RP_LIB ( const char* filePath, int filePath_len );

void RP_LIB_ELEMENT_COMP (  int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void RP_LIB_ELEMENT_ID(    int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void RP_LIB_ELEMENT_TYPE (  int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

int RP_LIB_ELEMENT_OBJ (    int* handle,
                            char* path,
                            int path_len );

int RP_LIB_CHILD_NUM (      int* handle,
                            char* path,
                            int* childHandle,
                            int path_len);

int RP_LIB_CHILDREN (       int* handle, /* integer handle of library */
                            char* path, /* search path of the xml */
                            int* childHandle, /*int handle of last returned child*/
                            int path_len  /*length of the search path buffer*/
                    );


void RP_LIB_GET (           int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void RP_LIB_GET_STR (       int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

double RP_LIB_GET_DOUBLE (  int* handle,
                            char* path,
                            int path_len);

int RP_LIB_GET_INTEGER (    int* handle,
                            char* path,
                            int path_len);

int RP_LIB_GET_BOOLEAN (    int* handle,
                            char* path,
                            int path_len);

int RP_LIB_GET_FILE (       int* handle,
                            char* path,
                            char* fileName,
                            int path_len,
                            int fileName_len);

void RP_LIB_PUT_STR (       int* handle,
                            char* path,
                            char* value,
                            int* append,
                            int path_len,
                            int value_len );

void RP_LIB_PUT_ID_STR (    int* handle,
                            char* path,
                            char* value,
                            char* id,
                            int* append,
                            int path_len,
                            int value_len,
                            int id_len );

void RP_LIB_PUT_DATA (      int* handle,
                            char* path,
                            char* bytes,
                            int* nbytes,
                            int* append,
                            int path_len,
                            int bytes_len );

void RP_LIB_PUT_FILE (      int* handle,
                            char* path,
                            char* fileName,
                            int* compress,
                            int* append,
                            int path_len,
                            int fileName_len );
/*
void RP_LIB_PUT_OBJ (       int* handle,
                            char* path,
                            int* valHandle,
                            int* append,
                            int path_len );

void RP_LIB_PUT_ID_OBJ (    int* handle,
                            char* path,
                            int* valHandle,
                            char* id,
                            int* append,
                            int path_len,
                            int id_len );
*/

int RP_LIB_REMOVE (         int* handle,
                            char* path,
                            int path_len);

int RP_LIB_XML_LEN(         int* handle);

void RP_LIB_XML(            int* handle,
                            char* retText,
                            int retText_len);

int RP_LIB_WRITE_XML(       int* handle,
                            char* outFile,
                            int outFile_len);

void RP_LIB_NODE_COMP (     int* handle,
                            char* retText,
                            int retText_len);

void RP_LIB_NODE_TYPE (     int* handle,
                            char* retText,
                            int retText_len);

void RP_LIB_NODE_ID (       int* handle,
                            char* retText,
                            int retText_len);

void RP_RESULT(             int* handle );


/**********************************************************/
/**********************************************************/

#ifdef __cplusplus
    }
#endif
