/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_UTIL_FILE_PATH_H
#define NV_UTIL_FILE_PATH_H

#include <list>
#include <string>

namespace nv {
namespace util {

class FilePath
{
public:
    FilePath();

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

    /// get FilePath instance
    static FilePath *getInstance();

private:
    typedef std::list<std::string> StringList;
    typedef std::list<std::string>::iterator StringListIter;

    /// application directory
    static std::string _curDirectory;

    /// FilePath instance
    static FilePath _instance;

    /// all default file paths
    StringList _pathList;
};

}
}

#endif

