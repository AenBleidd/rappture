/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include <vrutil/vrUtil.h>
#include <string>
#include <list>

#ifdef WIN32
#pragma warning ( disable : 4251)
#endif

class VrUtilExport vrFilePath {
	typedef std::list<std::string> StringList;
	typedef std::list<std::string>::iterator StringListIter;

	/**
	 * @brief application directory
	 */
	static std::string _curDirectory;

	/**
	 * @brief vrFilePath instance
	 */
	static vrFilePath _instance;

	/**
	 * @brief all default file paths
	 */
	StringList _pathList;
public :
	/**
	 * @brief constructor
	 */
	vrFilePath();

	/**
	 * @brief find a file whose name is fileName and return its full path
	 * @param fileName a file name 
	 * @return return full path of the file, but if not found, return ""
	 */
	std::string getPath(const char* fileName);

	/**
	 * @brief set default data paths
	 * @param filePath all default paths separated by colon(:)
	 */
	void setPath(const std::string& filePath);

    /**
     *
     */
    void setWorkingDirectory(int argc, const char** argv);
public :

	/**
	 * @brief get vrFilePath instance
	 */
	static vrFilePath* getInstance();

};


