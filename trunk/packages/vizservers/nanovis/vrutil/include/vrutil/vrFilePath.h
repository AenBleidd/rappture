/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRFILEPATH_H
#define VRFILEPATH_H

#include <vrutil/vrUtil.h>
#include <string>
#include <list>

class vrFilePath
{
public:
    vrFilePath();

    /**
     * @brief find a file whose name is fileName and return its full path
     * @param fileName a file name 
     * @return return full path of the file, but if not found, return ""
     */
    std::string getPath(const char *fileName);

    /**
     * @brief set default data paths
     * @param filePath all default paths separated by colon(:)
     */
    void setPath(const std::string& filePath);

    void setWorkingDirectory(int argc, const char **argv);

    /// get vrFilePath instance
    static vrFilePath* getInstance();

private:
    typedef std::list<std::string> StringList;
    typedef std::list<std::string>::iterator StringListIter;

    /// application directory
    static std::string _curDirectory;

    /// vrFilePath instance
    static vrFilePath _instance;

    /// all default file paths
    StringList _pathList;
};

#endif
