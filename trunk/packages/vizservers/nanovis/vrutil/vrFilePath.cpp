/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrutil/vrFilePath.h>

#ifdef _WIN32
#pragma warning (disable : 4996)
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#endif

std::string vrFilePath::_curDirectory;
vrFilePath vrFilePath::_instance;

static char seps[]  = ";";
vrFilePath::vrFilePath()
{
    char buff[255];
#ifdef _WIN32
    if (_getcwd(buff, 255) == 0) {
        printf("failed to get the current directory in vrFilePath::vrFilePath\n");
    }
#else
    if (getcwd(buff, 255) == 0) {
        printf("failed to get the current directory in vrFilePath::vrFilePath\n");
    }
#endif

    size_t length = strlen(buff);
    buff[length] = '/';
    buff[length + 1] = '\0';

    unsigned int len = (unsigned int) strlen(buff);
    for (unsigned int i = 0; i < len; ++i) {
        if (buff[i] == '\\') buff[i] = '/'; 
    }
    _curDirectory = buff;

    char * path;
    path = getenv("VR_ROOT_PATH");
    if (path) {
        int len = (int) strlen(path);
        for (int i = 0; i < len; ++i) {
            if (buff[i] == '\\') buff[i] = '/'; 
        }

        strcpy(buff, path);
        strcpy(buff + strlen(path), "/resources/");
        _pathList.push_back(buff);
    }
}

void vrFilePath::setPath(const std::string& filePath)
{
    char buff[255];

    _pathList.clear();

    char * path;
    path = getenv("VR_ROOT_PATH");
    if (path) {
        strcpy(buff, path);
        int len = (int)strlen(path);
        for (int i = 0; i < len; ++i) {
            if (buff[i] == '\\') buff[i] = '/'; 
        }

        strcpy(buff + strlen(path), "/resources/");
        _pathList.push_back(buff);
    }

    char *token;
    strcpy(buff, filePath.c_str());
    token = strtok(buff, seps );

    int lastIndex;
    while (token != NULL) {
        lastIndex = (int)strlen(token) - 1;
        //if (token[0] == '/' || token[1] == ';')
        if (token[0] == '/' || ((lastIndex >= 1) && token[1] == ':')) {
            if (token[lastIndex] == '/' || token[lastIndex] == '\\') {
                _pathList.push_back(std::string(token));
            } else {
                _pathList.push_back(std::string(token) + "/");
            }
        } else {
            if (token[lastIndex] == '/' || token[lastIndex] == '\\') {
                _pathList.push_back(_curDirectory + std::string(token));
            } else {
                _pathList.push_back(_curDirectory + std::string(token) + "/");
            }
        }
        token = strtok(NULL, seps);
    }
}

vrFilePath *vrFilePath::getInstance()
{
    return &_instance;
}

std::string vrFilePath::getPath(const char* fileName)
{
    std::string path;
    FILE *file;

    StringListIter iter;
    for (iter = _pathList.begin(); iter != _pathList.end(); ++iter) {
#ifdef _WIN32
        if (_chdir(iter->c_str()) == -1) {
            printf("error : change dir (%s)", iter->c_str());
        }
#else
        if (chdir(iter->c_str()) == -1) {
            printf("error : change dir (%s)", iter->c_str());
        }
#endif
        if ((file = fopen(fileName, "rb")) != NULL) {
            fclose(file);

            path = (*iter) + fileName;

            printf("returned [%s]\n", path.c_str());
            break;
        }
    }

#ifdef _WIN32
    if (_chdir(_curDirectory.c_str()) != -1) {
        printf("error : change dir (%s)", _curDirectory.c_str());
    }
#else
    if (chdir(_curDirectory.c_str()) != -1) {
        printf("error : change dir (%s)", _curDirectory.c_str());
    }
#endif

    return path;
}

void vrFilePath::setWorkingDirectory(int argc, const char **argv)
{
    char buff[255];
    strcpy(buff, argv[0]);
    for (int i = (int) strlen(buff) - 1; i >= 0; --i) {
        if (buff[i] == '\\' || buff[i] == '/') {
            buff[i] = '/'; 
            buff[i + 1] = '\0'; 
            break;
        }
    }

    int len = (int)strlen(buff);
    for (int i = 0; i < len; ++i) {
        if (buff[i] == '\\') buff[i] = '/'; 
    }
    _curDirectory = buff;
}
