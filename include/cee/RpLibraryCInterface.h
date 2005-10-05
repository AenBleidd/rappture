/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Library Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */


#ifdef __cplusplus
extern "C" {
#endif 

    typedef struct RpLibrary RpLibrary;

    // unit definition functions
    RpLibrary*  library             (const char* path);
    void freeLibrary                (RpLibrary* lib);

    // RpLibrary member functions
    RpLibrary*  element             (RpLibrary* lib, const char* path);
    RpLibrary*  elementAsObject     (RpLibrary* lib, const char* path);
    const char* elementAsType       (RpLibrary* lib, const char* path);
    const char* elementAsComp       (RpLibrary* lib, const char* path);
    const char* elementAsId         (RpLibrary* lib, const char* path);

    RpLibrary* children             (RpLibrary* lib, 
                                     const char* path, 
                                     RpLibrary* childEle);
    RpLibrary* childrenByType       (RpLibrary* lib, 
                                     const char* path, 
                                     RpLibrary* childEle, 
                                     const char* type   );
    RpLibrary* childrenAsObject     (RpLibrary* lib, 
                                     const char* path, 
                                     const char* type   );
    const char* childrenAsType      (RpLibrary* lib, 
                                     const char* path, 
                                     const char* type   );
    const char* childrenAsComp      (RpLibrary* lib, 
                                     const char* path, 
                                     const char* type   );
    const char* childrenAsId        (RpLibrary* lib, 
                                     const char* path, 
                                     const char* type   );

    RpLibrary*  get                 (RpLibrary* lib, const char* path);
    const char* getString           (RpLibrary* lib, const char* path);
    double      getDouble           (RpLibrary* lib, const char* path);

    void        put                 (RpLibrary* lib, 
                                     const char* path, 
                                     const char* value,
                                     const char* id,
                                     int append         );
    void        putStringId         (RpLibrary* lib, 
                                     const char* path, 
                                     const char* value,
                                     const char* id,
                                     int append         );
    void        putString           (RpLibrary* lib, 
                                     const char* path, 
                                     const char* value,
                                     int append         );
    void        putDoubleId         (RpLibrary* lib, 
                                     const char* path, 
                                     double value,
                                     const char* id,
                                     int append         );
    void        putDouble           (RpLibrary* lib, 
                                     const char* path, 
                                     double value,
                                     int append         );

    const char* xml                 (RpLibrary* lib);

    const char* nodeComp            (RpLibrary* node);
    const char* nodeType            (RpLibrary* node);
    const char* nodeId              (RpLibrary* node);

    void        result              (RpLibrary* lib);

#ifdef __cplusplus
}
#endif
