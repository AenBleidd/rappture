/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrutil/vrColorBrewerFactory.h>
#include <vrutil/vrColorBrewer.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const int COLOR_SCHEME_ORDER_SEQ[][9] =		{	{2, 5, 8, -1, -1, -1, -1, -1, -1},
												{1, 4, 6, 9, -1, -1, -1, -1, -1},
												{1, 4, 6, 8, 10, -1, -1, -1, -1},
												{1, 3, 5, 6, 8, 10, -1, -1, -1},
												{1, 3, 5, 6, 7, 9, 11, -1, -1},
												{0, 2, 3, 5, 6, 7, 9, 11, -1},
												{0, 2, 3, 5, 6, 7, 9, 10, 12}	};

static const int COLOR_SCHEME_ORDER_DIV[][11] =	{	{10, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1},
												{12, 9, 5, 2, -1, -1, -1, -1, -1, -1, -1},
												{12, 9, 7, 5, 2, -1, -1, -1, -1, -1, -1},
												{13, 10, 8, 6, 4, 1, -1, -1, -1, -1, -1},
												{13, 10, 8, 7, 6, 4, 1, -1, -1, -1},
												{13, 11, 9, 8, 6, 5, 3, 1, -1, -1, -1},
												{13, 11, 9, 8, 7, 6, 5, 3, 1, -1, -1},
												{14, 13, 11, 9, 8, 6, 5, 3, 1, 0, -1},
												{14, 13, 11, 9, 8, 7, 6, 5, 3, 1, 0}	};

vrColorBrewerFactory* vrColorBrewerFactory::_instance = 0;


vrColorBrewerFactory* vrColorBrewerFactory::getInstance()
{
	if (_instance == 0) {
		_instance = new vrColorBrewerFactory();
		_instance->loadColorBrewerList();
	}
	return _instance;
}

vrColorBrewerFactory::vrColorBrewerFactory()
: _currentColorBrewer(0)
{
}

vrColorBrewer* vrColorBrewerFactory::chooseColorScheme(const std::string& scheme, int size)
{
	std::vector<vrColorBrewer*>::iterator cbi;

	int newSize = size;
	for(cbi = colorList.begin(); (*cbi)->label != scheme && cbi != colorList.end(); cbi++)
	{
		printf("[%s]\n", (*cbi)->label.c_str());
	}

	if (cbi == colorList.end()) 
	{	
		printf("color set not found\n");
		return 0;
	}

	// INSOO
	//if (_currentColorBrewer) delete _currentColorBrewer;

	char label[100];
	strcpy(label, scheme.c_str());

	if((*cbi)->size == 13)
	{
		if(size > COLOR_SCHEME_SEQ_END)
			newSize = COLOR_SCHEME_SEQ_END;
		else if(size < COLOR_SCHEME_START)
			newSize = COLOR_SCHEME_START;
		
		_currentColorBrewer = new vrColorBrewer(newSize, label);
		for(int i = 0; i < newSize; i++)
		{
			for(int j = 0; j < 3; j++)
				_currentColorBrewer->colorScheme[i] = (*cbi)->colorScheme[COLOR_SCHEME_ORDER_SEQ[newSize - COLOR_SCHEME_START][i]];
		}
	}
	else if((*cbi)->size == 15)
	{
		if(size > COLOR_SCHEME_DIV_END)
			newSize = COLOR_SCHEME_DIV_END;
		else if(size < COLOR_SCHEME_START)
			newSize = COLOR_SCHEME_START;

		_currentColorBrewer = new vrColorBrewer(newSize, label);

		for(int i = 0; i < newSize; i++)
		{
			_currentColorBrewer->colorScheme[i] = (*cbi)->colorScheme[COLOR_SCHEME_ORDER_DIV[newSize - COLOR_SCHEME_START][i]];
		}
	}
	else
	{
		if(size > (*cbi)->size)
			newSize = (*cbi)->size;
		else if(size < 3)
			newSize = 3;

		_currentColorBrewer = new vrColorBrewer(newSize, label);

		for(int i = 0; i < newSize; i++)
		{
			_currentColorBrewer->colorScheme[i] = (*cbi)->colorScheme[i];
		}
	}

	//this->initializeDivColorMap();

	return _currentColorBrewer;
}

void vrColorBrewerFactory::loadColorBrewerList()
{
	char label[20];
	int size;
	double a, b, c;

	FILE* file = fopen("colorbrewer.txt", "rb");
	char line[256];
	
	if (file != 0)
	{
	
		int len;
		while(!feof(file))
		{
			if (fgets(line, 256, file) == 0) break;

			len = strlen(line);
			if ((line[len - 1] == '\r') || (line[len - 1] == '\n'))
			{
				line[len - 1] = '\0';
			}

			vrColorBrewer *cbo;

			strcpy(label, line);
			if (fgets(line, 256, file) == 0) break;

			size = atoi(line);
			cbo = new vrColorBrewer(size, label);
			
			for(int i = 0; i < size; i++)
			{
				if (fgets(line, 256, file) == 0) break;
				sscanf(line, "%lf,%lf,%lf", &a, &b, &c);
				
				cbo->colorScheme[i].r = (float) a;
				cbo->colorScheme[i].g = (float) b;
				cbo->colorScheme[i].b = (float) c;
			}

			colorList.push_back(cbo);
		}

		fclose(file);
	}
	else
	{
		printf("File Not Found - ColorBrewer");
	}
}

