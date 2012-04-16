/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef R2_FILE_PATH_H
#define R2_FILE_PATH_H

#include <list>
#include <string>

class R2FilePath
{
public:
    R2FilePath();

    /**
     * @brief find a file whose name is fileName and return its full path
     * @param fileName a file name 
     * @return return full path of the file, but if not found, return ""
     */
    std::string getPath(const char *fileName);

    /**
     * @brief set default data paths
     * @param path all default paths separated by colon(:)
     */
    bool setPath(const char *path);

    void setWorkingDirectory(int argc, const char **argv);

    /// get R2FilePath instance
    static R2FilePath *getInstance();

private:
    typedef std::list<std::string> StringList;
    typedef std::list<std::string>::iterator StringListIter;

    /// application directory
    static std::string _curDirectory;

    /// R2FilePath instance
    static R2FilePath _instance;

    /// all default file paths
    StringList _pathList;
};

#endif

