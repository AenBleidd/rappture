#include <GL/glew.h>
#include <GL/gl.h>
#include "Grid.h"

Grid::Grid()
: _axisColor(1.0f, 1.0f, 1.0f), _font(0), _axisGridLineCount(5.0f, 5.0f, 5.0f),
_axisMin(0.0f, 0.0f, 0.0f), _axisMax(1.0f, 1.0f, 1.0f), _gridLineColor(1.0f, 1.0f, 1.0f), _visible(false)
{
	__axisScale = _axisMax - _axisMin;
    _axisName[0] = "X";
    _axisName[1] = "Y";
    _axisName[2] = "Z";
}

void Grid::render()
{
	glPushMatrix();
	glEnable(GL_DEPTH_TEST);

	//glTranslatef(0.5f, 0.5f, 0.5f);
	glScalef(1.0f, __axisScale.y / __axisScale.x, __axisScale.z / __axisScale.x);
	glTranslatef(-0.5f, -0.5f, -0.5f);
	glLineWidth(2.0f);
    glColor3f(_axisColor.x, _axisColor.y, _axisColor.z);

	glBegin(GL_LINES);
		glVertex3f(0.0f,0.0f,0.0f);
		glVertex3f(1.2f,0.0f,0.0f);
		glVertex3f(0.0f,0.0f,0.0f);
		glVertex3f(0.0f,1.2f,0.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, 1.2f);
	glEnd();

    glDisable(GL_TEXTURE_2D);
	glLineWidth(1.0f);
	glEnable(GL_BLEND);
    glColor3f(_gridLineColor.x, _gridLineColor.y, _gridLineColor.z);
	glBegin(GL_LINES);

	float linecount = 2.0f;
	float linecountY = 4.0f;
		// +y
		for (int i = 1; i <= (int) _axisGridLineCount.y; ++i)
		{
			glVertex3f(0.0f, i / _axisGridLineCount.y, 0.0f);
			glVertex3f(1.1f, i / _axisGridLineCount.y, 0.0f);
		}
		
		for (int i = 1; i <= (int) _axisGridLineCount.x; ++i)
		{
			glVertex3f(i / _axisGridLineCount.x, 0.0f, 0.0f);
			glVertex3f(i / _axisGridLineCount.x, 1.0f, 0.0f);
		}

		for (int i = 1; i <= (int) _axisGridLineCount.z; ++i)
		{
			glVertex3f(0.0f, 0.0f, i / _axisGridLineCount.z);
			glVertex3f(0.0f, 1.0f, i / _axisGridLineCount.z);
		}

		for (int i = 1; i <= (int) _axisGridLineCount.y; ++i)
		{
			glVertex3f(0.0f, i / _axisGridLineCount.y, 0.0f);
			glVertex3f(0.0f, i / _axisGridLineCount.y, 1.0f);
		}

		for (int i = 1; i <= (int) _axisGridLineCount.x; ++i)
		{
			glVertex3f(i / _axisGridLineCount.x,0.0f, 0.0f);
			glVertex3f(i / _axisGridLineCount.x,0.0f, 1.1f);
		}

		for (int i = 1; i <= (int) _axisGridLineCount.z; ++i)
		{
			glVertex3f(0.0f, 0.0f, i / _axisGridLineCount.z);
			glVertex3f(1.1f, 0.0f, i / _axisGridLineCount.z);
		}
	glEnd();

	if (_font)
	{
		double x = 1.0f, y = 0.0f, z = 0.0f;
		double mv[16], prjm[16];
		int viewport[4];
		double winx, winy, winz;
		glGetDoublev(GL_MODELVIEW_MATRIX, mv);
		glGetDoublev(GL_PROJECTION_MATRIX, prjm);
		glGetIntegerv(GL_VIEWPORT, viewport);

		
		_font->begin();
		if (gluProject(1.2, 0.0, 0.0, mv, prjm, viewport, &winx, &winy, &winz))
		{
			glLoadIdentity();
			glTranslatef((int) winx, viewport[3] - (int) winy, 0);
			_font->draw((const char*) _axisName[0]);
		}
				
		if (gluProject(0.0, 1.2, 0.0, mv, prjm, viewport, &winx, &winy, &winz))
		{
			glLoadIdentity();
			glTranslatef((int) winx, viewport[3] - (int)winy, 0);
			_font->draw((const char*) _axisName[1]);
		}

		if (gluProject(0.0, 0.0, 1.2, mv, prjm, viewport, &winx, &winy, &winz))
		{
			glLoadIdentity();
			glTranslatef((int) winx, (int) viewport[3] - (int)winy, 0.0f);
			_font->draw((const char*) _axisName[2]);
		}

		glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
		char buff[20];
		// Y
		for (int i = 1; i <= (int) _axisGridLineCount.y; ++i)
		{
			if (gluProject(1.2, i / _axisGridLineCount.y, 0.0f, mv, prjm, viewport, &winx, &winy, &winz))
			{
				glLoadIdentity();
				glTranslatef((int) winx, (int) viewport[3] - (int)winy, 0.0f);
				sprintf(buff, "%0.2f", __axisScale.y / _axisGridLineCount.y * i);
				_font->draw(buff);
			}
		}

		for (int i = 1; i <= (int) _axisGridLineCount.z; ++i)
		{
			if (gluProject(1.2, 0.0, i / _axisGridLineCount.z , mv, prjm, viewport, &winx, &winy, &winz))
			{
				glLoadIdentity();
				glTranslatef((int) winx, (int) viewport[3] - (int)winy, 0.0f);
				sprintf(buff, "%0.2f", __axisScale.z / _axisGridLineCount.z * i);
				_font->draw(buff);
			}
		}

		for (int i = 1; i <= (int) _axisGridLineCount.x; ++i)
		{
			if (gluProject( i / _axisGridLineCount.x, 0.0f, 1.2, mv, prjm, viewport, &winx, &winy, &winz))
			{
				glLoadIdentity();
				glTranslatef((int) winx, (int) viewport[3] - (int)winy, 0.0f);
				sprintf(buff, "%0.2f", __axisScale.x / _axisGridLineCount.x * i);
				_font->draw(buff);
			}
		}

		_font->end();
	};

	glPopMatrix();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}
 
void Grid::setFont(R2Fonts* font)
{
//	if (font == _font) return;
//	if (font) font->addRef();
//	if (_font) _font->unrefDelete();
	
	_font = font;
}
