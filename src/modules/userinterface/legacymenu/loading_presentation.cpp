#include <algorithm>
#include <cmath>
#include <string>

#include <SDL.h>
#include <tgfclient.h>
#include <glfeatures.h>

#include "loading_presentation.h"

static void DrawLoadingRoundedRectangle(float left, float bottom, float width, float height,
    const float lowerColor[4], const float upperColor[4])
{
    const float radius = std::min(width, height) * 0.5f;
    glBegin(GL_TRIANGLE_FAN);
    glColor4f((lowerColor[0] + upperColor[0]) * 0.5f,
        (lowerColor[1] + upperColor[1]) * 0.5f,
        (lowerColor[2] + upperColor[2]) * 0.5f,
        (lowerColor[3] + upperColor[3]) * 0.5f);
    glVertex2f(left + width * 0.5f, bottom + height * 0.5f);
    for (int corner = 0; corner < 4; ++corner)
    {
        const int selectedCorner = corner % 4;
        const float centerHorizontal = left + (selectedCorner < 2 ? width - radius : radius);
        const float centerVertical = bottom + (selectedCorner == 0 || selectedCorner == 3
            ? radius : height - radius);
        for (int segment = 0; segment <= 12; ++segment)
        {
            const float angle = (-90.0f + selectedCorner * 90.0f + segment * 7.5f)
                * 0.017453292519943295f;
            const float vertical = centerVertical + std::sin(angle) * radius;
            const float fraction = (vertical - bottom) / height;
            glColor4f(lowerColor[0] + (upperColor[0] - lowerColor[0]) * fraction,
                lowerColor[1] + (upperColor[1] - lowerColor[1]) * fraction,
                lowerColor[2] + (upperColor[2] - lowerColor[2]) * fraction,
                lowerColor[3] + (upperColor[3] - lowerColor[3]) * fraction);
            glVertex2f(centerHorizontal + std::cos(angle) * radius, vertical);
        }
    }
    glColor4fv(lowerColor);
    glVertex2f(left + width - radius, bottom);
    glEnd();
}

void LoadingPresentation::load()
{
    release();
    backgroundTexture = GfTexReadTexture("data/img/pit-lane-background.png", &imageWidth,
        &imageHeight, &textureWidth, &textureHeight);
}

void LoadingPresentation::release()
{
    if (backgroundTexture)
    {
        GfTexFreeTexture(backgroundTexture);
        backgroundTexture = 0;
    }
}

void LoadingPresentation::draw(float progress, const char* message) const
{
    int screenWidth, screenHeight, viewportWidth, viewportHeight;
    GfScrGetSize(&screenWidth, &screenHeight, &viewportWidth, &viewportHeight);
    const float logicalHeight = 480.0f;
    const float logicalWidth = logicalHeight * viewportWidth / std::max(viewportHeight, 1);

    typedef void (APIENTRY *LoadingProgramSelection)(unsigned int);
    typedef void (APIENTRY *LoadingTextureSelection)(unsigned int);
    static LoadingProgramSelection selectLoadingProgram =
        reinterpret_cast<LoadingProgramSelection>(SDL_GL_GetProcAddress("glUseProgram"));
    static LoadingTextureSelection selectLoadingTexture =
        reinterpret_cast<LoadingTextureSelection>(SDL_GL_GetProcAddress("glActiveTexture"));
    if (selectLoadingProgram)
        selectLoadingProgram(0);
    if (selectLoadingTexture)
        selectLoadingTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glViewport((screenWidth - viewportWidth) / 2, (screenHeight - viewportHeight) / 2,
        viewportWidth, viewportHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, logicalWidth, 0, logicalHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    if (backgroundTexture && imageWidth > 0 && imageHeight > 0)
    {
        const float scale = std::max(logicalWidth / imageWidth, logicalHeight / imageHeight);
        const float visibleWidth = logicalWidth / scale;
        const float visibleHeight = logicalHeight / scale;
        const float left = (imageWidth - visibleWidth) * 0.5f / textureWidth;
        const float right = (imageWidth + visibleWidth) * 0.5f / textureWidth;
        const float bottom = 1.0f - (imageHeight + visibleHeight) * 0.5f / textureHeight;
        const float top = 1.0f - (imageHeight - visibleHeight) * 0.5f / textureHeight;
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, backgroundTexture);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1, 1, 1, 1);
        glBegin(GL_QUADS);
        glTexCoord2f(left, bottom); glVertex2f(0, 0);
        glTexCoord2f(right, bottom); glVertex2f(logicalWidth, 0);
        glTexCoord2f(right, top); glVertex2f(logicalWidth, logicalHeight);
        glTexCoord2f(left, top); glVertex2f(0, logicalHeight);
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);
    const float barWidth = std::min(logicalWidth * 0.34f, 340.0f);
    const float barLeft = (logicalWidth - barWidth) * 0.5f;
    const float barBottom = 234.0f;
    const float silverBottom[4] = {0.94f, 0.94f, 0.94f, 1};
    const float silverTop[4] = {0.29f, 0.29f, 0.29f, 1};
    const float trackBottom[4] = {0.17f, 0.17f, 0.17f, 1};
    const float trackTop[4] = {0.015f, 0.015f, 0.015f, 1};
    const float fillBottom[4] = {1.0f, 0.24f, 0.015f, 1};
    const float fillTop[4] = {1.0f, 0.86f, 0.12f, 1};
    DrawLoadingRoundedRectangle(barLeft, barBottom, barWidth, 12,
        silverBottom, silverTop);
    DrawLoadingRoundedRectangle(barLeft + 1.5f, barBottom + 1.5f, barWidth - 3, 9,
        trackBottom, trackTop);
    const float fraction = std::max(0.0f, std::min(progress, 1.0f));
    if (fraction > 0.0f)
    {
        DrawLoadingRoundedRectangle(barLeft + 2, barBottom + 2,
            (barWidth - 4) * fraction, 8, fillBottom, fillTop);
    }

    std::string status = message ? message : "";
    for (std::string::iterator character = status.begin(); character != status.end(); ++character)
    {
        if (*character == '\n' || *character == '\r' || *character == '\t')
            *character = ' ';
    }
    const int maximumTextWidth = static_cast<int>(logicalWidth * 0.8f);
    bool shortened = false;
    while (!status.empty() && GfuiFontWidth(GFUI_FONT_SMALL_T, status.c_str()) > maximumTextWidth)
    {
        std::size_t lastCharacter = status.size() - 1;
        while (lastCharacter > 0 && (static_cast<unsigned char>(status[lastCharacter]) & 0xc0) == 0x80)
            --lastCharacter;
        status.erase(lastCharacter);
        shortened = true;
    }
    if (shortened)
        status += "...";
    float shadow[4] = {0, 0, 0, 0.85f};
    float white[4] = {1, 1, 1, 1};
    GfuiDrawString(status.c_str(), shadow, GFUI_FONT_SMALL_T, 1, 214,
        static_cast<int>(logicalWidth), GFUI_ALIGN_HC);
    GfuiDrawString(status.c_str(), white, GFUI_FONT_SMALL_T, 0, 215,
        static_cast<int>(logicalWidth), GFUI_ALIGN_HC);
    glColor4f(1, 1, 1, 1);
    GfuiSwapBuffers();
}
