/***************************************************************************
                        guilabel.cpp -- labels management
                             -------------------
    created              : Fri Aug 13 22:22:12 CEST 1999
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

/** @file
            GUI labels management.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @ingroup	gui
*/

#include <cstdlib>
#include <cstring>

#include "gui.h"
#include "guimenu.h"

#include "portability.h"


static int NTipX = 10;
static int NTipY = 10;
static int NTipWidth = 620;
static int NTipFontId = GFUI_FONT_SMALL;
static int NTipAlign = GFUI_ALIGN_HC;


void
gfuiInitLabel(void)
{
    char path[1024];

    // Get tip layout properties from the screen config file.
    void* hparmScr = GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

    snprintf(path, sizeof(path), "%s/%s", GFSCR_SECT_MENUSETTINGS, GFSCR_SECT_TIP);
    NTipX = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_X, 0, 10.0);
    NTipY = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_Y, 0, 10.0);
    NTipWidth = (int)GfParmGetNum(hparmScr, path, GFSCR_ATT_WIDTH, 0, 620.0);
    NTipFontId = gfuiMenuGetFontId(GfParmGetStr(hparmScr, path, GFSCR_ATT_FONT, "small"));
    NTipAlign = gfuiMenuGetAlignment(GfParmGetStr(hparmScr, path, GFSCR_ATT_ALIGN, "center"));

    GfParmReleaseHandle(hparmScr);
}

/** Initialize a label
    @ingroup	gui
    @param	label	The label to initialize
    @param	text	Text to display
    @param	maxlen	Maximum length of the displayed text (used when the label is changed)
                    <br>0 for the text length.
    @param	x	Position of the label on the screen (pixels)
    @param	y	Position of the label on the screen (pixels)
    @param	width	Width of the label on the screen (pixels, for alignment maintenance)
    @param	align	Horizontal alignment:
                <br>GFUI_ALIGN_HR	right
                <br>GFUI_ALIGN_HC	center
                <br>GFUI_ALIGN_HL	left
    @param	width	Width (pixels) of the display bounding box
                    <br>0 for the actual text width (from the font specs).
    @param	font	Font id
    @param	bgColor	Pointer to static RGBA background color array
                    <br>0 for gfuiColors[GFUI_BGCOLOR]
    @param	fgColor	Pointer on static RGBA foreground color array
                    <br>0 for gfuiColors[GFUI_LABELCOLOR]
    @param	bgFocusColor	Pointer to static RGBA focused background color array
                    <br>0 for bgColor
    @param	fgFocusColor	Pointer on static RGBA focused foreground color array
                    <br>0 for fgColor
    @param	userDataOnFocus	User data given to the onFocus[Lost] call back functions
    @param	onFocus	Call back function called when getting the focus
    @param	onFocusLost	Call back function called when loosing the focus
    @return	None
 */
void
gfuiLabelInit(tGfuiLabel *label, const char *text, int maxlen,
              int x, int y, int width, int align, int font,
              const float *bgColor, const float *fgColor,
              const float *bgFocusColor, const float *fgFocusColor,
              std::string bgImgUrl,
              void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
    if (maxlen <= 0)
        maxlen = strlen(text);
    label->text = (char*)calloc(maxlen+1, sizeof(char));
    strncpy(label->text, text, maxlen);
    label->maxlen = maxlen;

    label->bgColor = GfuiColor::build(bgColor ? bgColor : gfuiColors[GFUI_BGCOLOR]);
    label->fgColor = GfuiColor::build(fgColor ? fgColor : gfuiColors[GFUI_LABELCOLOR]);
    label->bgFocusColor = bgFocusColor ? GfuiColor::build(bgFocusColor) : label->bgColor;
    label->fgFocusColor = fgFocusColor ? GfuiColor::build(fgFocusColor) : label->fgColor;

    label->font = gfuiFont[font];
    if (width <= 0)
        width = gfuiFont[font]->getWidth(text);
    label->width = width;
    label->align = align;
    label->x = x;
    label->y = y;

    label->userDataOnFocus = userDataOnFocus;
    label->onFocus = onFocus;
    label->onFocusLost = onFocusLost;

    if (!bgImgUrl.empty()){
		label->bgImg = GfTexReadTexture(bgImgUrl.c_str());
	}
}

/** Create a new label
    @ingroup	gui
    @param	scr	Screen where to add the label
    @param	text	Text of the label
    @param	font	Font id
    @param	x	Position of the label on the screen
    @param	y	Position of the label on the screen
    @param	align	Horizontal alignment:
                <br>GFUI_ALIGN_HR	right
                <br>GFUI_ALIGN_HC	center
                <br>GFUI_ALIGN_HL	left
    @param	maxlen	Maximum length of the button string (used when the label is changed)
                <br>0 for the text length.
    @param	fgColor	Pointer on static RGBA color array (0 => default)
    @param	fgFocusColor	Pointer on static RGBA focused color array (0 => fgColor)
    @param	userDataOnFocus	User data given to the onFocus[Lost] call back functions
    @param	onFocus	Call back function called when getting the focus
    @param	onFocusLost	Call back function called when loosing the focus
    @return	label Id
    @see	GfuiSetLabelText
 */
int
GfuiLabelCreate(void *scr, const char *text, int font, int x, int y,
                int width, int align, int maxlen,
                const float *fgColor, const float *fgFocusColor,
                std::string backgrounImageUrl,
                int bgImgPaddingTop,
                int bgImgPaddingBottom,
                int bgImgPaddingLeft,
                int bgImgPaddingRight,
                void *userDataOnFocus, tfuiCallback onFocus, tfuiCallback onFocusLost)
{
    tGfuiLabel	*label;
    tGfuiObject	*object;
    tGfuiScreen	*screen = (tGfuiScreen*)scr;

    object = (tGfuiObject*)calloc(1, sizeof(tGfuiObject));
    object->widget = GFUI_LABEL;
    object->focusMode = (onFocus || onFocusLost) ? GFUI_FOCUS_MOUSE_MOVE : GFUI_FOCUS_NONE;
    object->visible = 1;
    object->id = screen->curId++;

    label = &(object->u.label);
    gfuiLabelInit(label, text, maxlen, x, y, width, align, font,
                  screen->bgColor.toFloatRGBA(), fgColor,
                  screen->bgColor.toFloatRGBA(), fgFocusColor,
                  backgrounImageUrl,
                  userDataOnFocus, onFocus, onFocusLost);

    label->bgImgPaddingTop = bgImgPaddingTop;
    label->bgImgPaddingBottom = bgImgPaddingBottom;
    label->bgImgPaddingLeft = bgImgPaddingLeft;
    label->bgImgPaddingRight = bgImgPaddingRight;

    width = label->width;
    const int height = gfuiFont[font]->getHeight();

    object->xmin = x;
    object->ymin = y;

    object->xmax = object->xmin + width;
    object->ymax = object->ymin + height;

    gfuiAddObject(screen, object);

    return object->id;
}

/** Add a Tip (generally associated with a button).
    @param	scr	Screen where to add the label
    @param	text	Text of the label
    @param	maxlen	Maximum length of the button string (used when the label is changed)
    @return	label Id
    @see	GfuiSetLabelText
 */
int
GfuiTipCreate(void *scr, const char *text, int maxlen)
{
    return GfuiLabelCreate(scr, text, NTipFontId, NTipX, NTipY, NTipWidth,
                           NTipAlign, maxlen, gfuiColors[GFUI_TIPCOLOR]);
}

/** Change the text of a label.
    @ingroup	gui
    @param	label	Label itself
    @param	obj	Label object
    @param	text	Text of the label
    @warning	The maximum length is set at the label creation
    @see	GfuiAddLabel
 */
void
gfuiLabelSetText(tGfuiLabel *label, const char *text)
{
    if (!text)
        return;

    free(label->text);
    label->maxlen = strlen(text);
    label->text = (char*)calloc(label->maxlen+1, sizeof(char));

    // Update the text.
    strncpy(label->text, text, label->maxlen);
}

/** Change the text of a label.
    @ingroup	gui
    @param	scr	Screen of the label
    @param	id	Id of the label
    @param	text	Text of the label
    @warning	The maximum length is set at the label creation
    @see	GfuiAddLabel
 */
void
GfuiLabelSetText(void *scr, int id, const char *text)
{
    tGfuiObject* object = gfuiGetObject(scr, id);

    if (object && object->widget == GFUI_LABEL)
    {
        tGfuiLabel* label = &(object->u.label);

        // Update the text.
        gfuiLabelSetText(label, text);
    }
}

/** Change the color of a label.
    @ingroup	gui
    @param	label	Label to modify
    @param	color	an array of 4 floats (RGBA)
    @see	GfuiAddLabel
 */
void
gfuiLabelSetColor(tGfuiLabel *label, const float *color)
{
    label->fgColor = GfuiColor::build(color);
}

/** Change the color of a label object.
    @ingroup	gui
    @param	scr	Screen where to add the label
    @param	id	Id of the label
    @param	color	an array of 4 floats (RGBA)
    @see	GfuiAddLabel
 */
void
GfuiLabelSetColor(void *scr, int id, const float* color)
{
    tGfuiObject* object = gfuiGetObject(scr, id);

    if (object && object->widget == GFUI_LABEL)
        gfuiLabelSetColor(&object->u.label, color);
}

static std::string
gfuiGetMaskedString(const char *text)
{
    std::string ret;
    size_t n = strlen(text);

    ret.reserve(n);

    for (size_t i = 0; i < strlen(text); i++)
        ret += '*';

    return ret;
}

std::string
gfuiLabelGetText(const tGfuiLabel *label)
{
    std::string maskedtext;

    if (label->masked)
        maskedtext = gfuiGetMaskedString(label->text);

    return label->masked ? maskedtext : label->text;
}

/** Determine the actual text X coordinate.
    @ingroup	gui
    @param	obj	label to query
    @return	X coordinate of the text
 */
int
gfuiLabelGetTextX(const tGfuiLabel *label)
{
    std::string text = gfuiLabelGetText(label);
    int offset = label->width - label->font->getWidth(text.c_str());

    switch (label->align & GFUI_ALIGN_HMASK)
    {
        default:
        case GFUI_ALIGN_HL:
            break;

        case GFUI_ALIGN_HC:
            return label->x + offset / 2;

        case GFUI_ALIGN_HR:
            return label->x + offset;
    }

    return label->x;
}

static void
gfuiGetTokens(const std::string &s, std::vector<std::string> &v)
{
    std::string::size_type pos = 0;

    for (;;)
    {
        std::string::size_type del = s.find("\t", pos);

        v.push_back(s.substr(pos, del - pos));
        pos = del + 1;

        if (del == std::string::npos)
            break;
    }
}

/** Actually draw the given label.
    @ingroup	gui
    @param	obj	label to draw
 */
void
gfuiLabelDraw(tGfuiLabel *label, const GfuiColor& color)
{
    std::string text = gfuiLabelGetText(label);
    std::vector<std::string> tokens;
    int offset = 0;

    gfuiGetTokens(text, tokens);

    for (const auto &t : tokens)
    {
        int x = gfuiLabelGetTextX(label) + offset;
        // Select the right color from the state/focus.
        glColor4fv(color.toFloatRGBA());
        gfuiDrawString(x, label->y, label->font, t.c_str());
        offset += label->font->getWidth("o") * t.size();
    }
}

/** Actually draw the given label object.
    @ingroup	gui
    @param	obj	label object to draw
 */
void
gfuiDrawLabel(tGfuiObject *obj)
{
    tGfuiLabel *label = &(obj->u.label);

    // Draw the background if visible.
    if (label->bgColor.alpha)
    {
        glColor4fv(obj->focus ? label->bgFocusColor.toFloatRGBA() : label->bgColor.toFloatRGBA());
        glBegin(GL_QUADS);
        glVertex2i(obj->xmin, obj->ymin);
        glVertex2i(obj->xmin, obj->ymax);
        glVertex2i(obj->xmax, obj->ymax);
        glVertex2i(obj->xmax, obj->ymin);
        glEnd();
    }

    //draw the bg image
	if (label->bgImg)
	{
		const int x1 = obj->xmin - label->bgImgPaddingLeft;
		const int x2 = obj->xmax + label->bgImgPaddingRight;
		const int y1 = obj->ymin - label->bgImgPaddingBottom;
		const int y2 = obj->ymax + label->bgImgPaddingTop;

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor3f(1.0, 1.0, 1.0); //set color to mix with image

		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindTexture(GL_TEXTURE_2D, label->bgImg);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBegin(GL_QUADS);

		glTexCoord2f(0.0, 0.0);
		glVertex2i(x1, y1);

		glTexCoord2f(0.0, 1.0);
		glVertex2i(x1, y2);

		glTexCoord2f(1.0, 1.0);
		glVertex2i(x2, y2);

		glTexCoord2f(1.0, 0.0);
		glVertex2i(x2, y1);

		glEnd();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

    // Draw the label text itself.
    gfuiLabelDraw(label, obj->focus ? label->fgFocusColor : label->fgColor);
}

void
gfuiReleaseLabel(tGfuiObject *obj)
{
    tGfuiLabel	*label;

    label = &(obj->u.label);

    free(label->text);
	GfTexFreeTexture(label->bgImg);
    free(obj);

}
