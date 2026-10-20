/***************************************************************************
                         guifont.cpp -- GLTT fonts management
                             -------------------
    created              : Fri Aug 13 22:19:09 CEST 1999
    copyright            : (C) 1999 by Eric Espie
    email                : torcs@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* This font manipulation is based on Brad Fish's glFont format and code.  */
/* http://www.netxs.net/bfish/news.html                                    */

#include <portability.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_opengl.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <machine/endian.h>
#endif

#ifdef LINUX
#include <endian.h>
#endif

#include "tgfclient.h"

#include "guifont.h"


static char buf[1024];
/* Rendering fonts with their original size and resolution makes them look
 * blurry. To solve this, fonts are rendered with a larger font size. Then,
 * the OpenGL quads are reduced by this factor to compensate. */
static const float scale_factor = 2.0;

GfuiFontClass *gfuiFont[GFUI_FONT_NB];
const char *keySize[4] = { "size big", "size large", "size medium", "size small" };


#if !defined(_WIN32) && !defined(__HAIKU__)
#if BYTE_ORDER == BIG_ENDIAN
void swap32(unsigned int *p, unsigned int size)
{
	unsigned int i, t;
	for (i = 0; i < size; i += 4) {
		t = (unsigned int) *p;
		*p = (t & 0xff000000U) >> 24;
		*p |= (t & 0x00ff0000U) >> 8;
		*p |= (t & 0x0000ff00U) << 8;
		*p |= (t & 0x000000ffU) << 24;
		p++;
	}
}
#endif
#endif

void gfuiFreeFonts(void)
{
	GfuiFontClass* font;
	for ( int I = 0; I < GFUI_FONT_NB; I++)
	{
		font = gfuiFont[I];
		delete font;
	}

	TTF_Quit();
}

void gfuiLoadFonts(void)
{
	void *param;
	int	size;
	int	i;
	int nFontId;

	if (TTF_Init())
		throw std::runtime_error("TTF_Init failed: "
			+ std::string(TTF_GetError()));

	param = GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	snprintf(buf, sizeof(buf), "%sdata/fonts/%s", GfDataDir(),
			 GfParmGetStr(param, "Menu Font TrueType", "name",
				"Jura/Jura.ttf"));
	GfLogTrace("Loading font 'Menu Font' from %s : Sizes", buf);
	nFontId = GFUI_FONT_BIG;
	for(i = 0; i < 4; i++, nFontId++) {
		size = (int)GfParmGetNum(param, "Menu Font TrueType", keySize[i],
			(char*)NULL, 10.0);
		GfLogTrace(" %d,", size);
		gfuiFont[nFontId] = new GfuiFontClass(buf, size);
	}
	GfLogTrace("\n");

	snprintf(buf, sizeof(buf), "%sdata/fonts/%s", GfDataDir(),
			 GfParmGetStr(param, "Console Font TrueType", "name",
				"LiberationSans/LiberationSans-Bold.ttf"));
	GfLogTrace("Loading font 'Console Font' from %s : Sizes", buf);
	nFontId = GFUI_FONT_BIG_C;
	for(i = 0; i < 4; i++, nFontId++) {
		size = (int)GfParmGetNum(param, "Console Font TrueType", keySize[i],
			(char*)NULL, 10.0);
		GfLogTrace(" %d,", size);
		gfuiFont[nFontId] = new GfuiFontClass(buf, size);
	}
	GfLogTrace("\n");

	snprintf(buf, sizeof(buf), "%sdata/fonts/%s", GfDataDir(),
			 GfParmGetStr(param, "Text Font TrueType", "name",
				"DroidSans/DroidSans.ttf"));
	GfLogTrace("Loading font 'Text Font' from %s : Sizes", buf);
	nFontId = GFUI_FONT_BIG_T;
	for(i = 0; i < 4; i++, nFontId++) {
		size = (int)GfParmGetNum(param, "Text Font TrueType", keySize[i],
			(char*)NULL, 10.0);
		GfLogTrace(" %d,", size);
		gfuiFont[nFontId] = new GfuiFontClass(buf, size);
	}
	GfLogTrace("\n");

	GfParmReleaseHandle(param);
}

GfuiFontClass::GfuiFontClass(const char *FileName, int size) :
	font(TTF_OpenFont(FileName, size))
{
	if (!font)
		throw std::runtime_error("TTF_OpenFont failed: "
			+ std::string(TTF_GetError()));

	TTF_SetFontKerning(font, 1);
}

GfuiFontClass::~GfuiFontClass()
{
	TTF_CloseFont(font);
}

int GfuiFontClass::getHeight() const
{
	int w, h;

	if (TTF_SizeUTF8(font, "X", &w, &h))
		return -1;

	return h / scale_factor;
}

int GfuiFontClass::getWidth(const char* text) const
{
	int w, h;

	if (TTF_SizeUTF8(font, text, &w, &h))
		return -1;

	return w / scale_factor;
}

int GfuiFontClass::drawString(int X, int Y, const char* text,
	const float *color) const
{
	Uint8 r, g, b;
	GLuint texture;

	if (!strlen(text))
		return 0;
	else if (color) {
		r = static_cast<Uint8>(255.0 * color[0]);
		g = static_cast<Uint8>(255.0 * color[1]);
		b = static_cast<Uint8>(255.0 * color[2]);
	}
	else
		r = g = b = 255;

	SDL_Color fg = {r, g, b};
	SDL_Surface *psurface = TTF_RenderUTF8_Blended(font, text, fg);

	if (!psurface) {
		GfLogError("TTF_RenderUTF8_Solid failed: %s\n", TTF_GetError());
		return -1;
	}

	Uint32 rmask, gmask, bmask, amask = 0xff000000;

	if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
		rmask = 0x00ff0000;
		gmask = 0x0000ff00;
		bmask = 0x000000ff;
	} else {
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
	}

	SDL_Surface *surface = SDL_CreateRGBSurface(0, psurface->w, psurface->h,
		32, rmask, gmask, bmask, amask);

	if (!surface) {
		GfLogError("SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
		SDL_FreeSurface(psurface);
		return -1;
	}

  	SDL_BlitSurface(psurface, NULL, surface, NULL);

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0,
		GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);

	float realw = surface->w / scale_factor,
		realh = surface->h / scale_factor;

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(X, Y + realh);
	glTexCoord2f(1, 0); glVertex2f(X + realw, Y + realh);
	glTexCoord2f(1, 1); glVertex2f(X + realw, Y);
	glTexCoord2f(0, 1); glVertex2f(X, Y);
	glEnd();

	glDeleteTextures(1, &texture);
	SDL_FreeSurface(surface);
	SDL_FreeSurface(psurface);
	return 0;
}
