/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <R2/R2FilePath.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

R2string R2FilePath::_curDirectory;
R2FilePath R2FilePath::_instance;

char seps[]   = ":";

R2FilePath::R2FilePath()
{
    char buff[BUFSIZ+2];
#ifdef WIN32
    _getcwd(buff, 255);
#else
    if (getcwd(buff, BUFSIZ) == NULL) {
	fprintf(stderr, "can't get current working directory\n");
    }
#endif
    size_t length = strlen(buff);
    buff[length] = '/';
    buff[length + 1] = '\0';
    _curDirectory = buff;
}

bool 
R2FilePath::setPath(const char *filePath)
{
    char buff[255];

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
		_pathList.push_back(R2string(p));
	    } else {
		_pathList.push_back(R2string(p) + "/");
	    }
	} else {
	    if (*last == '/' || *last == '\\') {
		_pathList.push_back(_curDirectory + R2string(p));
	    } else {
		_pathList.push_back(_curDirectory + R2string(p) + "/");
	    }
	}
    }
    return true;
}

R2FilePath* R2FilePath::getInstance()
{
    return &_instance;
}

const char *
R2FilePath::getPath(const char* fileName)
{
    R2string path;

    R2stringListIter iter;
    int nameLength = strlen(fileName);
    for (iter = _pathList.begin(); iter != _pathList.end(); ++iter) {
	char *path;

	path = new char[strlen((char *)(*iter)) + 1 + nameLength + 1];
	//sprintf(path, "%s/%s", (char *)(*iter), fileName);
	sprintf(path, "%s%s", (char *)(*iter), fileName);
	if (access(path, R_OK) == 0) {
	    return path;
	}
	delete [] path;
    }
    return NULL;
}

void 
R2FilePath::setWorkingDirectory(int argc, const char** argv)
{
    char buff[255];

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

