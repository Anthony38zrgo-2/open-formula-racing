#ifndef OPEN_FORMULA_LOADING_PRESENTATION_H
#define OPEN_FORMULA_LOADING_PRESENTATION_H

class LoadingPresentation
{
public:
    void load();
    void release();
    void draw(float progress, const char* message) const;

private:
    unsigned int backgroundTexture = 0;
    int imageWidth = 0;
    int imageHeight = 0;
    int textureWidth = 0;
    int textureHeight = 0;
};

#endif
