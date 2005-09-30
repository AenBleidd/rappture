
#include "RpLibrary.h"
#include "RpDict.h"
#include <string.h>
#include <fstream>
#include "RpFortranCommon.h"

/*
// rappture-fortran api
*/

#ifdef __cplusplus 
    extern "C" {
#endif

#ifdef COMPNAME_ADD1UNDERSCORE
#   define rp_init                 rp_init_
#   define rp_lib                  rp_lib_
#   define rp_lib_element_comp     rp_lib_element_comp_
#   define rp_lib_element_id       rp_lib_element_id_
#   define rp_lib_element_type     rp_lib_element_type_
#   define rp_lib_element_obj      rp_lib_element_obj_
#   define rp_lib_child_num        rp_lib_child_num_
#   define rp_lib_child_comp       rp_lib_child_comp_
#   define rp_lib_child_id         rp_lib_child_id_
#   define rp_lib_child_type       rp_lib_child_type_
#   define rp_lib_child_obj        rp_lib_child_obj_
#   define rp_lib_get              rp_lib_get_
#   define rp_lib_get_double       rp_lib_get_double_
#   define rp_lib_put_str          rp_lib_put_str_
#   define rp_lib_put_id_str       rp_lib_put_id_str_
#   define rp_lib_put_obj          rp_lib_put_obj_
#   define rp_lib_put_id_obj       rp_lib_put_id_obj_
#   define rp_lib_remove           rp_lib_remove_
#   define rp_lib_xml_len          rp_lib_xml_len_
#   define rp_lib_write_xml        rp_lib_write_xml_
#   define rp_lib_xml              rp_lib_xml_
#   define rp_quit                 rp_quit_
#elif defined(COMPNAME_ADD2UNDERSCORE)
#   define rp_init                 rp_init__
#   define rp_lib                  rp_lib__
#   define rp_lib_element_comp     rp_lib_element_comp__
#   define rp_lib_element_id       rp_lib_element_id__
#   define rp_lib_element_type     rp_lib_element_type__
#   define rp_lib_element_obj      rp_lib_element_obj__
#   define rp_lib_child_num        rp_lib_child_num__
#   define rp_lib_child_comp       rp_lib_child_comp__
#   define rp_lib_child_id         rp_lib_child_id__
#   define rp_lib_child_type       rp_lib_child_type__
#   define rp_lib_child_obj        rp_lib_child_obj__
#   define rp_lib_get              rp_lib_get__
#   define rp_lib_get_double       rp_lib_get_double__
#   define rp_lib_put_str          rp_lib_put_str__
#   define rp_lib_put_id_str       rp_lib_put_id_str__
#   define rp_lib_put_obj          rp_lib_put_obj__
#   define rp_lib_put_id_obj       rp_lib_put_id_obj__
#   define rp_lib_remove           rp_lib_remove__
#   define rp_lib_xml_len          rp_lib_xml_len__
#   define rp_lib_write_xml        rp_lib_write_xml__
#   define rp_lib_xml              rp_lib_xml__
#   define rp_quit                 rp_quit__
#elif defined(COMPNAME_NOCHANGE)
#   define rp_init                 rp_init
#   define rp_lib                  rp_lib
#   define rp_lib_element_comp     rp_lib_element_comp
#   define rp_lib_element_id       rp_lib_element_id
#   define rp_lib_element_type     rp_lib_element_type
#   define rp_lib_element_obj      rp_lib_element_obj
#   define rp_lib_child_num        rp_lib_child_num
#   define rp_lib_child_comp       rp_lib_child_comp
#   define rp_lib_child_id         rp_lib_child_id
#   define rp_lib_child_type       rp_lib_child_type
#   define rp_lib_child_obj        rp_lib_child_obj
#   define rp_lib_get              rp_lib_get
#   define rp_lib_get_double       rp_lib_get_double
#   define rp_lib_put_str          rp_lib_put_str
#   define rp_lib_put_id_str       rp_lib_put_id_str
#   define rp_lib_put_obj          rp_lib_put_obj
#   define rp_lib_put_id_obj       rp_lib_put_id_obj
#   define rp_lib_remove           rp_lib_remove
#   define rp_lib_xml_len          rp_lib_xml_len
#   define rp_lib_write_xml        rp_lib_write_xml
#   define rp_lib_xml              rp_lib_xml
#   define rp_quit                 rp_quit
#elif defined(COMPNAME_UPPERCASE)
#   define rp_init                 RP_INIT
#   define rp_lib                  RP_LIB
#   define rp_lib_element_comp     RP_LIB_ELEMENT_COMP
#   define rp_lib_element_id       RP_LIB_ELEMENT_ID
#   define rp_lib_element_type     RP_LIB_ELEMENT_TYPE
#   define rp_lib_element_obj      RP_LIB_ELEMENT_OBJ
#   define rp_lib_child_num        RP_LIB_CHILD_NUM
#   define rp_lib_child_comp       RP_LIB_CHILD_COMP
#   define rp_lib_child_id         RP_LIB_CHILD_ID
#   define rp_lib_child_type       RP_LIB_CHILD_TYPE
#   define rp_lib_child_obj        RP_LIB_CHILD_OBJ
#   define rp_lib_get              RP_LIB_GET
#   define rp_lib_get_double       RP_LIB_GET_DOUBLE
#   define rp_lib_put_str          RP_LIB_PUT_STR
#   define rp_lib_put_id_str       RP_LIB_PUT_ID_STR
#   define rp_lib_put_obj          RP_LIB_PUT_OBJ
#   define rp_lib_put_id_obj       RP_LIB_PUT_ID_OBJ
#   define rp_lib_remove           RP_LIB_REMOVE
#   define rp_lib_xml_len          RP_LIB_XML_LEN
#   define rp_lib_write_xml        RP_LIB_WRITE_XML
#   define rp_lib_xml              RP_LIB_XML
#   define rp_quit                 RP_QUIT
#endif


void rp_init();

int rp_lib(const char* filePath, int filePath_len);

void rp_lib_element_comp( int* handle, 
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void rp_lib_element_id(   int* handle, 
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

void rp_lib_element_type( int* handle,
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

int rp_lib_element_obj(   int* handle, 
                            char* path,
                            int path_len );

int rp_lib_child_num(    int* handle, 
                            char* path, 
                            int path_len);

int rp_lib_child_comp(   int* handle,    /* integer handle of library */
                            char* path,     /* DOM path of requested object */
                            char* type,     /* specific name of element */
                            int* childNum,  /* child number for iteration */
                            char* retText,  /* buffer to store return text */
                            int path_len,   /* length of path */
                            int type_len,   /* length of type */
                            int retText_len /* length of return text buffer */
                       );

int rp_lib_child_id(     int* handle,    /* integer handle of library */
                            char* path,     /* DOM path of requested object */
                            char* type,     /* specific name of element */
                            int* childNum,  /* child number for iteration */
                            char* retText,  /* buffer to store return text */
                            int path_len,   /* length of path */
                            int type_len,   /* length of type */
                            int retText_len /* length of return text buffer */
                       );

int rp_lib_child_type(   int* handle,    /* integer handle of library */
                            char* path,     /* DOM path of requested object */
                            char* type,     /* specific name of element */
                            int* childNum,  /* child number for iteration */
                            char* retText,  /* buffer to store return text */
                            int path_len,   /* length of path */
                            int type_len,   /* length of type */
                            int retText_len /* length of return text buffer */
                       );

int rp_lib_child_obj(    int* handle, 
                            char* path, 
                            char* type, 
                            int path_len,
                            int type_len
                          );

void rp_lib_get(          int* handle, 
                            char* path,
                            char* retText,
                            int path_len,
                            int retText_len );

double rp_lib_get_double( int* handle, 
                            char* path,
                            int path_len);

void rp_lib_put_str(     int* handle, 
                            char* path, 
                            char* value, 
                            int* append,
                            int path_len,
                            int value_len ); 

void rp_lib_put_id_str(  int* handle, 
                            char* path, 
                            char* value, 
                            char* id, 
                            int* append,
                            int path_len,
                            int value_len,
                            int id_len ); 

void rp_lib_put_obj(     int* handle, 
                            char* path, 
                            int* valHandle, 
                            int* append,
                            int path_len ); 

void rp_lib_put_id_obj(  int* handle, 
                            char* path, 
                            int* valHandle, 
                            char* id, 
                            int* append,
                            int path_len,
                            int id_len ); 

int rp_lib_remove(       int* handle, 
                            char* path, 
                            int path_len);

int rp_lib_xml_len(      int* handle);

void rp_lib_xml(         int* handle, 
                            char* retText, 
                            int retText_len);

int rp_lib_write_xml(     int* handle, 
                            char* outFile, 
                            int outFile_len);
void rp_quit();


/**********************************************************/

/**********************************************************/

// private member functions
int objType( char* flavor);

int storeObject_Lib(RpLibrary* objectName);
RpLibrary* getObject_Lib(int objKey);

// global vars
// dictionary to hold the python objects that 
// cannot be sent to fortran

#define DICT_TEMPLATE <int,RpLibrary*>
RpDict DICT_TEMPLATE fortObjDict_Lib;

#ifdef __cplusplus 
    }
#endif
