#include <R2/R2FilePath.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#endif

R2string R2FilePath::_curDirectory;
R2FilePath R2FilePath::_instance;

char seps[]   = ":";
R2FilePath::R2FilePath()
{
	char buff[255];
#ifdef WIN32
	_getcwd(buff, 255);
#else
    getcwd(buff, 100);
#endif

	int length = strlen(buff);
	buff[length] = '/';
	buff[length + 1] = '\0';

	_curDirectory = buff;

}

void R2FilePath::setPath(const R2string& filePath)
{
	char buff[255];

	_pathList.clear();

	char *token;
	strcpy(buff, filePath);
	token = strtok(buff, seps );

	int lastIndex;
	while( token != NULL )
	{
		lastIndex = strlen(token) - 1;
		//if (token[0] == '/' || token[1] == ':')
		if (token[0] == '/')
		{
			if (token[lastIndex] == '/' || token[lastIndex] == '\\')
			{
				_pathList.push_back(R2string(token));
			}
			else 
			{
				_pathList.push_back(R2string(token) + "/");
			}
			
		}
		else
		{
            FILE* f = fopen("/tmp/insoo.txt", "wt");
            fprintf(f, "%s\n", (char*) (_curDirectory + R2string(token)));
            fprintf(f, "%s\n", (char*) (_curDirectory + R2string(token) + "/"));
            fclose(f);

			if (token[lastIndex] == '/' || token[lastIndex] == '\\')
			{
				_pathList.push_back(_curDirectory + R2string(token));
			}
			else 
			{
				_pathList.push_back(_curDirectory + R2string(token) + "/");
			}

			
		}
		token = strtok( NULL, seps );
	}
}

R2FilePath* R2FilePath::getInstance()
{
	return &_instance;
}

R2string R2FilePath::getPath(const char* fileName)
{
	R2string path;
	FILE* file;

	R2stringListIter iter;
	for (iter = _pathList.begin(); iter != _pathList.end(); ++iter)
	{
		chdir(*iter);

		if ((file = fopen(fileName, "rb")) != NULL)
		{
			fclose(file);
			
			path = (*iter) + fileName;

			break;
		}
		
	}

	chdir(_curDirectory);

	return path;
}

void R2FilePath::setWorkingDirectory(int argc, const char** argv)
{
    char buff[255];
    strcpy(buff, argv[0]);
    for (int i = strlen(buff) - 1; i >= 0; --i)
    {
        if (buff[i] == '\\' || buff[i] == '/')
        {
            buff[i] = '/'; 
            buff[i + 1] = '\0'; 
            break;
        }
    }

	_curDirectory = buff;
}

