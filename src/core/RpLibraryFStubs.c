/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Library Stub Functions
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

int
rp_lib_ ( const char* filePath, int filePath_len ) {
    return rp_lib (filePath,filePath_len);
}

int
rp_lib__ ( const char* filePath, int filePath_len ) {
    return rp_lib (filePath,filePath_len);
}

int
RP_LIB( const char* filePath, int filePath_len ) {
    return rp_lib (filePath,filePath_len);
}

void
rp_lib_element_comp_ ( int* handle,
                       char* path,
                       char* retText,
                       int path_len,
                       int retText_len ) {

    return rp_lib_element_comp(handle,path,retText,path_len,retText_len);
}

void
rp_lib_element_comp__ ( int* handle,
                       char* path,
                       char* retText,
                       int path_len,
                       int retText_len ) {

    return rp_lib_element_comp(handle,path,retText,path_len,retText_len);
}

void
RP_LIB_ELEMENT_COMP ( int* handle,
                       char* path,
                       char* retText,
                       int path_len,
                       int retText_len ) {

    return rp_lib_element_comp(handle,path,retText,path_len,retText_len);
}

void
rp_lib_element_id_ (   int* handle,
                       char* path,
                       char* retText,
                       int path_len,
                       int retText_len ) {

    return rp_lib_element_id(handle,path,retText,path_len,retText_len);
}

void
rp_lib_element_id__ (   int* handle,
                       char* path,
                       char* retText,
                       int path_len,
                       int retText_len ) {

    return rp_lib_element_id(handle,path,retText,path_len,retText_len);
}

void
RP_LIB_ELEMENT_ID (   int* handle,
                       char* path,
                       char* retText,
                       int path_len,
                       int retText_len ) {

    return rp_lib_element_id(handle,path,retText,path_len,retText_len);
}

void
rp_lib_element_type_ ( int* handle,
                       char* path,
                       char* retText,
                       int path_len,
                       int retText_len ) {

    return rp_lib_element_type(handle,path,retText,path_len,retText_len);
}

void
rp_lib_element_type__ ( int* handle,
                       char* path,
                       char* retText,
                       int path_len,
                       int retText_len ) {

    return rp_lib_element_type(handle,path,retText,path_len,retText_len);
}

void
RP_LIB_ELEMENT_TYPE  ( int* handle,
                       char* path,
                       char* retText,
                       int path_len,
                       int retText_len ) {

    return rp_lib_element_type(handle,path,retText,path_len,retText_len);
}

int
rp_lib_element_obj_ (   int* handle,
                        char* path,
                        int path_len ) {

    return rp_lib_element_obj(handle,path,path_len);
}

int
rp_lib_element_obj__ (   int* handle,
                        char* path,
                        int path_len ) {

    return rp_lib_element_obj(handle,path,path_len);
}

int
RP_LIB_ELEMENT_OBJ  (   int* handle,
                        char* path,
                        int path_len ) {

    return rp_lib_element_obj(handle,path,path_len);
}

/*
 * rp_lib_child_num still needs to be written 
 * keep these stub functions around
int
rp_lib_child_num_ (     int* handle,
                        char* path,
                        int* childHandle,
                        int path_len) {

    return rp_lib_child_num(handle,path,childHandle,path_len);
}

int
rp_lib_child_num__ (     int* handle,
                        char* path,
                        int* childHandle,
                        int path_len) {

    return rp_lib_child_num(handle,path,childHandle,path_len);
}

int
RP_LIB_CHILD_NUM (     int* handle,
                        char* path,
                        int* childHandle,
                        int path_len) {

    return rp_lib_child_num(handle,path,childHandle,path_len);
}
*/

int
rp_lib_children_ (     int* handle, /* integer handle of library */
                       char* path, /* search path of the xml */
                       int* childHandle, /*int handle of last returned child*/
                       int path_len  /*length of the search path buffer*/
                 ) {

    return rp_lib_children(handle,path,childHandle,path_len);
}

int
rp_lib_children__ (     int* handle, /* integer handle of library */
                       char* path, /* search path of the xml */
                       int* childHandle, /*int handle of last returned child*/
                       int path_len  /*length of the search path buffer*/
                 ) {

    return rp_lib_children(handle,path,childHandle,path_len);
}

int
RP_LIB_CHILDREN (     int* handle, /* integer handle of library */
                       char* path, /* search path of the xml */
                       int* childHandle, /*int handle of last returned child*/
                       int path_len  /*length of the search path buffer*/
                 ) {

    return rp_lib_children(handle,path,childHandle,path_len);
}

void
rp_lib_get_ (           int* handle,
                        char* path,
                        char* retText,
                        int path_len,
                        int retText_len ) {

    return rp_lib_get(handle,path,retText,path_len,retText_len);
}

void
rp_lib_get__ (           int* handle,
                        char* path,
                        char* retText,
                        int path_len,
                        int retText_len ) {

    return rp_lib_get(handle,path,retText,path_len,retText_len);
}

void
RP_LIB_GET  (           int* handle,
                        char* path,
                        char* retText,
                        int path_len,
                        int retText_len ) {

    return rp_lib_get(handle,path,retText,path_len,retText_len);
}

void
rp_lib_get_str_ (       int* handle,
                        char* path,
                        char* retText,
                        int path_len,
                        int retText_len ) {

    return rp_lib_get_str(handle,path,retText,path_len,retText_len);
}

void
rp_lib_get_str__ (      int* handle,
                        char* path,
                        char* retText,
                        int path_len,
                        int retText_len ) {

    return rp_lib_get_str(handle,path,retText,path_len,retText_len);
}

void
RP_LIB_GET_STR  (       int* handle,
                        char* path,
                        char* retText,
                        int path_len,
                        int retText_len ) {

    return rp_lib_get_str(handle,path,retText,path_len,retText_len);
}

double
rp_lib_get_double_ (    int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_get_double(handle,path,path_len);
}

double
rp_lib_get_double__ (    int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_get_double(handle,path,path_len);
}

double
RP_LIB_GET_DOUBLE (    int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_get_double(handle,path,path_len);
}

int
rp_lib_get_integer_ (   int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_get_integer(handle,path,path_len);
}

int
rp_lib_get_integer__ (  int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_get_integer(handle,path,path_len);
}

int
RP_LIB_GET_INTEGER (    int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_get_integer(handle,path,path_len);
}

int
rp_lib_get_boolean_ (   int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_get_boolean(handle,path,path_len);
}

int
rp_lib_get_boolean__ (  int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_get_boolean(handle,path,path_len);
}

int
RP_LIB_GET_BOOLEAN (    int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_get_boolean(handle,path,path_len);
}

int rp_lib_get_file_ (  int* handle,
                        char* path,
                        char* fileName,
                        int path_len,
                        int fileName_len) {

    return rp_lib_get_file(handle,path,fileName,path_len,fileName_len);
}

int rp_lib_get_file__ ( int* handle,
                        char* path,
                        char* fileName,
                        int path_len,
                        int fileName_len) {

    return rp_lib_get_file(handle,path,fileName,path_len,fileName_len);
}

int RP_LIB_GET_FILE (   int* handle,
                        char* path,
                        char* fileName,
                        int path_len,
                        int fileName_len) {

    return rp_lib_get_file(handle,path,fileName,path_len,fileName_len);
}

void
rp_lib_put_str_ (       int* handle,
                        char* path,
                        char* value,
                        int* append,
                        int path_len,
                        int value_len ) {

    rp_lib_put_str(handle,path,value,append,path_len,value_len);
    return;
}

void
rp_lib_put_str__ (      int* handle,
                        char* path,
                        char* value,
                        int* append,
                        int path_len,
                        int value_len ) {

    rp_lib_put_str(handle,path,value,append,path_len,value_len);
    return;
}

void
RP_LIB_PUT_STR (       int* handle,
                        char* path,
                        char* value,
                        int* append,
                        int path_len,
                        int value_len ) {

    rp_lib_put_str(handle,path,value,append,path_len,value_len);
    return;
}

void
rp_lib_put_id_str_ (    int* handle,
                        char* path,
                        char* value,
                        char* id,
                        int* append,
                        int path_len,
                        int value_len,
                        int id_len ) {

    rp_lib_put_id_str(handle,path,value,id,append,path_len,value_len,id_len);
    return;
}

void
rp_lib_put_id_str__ (    int* handle,
                        char* path,
                        char* value,
                        char* id,
                        int* append,
                        int path_len,
                        int value_len,
                        int id_len ) {

    rp_lib_put_id_str(handle,path,value,id,append,path_len,value_len,id_len);
    return;
}

void
RP_LIB_PUT_ID_STR (    int* handle,
                        char* path,
                        char* value,
                        char* id,
                        int* append,
                        int path_len,
                        int value_len,
                        int id_len ) {

    rp_lib_put_id_str(handle,path,value,id,append,path_len,value_len,id_len);
    return;
}

void
rp_lib_put_data_ (      int* handle,
                        char* path,
                        char* bytes,
                        int* nbytes,
                        int* append,
                        int path_len,
                        int bytes_len ) {

    rp_lib_put_data(handle,path,bytes,nbytes,append,path_len,bytes_len);
    return;
}

void
rp_lib_put_data__ (     int* handle,
                        char* path,
                        char* bytes,
                        int* nbytes,
                        int* append,
                        int path_len,
                        int bytes_len ) {

    rp_lib_put_data(handle,path,bytes,nbytes,append,path_len,bytes_len);
    return;
}

void
RP_LIB_PUT_DATA (       int* handle,
                        char* path,
                        char* bytes,
                        int* nbytes,
                        int* append,
                        int path_len,
                        int bytes_len ) {

    rp_lib_put_data(handle,path,bytes,nbytes,append,path_len,bytes_len);
    return;
}

void
rp_lib_put_file_ (      int* handle,
                        char* path,
                        char* fileName,
                        int* compress,
                        int* append,
                        int path_len,
                        int fileName_len ) {

    rp_lib_put_file(handle,path,fileName,compress,append,path_len,fileName_len);
    return;
}

void
rp_lib_put_file__ (     int* handle,
                        char* path,
                        char* fileName,
                        int* compress,
                        int* append,
                        int path_len,
                        int fileName_len ) {

    rp_lib_put_file(handle,path,fileName,compress,append,path_len,fileName_len);
    return;
}

void
RP_LIB_PUT_FILE (       int* handle,
                        char* path,
                        char* fileName,
                        int* compress,
                        int* append,
                        int path_len,
                        int fileName_len ) {

    rp_lib_put_file(handle,path,fileName,compress,append,path_len,fileName_len);
    return;
}

/*
 * rp_lib_put_obj still needs to be written 
 * keep these stub functions around
void
rp_lib_put_obj_ (       int* handle,
                        char* path,
                        int* valHandle,
                        int* append,
                        int path_len ) {

    return rp_lib_put_obj(handle,path,valHandle,append,path_len);
}

void
rp_lib_put_obj__ (       int* handle,
                        char* path,
                        int* valHandle,
                        int* append,
                        int path_len ) {

    return rp_lib_put_obj(handle,path,valHandle,append,path_len);
}

void
RP_LIB_PUT_OBJ (       int* handle,
                        char* path,
                        int* valHandle,
                        int* append,
                        int path_len ) {

    return rp_lib_put_obj(handle,path,valHandle,append,path_len);
}
*/

/*
 * rp_lib_put_id_obj still needs to be written 
 * keep these stub functions around
void
rp_lib_put_id_obj_ (    int* handle,
                        char* path,
                        int* valHandle,
                        char* id,
                        int* append,
                        int path_len,
                        int id_len ) {

    return rp_lib_put_id_obj(handle,path,valHandle,id,append,path_len,id_len);
}

void
rp_lib_put_id_obj__ (    int* handle,
                        char* path,
                        int* valHandle,
                        char* id,
                        int* append,
                        int path_len,
                        int id_len ) {

    return rp_lib_put_id_obj(handle,path,valHandle,id,append,path_len,id_len);
}

void
RP_LIB_PUT_ID_OBJ (    int* handle,
                        char* path,
                        int* valHandle,
                        char* id,
                        int* append,
                        int path_len,
                        int id_len ) {

    return rp_lib_put_id_obj(handle,path,valHandle,id,append,path_len,id_len);
}
*/

/*
 * rp_lib_remove still needs to be written 
 * keep these stub functions around
int
rp_lib_remove_ (        int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_remove(handle,path,path_len);
}

int
rp_lib_remove__ (        int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_remove(handle,path,path_len);
}

int
RP_LIB_REMOVE  (        int* handle,
                        char* path,
                        int path_len) {

    return rp_lib_remove(handle,path,path_len);
}
*/

/*
 * rp_lib_xml_len still needs to be written 
 * keep these stub functions around
int
rp_lib_xml_len_(        int* handle) {
    return rp_lib_xml_len(handle);
}

int
rp_lib_xml_len__(        int* handle) {
    return rp_lib_xml_len(handle);
}

int
RP_LIB_XML_LEN (        int* handle) {
    return rp_lib_xml_len(handle);
}
*/

void
rp_lib_xml_(            int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_xml(handle,retText,retText_len);
}

void
rp_lib_xml__(            int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_xml(handle,retText,retText_len);
}

void
RP_LIB_XML (            int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_xml(handle,retText,retText_len);
}

int
rp_lib_write_xml_(      int* handle,
                        char* outFile,
                        int outFile_len) {
    return rp_lib_write_xml (handle,outFile,outFile_len);
}

int
rp_lib_write_xml__(      int* handle,
                        char* outFile,
                        int outFile_len) {
    return rp_lib_write_xml (handle,outFile,outFile_len);
}

int
RP_LIB_WRITE_XML(      int* handle,
                        char* outFile,
                        int outFile_len) {
    return rp_lib_write_xml (handle,outFile,outFile_len);
}

void
rp_lib_node_comp_ (     int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_node_comp (handle,retText,retText_len);
}

void
rp_lib_node_comp__ (     int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_node_comp (handle,retText,retText_len);
}

void
RP_LIB_NODE_COMP (     int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_node_comp (handle,retText,retText_len);
}

void
rp_lib_node_type_ (     int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_node_type (handle,retText,retText_len);
}

void
rp_lib_node_type__ (     int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_node_type (handle,retText,retText_len);
}

void
RP_LIB_NODE_TYPE (      int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_node_type (handle,retText,retText_len);
}

void
rp_lib_node_id_ (       int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_node_id(handle,retText,retText_len);
}

void
rp_lib_node_id__ (       int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_node_id(handle,retText,retText_len);
}

void
RP_LIB_NODE_ID (        int* handle,
                        char* retText,
                        int retText_len) {
    return rp_lib_node_id(handle,retText,retText_len);
}

void
rp_result_(             int* handle ) {
    return rp_result(handle);
}

void
rp_result__(             int* handle ) {
    return rp_result(handle);
}

void
RP_RESULT (             int* handle ) {
    return rp_result(handle);
}


/**********************************************************/
/**********************************************************/

#ifdef __cplusplus
    }
#endif
