#ifndef __R2_FILE_PATH_H__
#define __R2_FILE_PATH_H__

/** \class R2FilePath R2FilePath <R2/R2FilePath.h>
 *  \brief a class that searches a full path of the file
 *  \author Insoo Woo(iwoo@purdue.edu), Sung-Ye Kim (inside@purdue.edu)
 *  \author PhD research assistants in PURPL at Purdue University  
 *  \version 1.0
 *  \date    Nov. 2006-2007
 */

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
	R2string getPath(const char* fileName);

	/**
	 * @brief set default data paths
	 * @param filePath all default paths separated by colon(:)
	 */
	void setPath(const R2string& filePath);
public :

	/**
	 * @brief get R2FilePath instance
	 */
	static R2FilePath* getInstance();

};

#endif // __R2_FILE_PATH_H__

