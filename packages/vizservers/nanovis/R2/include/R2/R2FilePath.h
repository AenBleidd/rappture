/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __R2_FILE_PATH_H__
#define __R2_FILE_PATH_H__

#include <list>
#include <R2/R2string.h>

class R2FilePath {
    typedef std::list<R2string> R2stringList;
    typedef std::list<R2string>::iterator R2stringListIter;
    
    /**
     * @brief application directory
     */
    static R2string _curDirectory;
    
    /**
     * @brief R2FilePath instance
     */
    static R2FilePath _instance;
    
    /**
     * @brief all default file paths
     */
    R2stringList _pathList;
public :
    /**
     * @brief constructor
     */
    R2FilePath();
    
    /**
     * @brief find a file whose name is fileName and return its full path
     * @param fileName a file name 
     * @return return full path of the file, but if not found, return ""
     */
    const char *getPath(const char* fileName);
    
    /**
     * @brief set default data paths
     * @param filePath all default paths separated by colon(:)
     */
    bool setPath(const char *path);
    
    /**
     *
     */
    void setWorkingDirectory(int argc, const char** argv);
public :
    
    /**
     * @brief get R2FilePath instance
     */
    static R2FilePath* getInstance();
};

#endif // __R2_FILE_PATH_H__

