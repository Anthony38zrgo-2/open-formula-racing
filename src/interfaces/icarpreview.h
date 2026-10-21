#ifndef OPEN_FORMULA_ICARPREVIEW_H
#define OPEN_FORMULA_ICARPREVIEW_H

struct CarElt;

// Optional capability: do not change the IGraphicsEngine vtable or require
// other renderers to implement an editor-only feature.
class ICarPreviewRenderer
{
public:
    virtual ~ICarPreviewRenderer() {}
    virtual bool drawGarageCarPreview(CarElt* car, int x, int y, int width,
        int height, bool sideView) = 0;
    virtual void resetGarageCarPreview() = 0;
};
#endif
