/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <R2/R2FilePath.h>

#include "Trace.h"

using namespace nv::util;

std::string FilePath::_curDirectory;
FilePath FilePath::_instance;

static char seps[] = ":";

FilePath::FilePath()
{
    char buff[BUFSIZ+2];
#ifdef _WIN32
    _getcwd(buff, sizeof(buff)-1);
#else
    if (getcwd(buff, sizeof(buff)-1) == NULL) {
        ERROR("can't get current working directory\n");
    }
#endif
    size_t length = strlen(buff);
    buff[length] = '/';
    buff[length + 1] = '\0';
    _curDirectory = buff;
}

bool 
FilePath::setPath(const char *filePath)
{
    char buff[256];

    if (filePath == NULL) {
        return false;
    }
    _pathList.clear();

    char *p;
    strcpy(buff, filePath);
    for (p = strtok(buff, seps); p != NULL; p = strtok(NULL, seps)) {
        char *last;
        struct stat buf;
        if ((stat(p, &buf) != 0) || (!S_ISDIR(buf.st_mode))) {
            return false;
        }
        last = p + strlen(p) - 1;
        if (*p == '/') {
            if (*last == '/' || *last == '\\') {
                _pathList.push_back(std::string(p));
            } else {
                _pathList.push_back(std::string(p) + "/");
            }
        } else {
            if (*last == '/' || *last == '\\') {
                _pathList.push_back(_curDirectory + std::string(p));
            } else {
                _pathList.push_back(_curDirectory + std::string(p) + "/");
            }
        }
    }
    return true;
}

FilePath *
FilePath::getInstance()
{
    return &_instance;
}

std::string
FilePath::getPath(const char* fileName)
{
    std::string path;

    for (StringListIter iter = _pathList.begin();
         iter != _pathList.end(); ++iter) {
        path = *iter + std::string(fileName);

        if (access(path.c_str(), R_OK) == 0) {
            return path;
        } else {
            path = "";
        }
    }
    return path;
}

void 
FilePath::setWorkingDirectory(int argc, const char **argv)
{
    char buff[256];

    strcpy(buff, argv[0]);
    for (int i = strlen(buff) - 1; i >= 0; --i) {
        if (buff[i] == '\\' || buff[i] == '/') {
            buff[i] = '/';
            buff[i + 1] = '\0';
            break;
        }
    }
    _curDirectory = buff;
}
