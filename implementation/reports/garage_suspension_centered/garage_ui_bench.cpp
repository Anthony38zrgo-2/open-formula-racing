
#define SDL_MAIN_HANDLED
#include "OsgGaragePreview.h"
#include "OsgLoader.h"
#include <osg/MatrixTransform>
#include <osg/Switch>
#include <tgfclient.h>
#include <car.h>
#include <raceman.h>
#include <icarpreview.h>
#include <igraphicsengine.h>
#include <iphysicsengine.h>
#include "formula_garage_music.h"
#include <guifont.h>
#include <SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
static const float scale_factor = 2.0;
GfuiFontClass* gfuiFont[GFUI_FONT_NB];
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
void PreviewDrawString(const char *text, float *fgColor, int font,
					int x, int y, int width, int hAlign)
{
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1) ;
	glColor4fv(fgColor);

	switch(hAlign & GFUI_ALIGN_HMASK) {
		case GFUI_ALIGN_HL:
			gfuiFont[font]->drawString(x, y, text, fgColor);
			break;
		case GFUI_ALIGN_HC:
			gfuiFont[font]->drawString(x + (width - gfuiFont[font]->getWidth(text)) / 2, y, text, fgColor);
			break;
		case GFUI_ALIGN_HR:
			gfuiFont[font]->drawString(x + width - gfuiFont[font]->getWidth(text), y, text, fgColor);
			break;
	}

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}

constexpr double radiansPerDegree = 0.017453292519943295;
constexpr double garagePositionX = 940.04363594;
constexpr double garagePositionY = 511.07507842;
constexpr double garageGroundHeight = 33.21259947;
constexpr double garageHeading = -0.0066;
// Native rendering of implementation/ui/formula-garage/reference.html.
float turquoise[4] = {0.3765f, 0.4941f, 0.5686f, 1.0f};
float white[4] = {0.9137f, 0.9255f, 0.9098f, 1.0f};
float gold[4] = {0.8902f, 0.6980f, 0.2353f, 1.0f};
float graphite[4] = {0.0902f, 0.1059f, 0.1216f, 1.0f};
float panelColor[4] = {0.116f, 0.141f, 0.162f, 1.0f};
float muted[4] = {0.66f, 0.71f, 0.73f, 1.0f};
constexpr int editorWidth = 1280;
constexpr int editorHeight = 800;
void* garageScreen = nullptr;
tCarElt* garageCar = nullptr;
bool initialArrivalPending = false;
bool arrivalArmed = false;
bool garageOpen = false;
bool editorOpen = false;
bool symmetricFront = true;
bool symmetricRear = true;
bool replaceTires = false;
bool rustGarage = false;
int selectedPage = 0;
int selectedControl = 0;
int draggedControl = -1;
int mousePositionX = 0;
int mousePositionY = 0;
int viewportLeft = 0;
int viewportBottom = 0;
int viewportWidth = 640;
int viewportHeight = 480;
std::string statusMessage;
SDL_GameController* menuController = nullptr;
unsigned previousControllerButtons = 0;
unsigned lastControllerEvent = 0;

struct GarageTexture
{
    unsigned identifier = 0;
    int width = 0;
    int height = 0;
    int paddedWidth = 0;
    int paddedHeight = 0;
};
std::map<std::string, GarageTexture> textures;
std::map<std::string, GarageTexture> imageTextures;
std::map<int, TTF_Font*> garageFonts;
FormulaGarageMusic garageMusic;

struct GarageField
{
    std::string name;
    std::string unit;
    tCarSetupItem* target = nullptr;
    double value = 0.0;
    double original = 0.0;
    double minimum = 0.0;
    double maximum = 1.0;
    double step = 1.0;
    double scale = 1.0;
    int wheel = -1;
    int kind = 0;
    std::string path;
    bool editable = true;
    int group = 0;
};
struct GaragePage
{
    std::string title;
    std::string image;
    std::string icon;
    std::vector<int> fields;
    bool vertical;
    std::string information;
};
struct GarageControl
{
    double left, bottom, width, height;
    std::string text;
    int field;
    bool vertical;
    std::function<void()> action;
};
std::vector<GarageField> fields;
std::vector<GaragePage> pages;
std::vector<GarageControl> controls;


class PreviewRenderer : public ICarPreviewRenderer {
public:
    osggraph::GarageCarPreview mesh;
    int topCalls=0, sideCalls=0;
    bool drawGarageCarPreview(CarElt*,int x,int y,int w,int h,bool side) override {
        if(side) ++sideCalls; else ++topCalls;
        return mesh.draw(x,y,w,h,side);
    }
    void resetGarageCarPreview() override { mesh.clear(); }
};
static PreviewRenderer previewRenderer;
class LegacyMenu {
public:
    static LegacyMenu& self() { static LegacyMenu menu; return menu; }
    ICarPreviewRenderer* graphicsEngine() { return &previewRenderer; }
};
static tRmInfo previewRace{};
struct PreviewEngine { tRmInfo* inData() { return &previewRace; } IPhysicsEngine* garagePhysics() { return nullptr; } };
PreviewEngine& LmRaceEngine() { static PreviewEngine engine; return engine; }
static int applied = 0, cancelled = 0;
void ApplyEditor() { ++applied; }
void CancelEditor() { ++cancelled; }
void OpenEditor() {}
void BeginDriving() {}
static int previewScreenWidth=1280, previewScreenHeight=800;
void PreviewScreenSize(int* sw,int* sh,int* vw,int* vh) {
    *sw=*vw=previewScreenWidth; *sh=*vh=previewScreenHeight;
}
void Rectangle(double left, double bottom, double width, double height, const float* color)
{
    glDisable(GL_TEXTURE_2D);
    glColor4fv(color);
    glBegin(GL_QUADS);
    glVertex2d(left, bottom); glVertex2d(left + width, bottom);
    glVertex2d(left + width, bottom + height); glVertex2d(left, bottom + height);
    glEnd();
}
void Outline(double left, double bottom, double width, double height, const float* color)
{
    glDisable(GL_TEXTURE_2D);
    glColor4fv(color);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(left, bottom); glVertex2d(left + width, bottom);
    glVertex2d(left + width, bottom + height); glVertex2d(left, bottom + height);
    glEnd();
}
void ClearGarageText()
{
    for (const auto& entry : textures)
        if (entry.second.identifier) glDeleteTextures(1, &entry.second.identifier);
    textures.clear();
}
void Text(const std::string& text, double horizontal, double vertical, int font = GFUI_FONT_SMALL,
    float* color = turquoise, int alignment = GFUI_ALIGN_HC, double maximumWidth = 0)
{
    if (editorOpen && !text.empty())
    {
        if (!garageFonts.count(font))
        {
            const char* file = font == GFUI_FONT_BIG || font == GFUI_FONT_MEDIUM
                ? "Libron-Bold.ttf" : "Libron-Regular.ttf";
            const int size = font == GFUI_FONT_BIG ? 36 : font == GFUI_FONT_MEDIUM ? 19 : 15;
            const std::string path = std::string(GfDataDir()) + "data/fonts/Libron/" + file;
            garageFonts[font] = TTF_OpenFont(path.c_str(), size * 2);
            if (!garageFonts[font]) GfLogWarning("Garage font unavailable: %s; using menu font\n", path.c_str());
        }
        if (garageFonts[font])
        {
            // Cache white glyph textures; palette changes use GL modulation.
            // Bound the cache because dragging sliders creates new strings.
            if (textures.size() >= 512) ClearGarageText();
            const std::string key = std::to_string(font) + ":" + text;
            if (!textures.count(key))
            {
                SDL_Color ink = {255, 255, 255, 255};
                SDL_Surface* rendered = TTF_RenderUTF8_Blended(garageFonts[font], text.c_str(), ink);
                SDL_Surface* rgba = rendered ? SDL_ConvertSurfaceFormat(rendered, SDL_PIXELFORMAT_RGBA32, 0) : nullptr;
                if (rendered) SDL_FreeSurface(rendered);
                if (!rgba) return;
                GarageTexture texture;
                texture.width = rgba->w; texture.height = rgba->h;
                glGenTextures(1, &texture.identifier);
                glBindTexture(GL_TEXTURE_2D, texture.identifier);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
                SDL_FreeSurface(rgba);
                textures[key] = texture;
            }
            const GarageTexture& texture = textures[key];
            const double fit = maximumWidth > 0 ? std::min(1.0, maximumWidth / (texture.width * .5)) : 1.0;
            const double width = texture.width * .5 * fit, height = texture.height * .5 * fit;
            double x = horizontal;
            if (alignment == GFUI_ALIGN_HC) x -= width * .5;
            else if (alignment == GFUI_ALIGN_HR) x -= width;
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, texture.identifier);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glColor4fv(color);
            glBegin(GL_QUADS);
            glTexCoord2d(0, 1); glVertex2d(x, vertical);
            glTexCoord2d(1, 1); glVertex2d(x + width, vertical);
            glTexCoord2d(1, 0); glVertex2d(x + width, vertical + height);
            glTexCoord2d(0, 0); glVertex2d(x, vertical + height);
            glEnd();
            glDisable(GL_TEXTURE_2D);
            return;
        }
    }
    PreviewDrawString(text.c_str(), color, font, (int)horizontal, (int)vertical, 0, alignment);
}
void TabImage(const std::string& name, double x, double y, double width, double height)
{
    if (!imageTextures.count(name))
    {
        GarageTexture texture;
        const std::string path = std::string(GfDataDir()) + "data/img/formula-garage/" + name;
        // UI silhouettes use authored display colors, not scene/photograph
        // gamma correction. Keep their gray consistent with the clay model.
        unsigned char* pixels = GfTexReadImageFromPNG(path.c_str(), 1.0f,
            &texture.width, &texture.height, &texture.paddedWidth, &texture.paddedHeight, false);
        if (pixels)
        {
            glGenTextures(1, &texture.identifier);
            glBindTexture(GL_TEXTURE_2D, texture.identifier);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.paddedWidth,
                texture.paddedHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            std::free(pixels);
        }
        imageTextures[name] = texture;
    }
    const GarageTexture& texture = imageTextures[name];
    if (!texture.identifier || texture.width <= 0 || texture.height <= 0) return;
    const double scale = std::min(width / texture.width, height / texture.height);
    const double w = texture.width * scale, h = texture.height * scale;
    x += (width - w) / 2; y += (height - h) / 2;
    const double u = (double)texture.width / texture.paddedWidth;
    const double v = 1.0 - (double)texture.height / texture.paddedHeight;
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texture.identifier);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2d(0, v); glVertex2d(x, y);
    glTexCoord2d(u, v); glVertex2d(x + w, y);
    glTexCoord2d(u, 1); glVertex2d(x + w, y + h);
    glTexCoord2d(0, 1); glVertex2d(x, y + h);
    glEnd(); glDisable(GL_TEXTURE_2D);
}
std::string Number(double value, const std::string& unit)
{
    char result[100];
    const bool integral = unit == "rpm" || unit == "N/m" || unit == "N.s/m";
    std::snprintf(result, sizeof(result), integral ? "%.0f %s" : "%.2f %s", value, unit.c_str());
    return result;
}
void AddButton(double left, double bottom, double width, double height,
    const std::string& text, const std::function<void()>& action)
{
    controls.push_back({left, bottom, width, height, text, -1, false, action});
}
void AddField(GaragePage& page, const std::string& name, const std::string& unit,
    tCarSetupItem* target, double minimum, double maximum, double step, double scale = 1.0,
    int wheel = -1, int kind = 0, double rawValue = 0.0)
{
    GarageField field;
    field.name = name; field.unit = unit; field.target = target;
    field.value = target ? target->value * scale : rawValue;
    field.original = field.value;
    field.minimum = std::min(minimum, field.value);
    field.maximum = std::max(maximum, field.value);
    field.step = step; field.scale = scale; field.wheel = wheel; field.kind = kind;
    page.fields.push_back((int)fields.size());
    fields.push_back(field);
}
void BuildPages()
{
    fields.clear(); pages.clear();
    rustGarage = false;
    std::vector<PhysicsSetupField> live;
    IPhysicsEngine* physics = LmRaceEngine().garagePhysics();
    if (physics && physics->ownsGarageSetup())
    {
        rustGarage = true;
        if (!physics->garageFields(garageCar->index, live)) {
            statusMessage = "Rust setup unavailable; vehicle configuration failed";
            return;
        }
        for (const PhysicsSetupField& source : live) {
            GarageField field;
            field.name = source.label; field.unit = source.unit; field.path = source.path;
            field.scale = source.unit == "rad" ? 1.0 / radiansPerDegree : 1.0;
            if (source.unit == "rad") field.unit = "deg";
            field.value = field.original = source.value * field.scale;
            field.minimum = source.minimum * field.scale; field.maximum = source.maximum * field.scale;
            field.step = source.step * field.scale; field.editable = source.editable;
            field.group = source.path.find("suspension.") == 0 ? 0 :
                ((source.path.find("aero.") == 0 || source.path.find("tires.") == 0) ? 1 :
                ((source.path.find("powertrain.") == 0 || source.path.find("steering.") == 0) ? 2 : 3));
            fields.push_back(field);
        }
        return;
    }
    tCarSetup& setup = garageCar->setup;
    const char* cornerNames[4] = {"Front right", "Front left", "Rear right", "Rear left"};
    GaragePage page;
    page = {"Downforce", "SimuFondAero.png", "Aero", {}, false, "Wing incidence, degrees"};
    for (int axle = 0; axle < 2; ++axle)
        AddField(page, axle == 0 ? "Front" : "Rear", "deg", &setup.wingAngle[axle], 0, 40, 0.5, 1.0 / radiansPerDegree);
    pages.push_back(page);
    struct FourWheelPage { const char* title; const char* image; const char* icon; tCarSetupItem* targets; double minimum, maximum, step, scale; const char* unit; };
    const FourWheelPage fourWheelPages[] = {
        {"Ride height", "SimuFondHeight.png", "Height", setup.rideHeight, 30, 100, 1, 1000, "mm"},
        {"Camber", "SimuFondCar.png", "Car", setup.camber, -10, 0, 0.1, 1.0/radiansPerDegree, "deg"},
        {"Toe", "SimuFondCar.png", "Par", setup.toe, -2, 2, 0.05, 1.0/radiansPerDegree, "deg"},
        {"Bump stops", "SimuFondButee.png", "Stops", setup.suspPacker, 0, 90, 1, 1000, "mm"},
        {"Springs", "SimuFondSprings.png", "Springs", setup.suspSpring, 50000, 2000000, 10000, 1, "N/m"},
        {"Dampers (rebound)", "SimuFondDampers.png", "Rebound", setup.suspSlowRebound, 100, 40000, 100, 1, "N.s/m"},
        {"Dampers (bump)", "SimuFondDampers.png", "Bump", setup.suspSlowBump, 100, 40000, 100, 1, "N.s/m"},
        {"Fast rebound", "SimuFondDampers.png", "Rebound", setup.suspFastRebound, 100, 40000, 100, 1, "N.s/m"},
        {"Fast bump", "SimuFondDampers.png", "Bump", setup.suspFastBump, 100, 40000, 100, 1, "N.s/m"},
    };
    for (const FourWheelPage& definition : fourWheelPages)
    {
        page = {definition.title, definition.image, definition.icon, {}, true, "Physical values, left / right symmetry per axle"};
        for (int wheel = 0; wheel < 4; ++wheel)
            AddField(page, cornerNames[wheel], definition.unit, &definition.targets[wheel], definition.minimum,
                definition.maximum, definition.step, definition.scale, wheel);
        pages.push_back(page);
    }
    page = {"Anti-roll", "SimuFondRoll.png", "Roll", {}, false, "Anti-roll spring stiffness"};
    for (int axle = 0; axle < 2; ++axle)
        AddField(page, axle == 0 ? "Front" : "Rear", "N/m", &setup.arbSpring[axle], 0, 300000, 1000);
    pages.push_back(page);
    page = {"Engine", "SimuFondEngine.png", "Engine", {}, false, "Reliability: engine thermal simulation; no artificial rating"};
    AddField(page, "Maximum revs", "rpm", &setup.revsLimiter, 10000, 19000, 50, 60.0 / (2 * 3.141592653589793));
    pages.push_back(page);
    page = {"Steering", "SimuFondSteering.png", "Steering", {}, false, "Controller filtering retains the current driver configuration"};
    AddField(page, "Steering lock", "deg", &setup.steerLock, 5, 40, 0.5, 1.0/radiansPerDegree);
    AddField(page, "Steering wheel", "deg", &setup.steerWheelRot, 90, 900, 10, 1.0/radiansPerDegree);
    pages.push_back(page);
    page = {"Brakes", "SimuFondFreins.png", "Brakes", {}, false, "Brake balance and maximum hydraulic pressure"};
    AddField(page, "Front balance", "%", &setup.brakeRepartition, 35, 75, 0.5, 100);
    AddField(page, "Maximum pressure", "MPa", &setup.brakePressure, 1, 30, 0.1, 0.000001);
    pages.push_back(page);
    page = {"Gearbox", "SimuFondGearbox.png", "Gear", {}, false, "Actual gearbox ratios, not decorative tooth counts"};
    for (int gear = 1; gear <= std::min(6, garageCar->_gearNb - garageCar->_gearOffset - 1); ++gear)
        AddField(page, "Gear " + std::to_string(gear), "ratio", &setup.gearRatio[gear + 1], 0.5, 5, 0.01);
    AddField(page, "Final drive", "ratio", &setup.differentialRatio[1], 1, 8, 0.01);
    pages.push_back(page);
    page = {"Tyres", "SimuFondPneus.png", "Tyres", {}, false, "Cold gauge pressure; current thermal state is shown below"};
    const char* pressureNames[4] = {"front right", "front left", "rear right", "rear left"};
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double coldPressure = GfParmGetNum(garageCar->_carHandle, "Formula Tires",
            (std::string("cold pressure ") + pressureNames[wheel]).c_str(), nullptr, 100000.0);
        AddField(page, cornerNames[wheel], "kPa", nullptr, 60, 200, 1, 1, wheel, 1, coldPressure / 1000.0);
    }
    pages.push_back(page);
    page = {"Fuel quantity", "SimuFondQuantity.png", "Quant", {}, false, "Fuel loaded in the car"};
    AddField(page, "Fuel", "liters", &setup.fuel, 1, GfParmGetNum(garageCar->_carHandle, SECT_CAR, PRM_TANK, nullptr, 120), 1);
    fields.back().value = fields.back().original = garageCar->_fuel;
    pages.push_back(page);
    page = {"Differential", "SimuFondGearbox.png", "Gear", {}, false, "Rear limited slip: preload and torque bias limits"};
    AddField(page, "On throttle", "%", &setup.differentialMaxSlipBias[1], 0, 100, 1, 100);
    AddField(page, "Off throttle", "%", &setup.differentialCoastMaxSlipBias[1], 0, 100, 1, 100);
    AddField(page, "Preload", "N.m", nullptr, 0, 200, 1, 1, -1, 2,
        GfParmGetNum(garageCar->_carHandle, SECT_REARDIFFERENTIAL, "preload torque", nullptr, 10));
    pages.push_back(page);
    // Append to preserve the existing family indexes used by the four tabs.
    page = {"Third spring", "SimuFondSprings.png", "Springs", {}, false, "Rear heave spring stiffness"};
    AddField(page, "Rear third spring", "kN/m", &setup.heaveSpring[1], 0, 1000, 5, 0.001);
    pages.push_back(page);
}
void ChangeField(int fieldIndex, double value)
{
    GarageField& field = fields[fieldIndex];
    if (!field.editable) return;
    field.value = std::max(field.minimum, std::min(field.maximum,
        std::round(value / field.step) * field.step));
    if (field.wheel >= 0 && (field.wheel < 2 ? symmetricFront : symmetricRear))
    {
        // A tab now contains several parameter families. Mirror only this
        // field's original family, never another setting on the same wheel.
        for (const GaragePage& page : pages)
            if (std::find(page.fields.begin(), page.fields.end(), fieldIndex) != page.fields.end())
                for (int counterpart : page.fields)
                    if (fields[counterpart].wheel == (field.wheel ^ 1))
                        fields[counterpart].value = std::max(fields[counterpart].minimum,
                            std::min(fields[counterpart].maximum, field.value));
    }
}
void StyledButton(double x, double y, double width, double height,
    const std::string& label, const std::function<void()>& action, bool primary = false)
{
    Rectangle(x, y, width, height, primary ? gold : panelColor);
    Outline(x, y, width, height, primary ? gold : turquoise);
    Text(label, x + width / 2, y + (height - 14) / 2, GFUI_FONT_SMALL,
        primary ? graphite : white);
    AddButton(x, y, width, height, label, action);
}
void Panel(double x, double y, double width, double height, const std::string& title,
    const std::string& tag)
{
    Rectangle(x, y, width, height, panelColor);
    Outline(x, y, width, height, turquoise);
    const double titleWidth = tag.empty() ? width - 28 : (width - 28) * .60;
    Text(title, x + 14, y + height - 28, GFUI_FONT_MEDIUM, gold, GFUI_ALIGN_HL, titleWidth);
    Text(tag, x + width - 14, y + height - 26, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HR, (width - 28) * .40 - 10);
    Rectangle(x + 14, y + height - 40, width - 28, 1, turquoise);
}
void Parameter(int index, const std::string& label, double x, double y, double width)
{
    GarageField& field = fields[index];
    if (!field.editable) {
        Text(label, x, y + 5, GFUI_FONT_SMALL_C, white, GFUI_ALIGN_HL);
        Text(Number(field.value, field.unit) + " [solo lectura]", x + width, y + 5,
            GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HR);
        return;
    }
    const double ratio = (field.value - field.minimum) / std::max(field.maximum - field.minimum, 1e-9);
    const double minus = x + width * .32;
    const double slider = minus + 26;
    const double length = width * .25;
    Text(label, x, y + 5, GFUI_FONT_SMALL_C, white, GFUI_ALIGN_HL);
    StyledButton(minus, y + 2, 18, 20, "-", [index]() { ChangeField(index, fields[index].value - fields[index].step); });
    Rectangle(slider, y + 9, length, 6, turquoise);
    Rectangle(slider, y + 9, length * ratio, 6, gold);
    Rectangle(slider + length * ratio - 3, y + 4, 6, 16, white);
    controls.push_back({slider - 6, y, length + 12, 24, label, index, false, {}});
    std::string displayed = Number(field.value, field.unit);
    if (rustGarage && field.step < 0.01) {
        char precise[96]; std::snprintf(precise, sizeof(precise), "%.4f %s", field.value, field.unit.c_str());
        displayed = precise;
    }
    Text(displayed, x + width - 29, y + 5, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HR);
    StyledButton(x + width - 20, y + 2, 18, 20, "+", [index]() { ChangeField(index, fields[index].value + fields[index].step); });
}
void PageParameter(int page, int item, const std::string& label, double x, double y, double width)
{
    if (item < (int)pages[page].fields.size()) Parameter(pages[page].fields[item], label, x, y, width);
}
void CarDiagram(double x, double y)
{
    auto* preview = dynamic_cast<ICarPreviewRenderer*>(LegacyMenu::self().graphicsEngine());
    const double width = 324, height = 270;
    const int left = viewportLeft + (int)((x - width / 2) * viewportWidth / editorWidth);
    const int bottom = viewportBottom + (int)((y - height / 2) * viewportHeight / editorHeight);
    if (!preview || !preview->drawGarageCarPreview(garageCar, left, bottom,
        (int)(width * viewportWidth / editorWidth), (int)(height * viewportHeight / editorHeight), selectedPage == 1))
        Text("Vista 3D no disponible", x, y, GFUI_FONT_SMALL_C, muted);
}
void RustParameter(const std::string& path, const std::string& label, double x, double y, double width)
{
    for (int i = 0; i < (int)fields.size(); ++i)
        if (fields[i].path == path) { Parameter(i, label, x, y, width); return; }
    Text(label + "  --", x, y + 5, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
}
void BuildRustPanels()
{
    // Reuse the original four-tab composition and artwork. Only the data bindings
    // differ: every editable widget still addresses the authoritative Rust setup.
    const char* cornerNames[] = {"DELANTERA IZQUIERDA", "DELANTERA DERECHA", "TRASERA IZQUIERDA", "TRASERA DERECHA"};
    const char* pressureCorners[] = {"FL", "FR", "RL", "RR"};
    const int wheels[] = {1, 0, 3, 2};
    Rectangle(20, 598, 1240, 32, panelColor);
    Rectangle(20, 598, 3, 32, gold);
    Text("REGLAJES DEL VEHICULO", 35, 608, GFUI_FONT_SMALL_C, gold, GFUI_ALIGN_HL);
    Text("Arrastra o usa - / +", 1242, 608, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HR);
    if (selectedPage < 2)
    {
        Text("Suspension y geometria compartidas por eje", 550, 608, GFUI_FONT_SMALL_C, muted);
        for (int corner = 0; corner < 4; ++corner)
        {
            const double x = corner % 2 ? 838 : 20;
            const double y = corner < 2 ? 353 : 108;
            const std::string axle = corner < 2 ? "suspension.front." : "suspension.rear.";
            Panel(x, y, 422, 231, cornerNames[corner], selectedPage == 0 ? "SUSPENSION / POR EJE" : "NEUMATICO");
            if (selectedPage == 0)
            {
                const char* keys[] = {"bump_stop_multiplier", "bump_damp_multiplier", "rebound_damp_multiplier", "damping_ratio", "resting_ratio", "spring_length"};
                const char* labels[] = {"Topes", "Compresion", "Extension", "Amortiguacion", "Altura / ratio", "Long. muelle"};
                for (int row = 0; row < 6; ++row)
                    RustParameter(axle + keys[row], labels[row], x + 14, y + 158 - row * 26, 394);
            }
            else
            {
                RustParameter(std::string("tires.pressure.cold_kpa_gauge.") + pressureCorners[corner], "Presion fria", x + 14, y + 151, 394);
                RustParameter(axle + "camber", "Caida", x + 14, y + 113, 394);
                RustParameter(axle + "toe", "Convergencia", x + 14, y + 75, 394);
                Text(Number(garageCar->_tyreT_mid(wheels[corner]) - 273.15, "C / banda actual"), x + 14, y + 29, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
            }
        }
        Panel(458, 108, 360, 476, selectedPage == 0 ? "BARRAS / CHASIS" : "BALANCE / REGLAJES", "");
        CarDiagram(638, selectedPage == 0 ? 346 : 315);
        if (selectedPage == 0)
        {
            RustParameter("suspension.geometry_physical.front_arb.ratio", "Barra del.", 472, 492, 332);
            RustParameter("suspension.geometry_physical.rear_arb.ratio", "Barra tras.", 472, 459, 332);
            RustParameter("steering.max_steering_angle", "Direccion", 472, 145, 332);
            Text("Vista superior / modelo 3D", 638, 119, GFUI_FONT_SMALL_C, muted);
        }
        else
        {
            RustParameter("aero.model.front_wing.incidence_deg", "Ala del.", 472, 492, 332);
            RustParameter("aero.model.rear_wing.incidence_deg", "Ala tras.", 472, 459, 332);
            RustParameter("brakes.front_brake_bias", "Balance del.", 472, 145, 332);
            RustParameter("brakes.max_brake_torque", "Par freno", 472, 115, 332);
        }
    }
    else if (selectedPage == 2)
    {
        Panel(20, 353, 422, 231, "MOTOR", "LIMITADOR");
        RustParameter("powertrain.max_rpm", "Regimen max.", 34, 492, 394);
        Text("Valores reales del motor instalado", 34, 460, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
        Panel(20, 108, 422, 231, "DIRECCION / FRENADA", "CONTROLES");
        RustParameter("steering.max_steering_angle", "Tope direccion", 34, 260, 394);
        RustParameter("steering.steering_speed", "Respuesta", 34, 218, 394);
        RustParameter("brakes.max_brake_torque", "Par freno", 34, 176, 394);
        Panel(458, 108, 360, 476, "DIFERENCIAL", "TRASERO");
        RustParameter("powertrain.differential.power_ramp_angle_deg", "Aceleracion", 472, 492, 332);
        RustParameter("powertrain.differential.coast_ramp_angle_deg", "Retencion", 472, 457, 332);
        RustParameter("powertrain.differential.preload_nm", "Precarga", 472, 422, 332);
        RustParameter("powertrain.differential.clutches", "Discos", 472, 387, 332);
        TabImage("engine-gray-v2.png", 472, 124, 332, 240);
        for (int half = 0; half < 2; ++half)
        {
            const double y = half == 0 ? 353 : 108;
            Panel(838, y, 422, 231, half == 0 ? "CAJA / 1 - 3" : "CAJA / 4 - 6", "RELACIONES / SOLO LECTURA");
            for (int row = 0; row < 3; ++row)
            {
                const int gear = half * 3 + row + 1;
                // Native ratios include the installed final drive. Do not derive the
                // individual gears from an unapplied final-drive edit.
                double installedFinalDrive = 0;
                for (const auto& field : fields) if (field.path == "powertrain.final_drive") installedFinalDrive = field.original;
                const std::string ratio = gear < garageCar->_gearNb && installedFinalDrive > 0
                    && garageCar->_gearRatio[gear + garageCar->_gearOffset] > 0
                    ? Number(garageCar->_gearRatio[gear + garageCar->_gearOffset] / installedFinalDrive, "ratio") : "--";
                Text("Marcha " + std::to_string(gear), 852, y + 150 - row * 35, GFUI_FONT_SMALL_C, white, GFUI_ALIGN_HL);
                Text(ratio, 1246, y + 150 - row * 35, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HR);
            }
            if (half == 1) RustParameter("powertrain.final_drive", "Grupo final", 852, y + 35, 394);
        }
    }
    else
    {
        Panel(20, 353, 422, 231, "COMBUSTIBLE", "CARGA AL SALIR");
        RustParameter("fuel.initial_kg", "Combustible", 34, 492, 394);
        Text("Carga actual: " + Number(garageCar->_fuel, "L"), 34, 452, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
        Panel(838, 353, 422, 231, "NEUMATICOS", "SERVICIO");
        StyledButton(852, 485, 394, 34, replaceTires ? "[X] Montar neumaticos nuevos" : "[ ] Montar neumaticos nuevos", []() { replaceTires = !replaceTires; }, replaceTires);
        Text("Presiones: pestana Aero / Ruedas.", 852, 448, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
        Panel(458, 108, 360, 476, "ESTRATEGIA / BOXES", "");
        TabImage("strategy-gray-v2.png", 472, 129, 332, 401);
        Panel(20, 108, 422, 231, "TEMPERATURAS", "DELANTERAS");
        Panel(838, 108, 422, 231, "TEMPERATURAS", "TRASERAS");
        for (int corner = 0; corner < 4; ++corner)
        {
            const double x = corner < 2 ? 34 : 852;
            const double y = corner % 2 == 0 ? 252 : 179;
            Text(cornerNames[corner], x, y, GFUI_FONT_SMALL, gold, GFUI_ALIGN_HL);
            Text(Number(garageCar->_tyreT_mid(wheels[corner]) - 273.15, "C / banda"), x, y - 27, GFUI_FONT_MEDIUM, white, GFUI_ALIGN_HL);
        }
    }
}
void BuildEditorControls()
{
    Rectangle(0, 0, editorWidth, editorHeight, graphite);
    for (int y = 0; y < editorHeight; y += 7)
    {
        float line[4] = {turquoise[0], turquoise[1], turquoise[2], .04f};
        Rectangle(0, y, editorWidth, 1, line);
    }
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor4fv(gold); glVertex2d(20, 706); glVertex2d(20, 780);
    glColor4fv(graphite); glVertex2d(1260, 780); glVertex2d(1260, 706);
    glEnd();
    Rectangle(20, 706, 7, 74, gold);
    Rectangle(20, 706, 1240, 2, gold);
    Text("OPEN FORMULA", 44, 740, GFUI_FONT_BIG, white, GFUI_ALIGN_HL);
    Text("SIMULATION / GARAGE", 46, 719, GFUI_FONT_SMALL, graphite, GFUI_ALIGN_HL);
    const tRmInfo* race = LmRaceEngine().inData();
    Text(race->track->name ? race->track->name : "PRACTICE", 1238, 746, GFUI_FONT_MEDIUM, white, GFUI_ALIGN_HR);
    Text(garageCar->_name, 1238, 723, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HR);
    const char* tabs[] = {"SUSPENSION", "AERO / RUEDAS", "MOTOR / TRANSMISION", "ESTRATEGIA / BOXES"};
    for (int tab = 0; tab < 4; ++tab)
    {
        const double x = 20 + tab * 312;
        Rectangle(x, 644, 304, 46, selectedPage == tab ? turquoise : panelColor);
        Outline(x, 644, 304, 46, turquoise);
        if (selectedPage == tab) Rectangle(x, 644, 304, 3, gold);
        Text(tabs[tab], x + 152, 660, GFUI_FONT_SMALL, white);
        AddButton(x, 644, 304, 46, tabs[tab], [tab]() {
            selectedPage = tab; selectedControl = 0; draggedControl = -1;
        });
    }
    if (rustGarage) {
        BuildRustPanels();
        Text(statusMessage.empty() ? "Configuracion actual" : statusMessage,
            24, 83, GFUI_FONT_SMALL_C, gold, GFUI_ALIGN_HL);
        StyledButton(711, 37, 150, 42, "RESTABLECER", []() {
            for (GarageField& field : fields) field.value = field.original;
            replaceTires = false;
        });
        StyledButton(875, 37, 154, 42, "CANCELAR", CancelEditor);
        StyledButton(1043, 37, 217, 42, "APLICAR Y VOLVER", ApplyEditor, true);
        return;
    }
    Rectangle(20, 598, 1240, 32, panelColor);
    Rectangle(20, 598, 3, 32, gold);
    Text("REGLAJES DEL VEHICULO", 35, 608, GFUI_FONT_SMALL_C, gold, GFUI_ALIGN_HL);
    if (selectedPage < 2)
    {
        StyledButton(270, 603, 165, 22, symmetricFront ? "[X] Simetria delantera" : "[ ] Simetria delantera",
            []() { symmetricFront = !symmetricFront; });
        StyledButton(446, 603, 165, 22, symmetricRear ? "[X] Simetria trasera" : "[ ] Simetria trasera",
            []() { symmetricRear = !symmetricRear; });
    }
    Text("Arrastra o usa - / +", 1242, 608, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HR);
    if (selectedPage < 2)
    {
        const char* corners[] = {"DELANTERA IZQUIERDA", "DELANTERA DERECHA", "TRASERA IZQUIERDA", "TRASERA DERECHA"};
        const int wheels[] = {1, 0, 3, 2};
        const int families[] = {4, 9, 8, 7, 6, 5, 1};
        const char* names[] = {"Topes", "Compresion rapida", "Extension rapida", "Compresion lenta", "Extension lenta", "Muelle", "Altura"};
        for (int corner = 0; corner < 4; ++corner)
        {
            const double x = corner % 2 ? 838 : 20;
            const double y = corner < 2 ? 353 : 108;
            Panel(x, y, 422, 231, corners[corner], selectedPage == 0 ? "SUSPENSION" : "NEUMATICO");
            if (selectedPage == 0)
                for (int row = 0; row < 7; ++row)
                    PageParameter(families[row], wheels[corner], names[row], x + 14, y + 158 - row * 24, 394);
            else
            {
                PageParameter(15, wheels[corner], "Presion fria", x + 14, y + 151, 394);
                PageParameter(2, wheels[corner], "Caida", x + 14, y + 113, 394);
                PageParameter(3, wheels[corner], "Convergencia", x + 14, y + 75, 394);
                Text(Number(garageCar->_tyreT_mid(wheels[corner]) - 273.15, "C / banda actual"),
                    x + 14, y + 29, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
            }
        }
        Panel(458, 108, 360, 476, selectedPage == 0 ? "BARRAS / CHASIS" : "BALANCE / REGLAJES", "");
        CarDiagram(638, 315);
        if (selectedPage == 0)
        {
            PageParameter(10, 0, "Barra del.", 472, 492, 332);
            PageParameter(10, 1, "Barra tras.", 472, 459, 332);
            PageParameter(18, 0, "Tercer muelle", 472, 426, 332);
            PageParameter(12, 0, "Direccion", 472, 145, 332);
            Text("Vista superior / modelo 3D", 638, 119, GFUI_FONT_SMALL_C, muted);
        }
        else
        {
            PageParameter(0, 0, "Ala del.", 472, 492, 332);
            PageParameter(0, 1, "Ala tras.", 472, 459, 332);
            Text("Vista lateral / modelo 3D", 638, 200, GFUI_FONT_SMALL_C, muted);
            PageParameter(13, 0, "Freno del.", 472, 145, 332);
            PageParameter(13, 1, "Presion", 472, 115, 332);
        }
    }
    else if (selectedPage == 2)
    {
        Panel(20, 353, 422, 231, "MOTOR", "LIMITADOR");
        PageParameter(11, 0, "Regimen max.", 34, 492, 394);
        Text("Valores reales del motor instalado", 34, 460, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
        Panel(20, 108, 422, 231, "DIRECCION / FRENADA", "CONTROLES");
        PageParameter(12, 0, "Tope direccion", 34, 260, 394);
        PageParameter(12, 1, "Giro volante", 34, 218, 394);
        PageParameter(13, 1, "Presion freno", 34, 176, 394);
        Panel(458, 108, 360, 476, "DIFERENCIAL", "TRASERO");
        for (int i = 0; i < 3; ++i)
            PageParameter(17, i, i == 0 ? "Aceleracion" : i == 1 ? "Retencion" : "Precarga", 472, 492 - i * 35, 332);
        TabImage("engine-gray-v2.png", 472, 124, 332, 274);
        for (int half = 0; half < 2; ++half)
        {
            const double y = half == 0 ? 353 : 108;
            Panel(838, y, 422, 231, half == 0 ? "CAJA / 1 - 3" : "CAJA / 4 - 6", "RELACIONES");
            for (int row = 0; row < 3; ++row)
                PageParameter(14, half * 3 + row, "Marcha " + std::to_string(half * 3 + row + 1), 852, y + 145 - row * 35, 394);
            if (half == 1)
            {
                const int finalDrive = (int)pages[14].fields.size() - 1;
                PageParameter(14, finalDrive, "Grupo final", 852, y + 35, 394);
            }
        }
    }
    else
    {
        Panel(20, 353, 422, 231, "COMBUSTIBLE", "CARGA AL SALIR");
        PageParameter(16, 0, "Combustible", 34, 492, 394);
        Text("Carga actual: " + Number(garageCar->_fuel, "L"), 34, 452, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
        Text("Se aplica al confirmar los reglajes.", 34, 419, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
        Panel(838, 353, 422, 231, "NEUMATICOS", "SERVICIO");
        StyledButton(852, 485, 394, 34, replaceTires ? "[X] Montar neumaticos nuevos" : "[ ] Montar neumaticos nuevos",
            []() { replaceTires = !replaceTires; }, replaceTires);
        Text("Presiones: pestana Aero / Ruedas.", 852, 448, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
        Panel(458, 108, 360, 476, "ESTRATEGIA / BOXES", "");
        TabImage("strategy-gray-v2.png", 472, 129, 332, 401);
        Panel(20, 108, 422, 231, "TEMPERATURAS", "DELANTERAS");
        Panel(838, 108, 422, 231, "TEMPERATURAS", "TRASERAS");
        const char* corners[] = {"DEL. IZQ.", "DEL. DER.", "TRAS. IZQ.", "TRAS. DER."};
        const int wheels[] = {1, 0, 3, 2};
        for (int i = 0; i < 4; ++i)
        {
            const double x = i < 2 ? 34 : 852;
            const double y = i % 2 == 0 ? 252 : 179;
            Text(corners[i], x, y, GFUI_FONT_SMALL, gold, GFUI_ALIGN_HL);
            Text(Number(garageCar->_tyreT_mid(wheels[i]) - 273.15, "C / banda"),
                x, y - 27, GFUI_FONT_MEDIUM, white, GFUI_ALIGN_HL);
        }
    }
    bool changed = replaceTires;
    for (const GarageField& field : fields) changed = changed || field.value != field.original;
    Text(changed ? "Cambios sin aplicar" : "Configuracion actual", 24, 75, GFUI_FONT_SMALL, changed ? gold : muted, GFUI_ALIGN_HL);
    Text("TAB / D-pad: navegar   Flechas: ajustar   LB / RB: pestana   START / F5: aplicar", 24, 48, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HL);
    StyledButton(711, 37, 150, 42, "RESTABLECER", []() {
        for (GarageField& field : fields) field.value = field.original;
        replaceTires = false;
    });
    StyledButton(875, 37, 154, 42, "CANCELAR", CancelEditor);
    StyledButton(1043, 37, 217, 42, "APLICAR Y VOLVER", ApplyEditor, true);
}
void BuildPitControls()
{
    float background[4] = {panelColor[0], panelColor[1], panelColor[2], .92f};
    Rectangle(120, 168, 400, 215, background);
    Outline(120, 168, 400, 215, turquoise);
    Rectangle(120, 377, 400, 6, gold);
    Text("PIT", 320, 342, GFUI_FONT_BIG, gold);
    StyledButton(154, 262, 332, 48, "EMPEZAR", BeginDriving, true);
    StyledButton(154, 200, 332, 48, "IR AL GARAJE", OpenEditor);
}
void SetSliderFromMouse(int index, double horizontal, double vertical)
{
    if (index < 0 || index >= (int)controls.size()) return;
    const GarageControl& control = controls[index];
    if (control.field < 0) return;
    const GarageField& field = fields[control.field];
    const double fraction = control.vertical ? (vertical - control.bottom - 6) / (control.height - 12)
        : (horizontal - control.left - 6) / (control.width - 12);
    ChangeField(control.field, field.minimum + fraction * (field.maximum - field.minimum));
}
void MouseMotion(int horizontal, int vertical)
{
    int screenWidth, screenHeight, viewWidth, viewHeight;
    PreviewScreenSize(&screenWidth, &screenHeight, &viewWidth, &viewHeight);
    mousePositionX = (horizontal - viewportLeft) * (editorOpen ? editorWidth : 640) / std::max(viewportWidth, 1);
    mousePositionY = (screenHeight - vertical - viewportBottom) * (editorOpen ? editorHeight : 480) / std::max(viewportHeight, 1);
    if (draggedControl >= 0) SetSliderFromMouse(draggedControl, mousePositionX, mousePositionY);
    else for (size_t index = 0; index < controls.size(); ++index)
    {
        const GarageControl& control = controls[index];
        if (mousePositionX >= control.left && mousePositionX <= control.left + control.width
            && mousePositionY >= control.bottom && mousePositionY <= control.bottom + control.height)
            selectedControl = (int)index;
    }
}
void MouseButton(int button, int state, int horizontal, int vertical)
{
    if (button != SDL_BUTTON_LEFT) return;
    MouseMotion(horizontal, vertical);
    if (state == SDL_RELEASED) { draggedControl = -1; return; }
    for (size_t index = 0; index < controls.size(); ++index)
    {
        GarageControl control = controls[index];
        if (mousePositionX < control.left || mousePositionX > control.left + control.width
            || mousePositionY < control.bottom || mousePositionY > control.bottom + control.height) continue;
        selectedControl = (int)index;
        if (control.field >= 0) { draggedControl = (int)index; SetSliderFromMouse((int)index, mousePositionX, mousePositionY); }
        else if (control.action) control.action();
        return;
    }
}
void Keyboard(int key, int, int, int)
{
    if (!garageOpen) return;
    if (key == SDLK_ESCAPE || key == SDLK_BACKSPACE) { if (editorOpen) CancelEditor(); return; }
    if (key == SDLK_g && !editorOpen) { OpenEditor(); return; }
    if (key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
    {
        if (editorOpen) { selectedPage = (selectedPage + (key == SDLK_PAGEUP ? 3 : 1)) % 4; selectedControl = 0; draggedControl = -1; }
        return;
    }
    if (key == SDLK_F5 && editorOpen) { ApplyEditor(); return; }
    if (key == SDLK_TAB || key == SDLK_DOWN || key == SDLK_UP)
    {
        if (!controls.empty()) selectedControl = (selectedControl + (key == SDLK_UP ? controls.size() - 1 : 1)) % controls.size();
        return;
    }
    if (selectedControl < 0 || selectedControl >= (int)controls.size()) return;
    GarageControl control = controls[selectedControl];
    if ((key == SDLK_LEFT || key == SDLK_RIGHT) && control.field >= 0)
        ChangeField(control.field, fields[control.field].value + (key == SDLK_LEFT ? -1 : 1) * fields[control.field].step);
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
        if (control.action) control.action();
}

int main(int argc, char** argv) {
    if (argc != 4) return 2;
    GfInit(false);
    const std::string data = std::string(argv[1]) + "/distribution/share/games/speed-dreams-2/";
    GfSetDataDir(data.c_str());
    GfSetLocalDir((std::string(argv[1])+"/distribution/user-settings/").c_str());
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) || TTF_Init()) return 3;
    SDL_Window* window = SDL_CreateWindow("Garage validation", 0, 0, 1280, 800,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window || !SDL_GL_CreateContext(window)) return 4;
    const std::string font = data + "data/fonts/Libron/Libron-Bold.ttf";
    int sizes[] = {44,36,28,22,40,32,24,20,64,48,28,24};
    for (int i=0; i<12; ++i) gfuiFont[i] = new GfuiFontClass(font.c_str(), sizes[i]);
    tCarElt car{};
    garageCar = &car;
    std::strcpy(car._name, "Williams FW19");
    car._fuel = 60; car._gearNb = 8; car._gearOffset = 1;
    car._carHandle = GfParmReadFile((data + "cars/models/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml").c_str(), GFPARM_RMODE_STD);
    if (!car._carHandle) return 5;
    for(int i=0; i<4; ++i) {
        car.setup.rideHeight[i].value = i<2 ? .05 : .045;
        car.setup.suspPacker[i].value = .01;
        car.setup.suspSpring[i].value = 103040;
        car.setup.suspSlowBump[i].value = 5348;
        car.setup.suspSlowRebound[i].value = 8804;
        car.setup.suspFastBump[i].value = 3000;
        car.setup.suspFastRebound[i].value = 5000;
        car.setup.camber[i].value = -.04;
        car._tyreT_mid(i) = 355;
    }
    car.setup.arbSpring[0].value = 100000; car.setup.arbSpring[1].value = 80000;
    car.setup.wingAngle[0].value=.2; car.setup.wingAngle[1].value=.3;
    car.setup.revsLimiter.value=17500 * 2 * 3.141592653589793 / 60;
    car.setup.steerLock.value=.3; car.setup.steerWheelRot.value=7;
    car.setup.brakeRepartition.value=.6; car.setup.brakePressure.value=12000000;
    car.setup.fuel.value=60;
    for(int i=2;i<8;++i) car.setup.gearRatio[i].value=3.4-(i-2)*.4;
    car.setup.differentialRatio[1].value=4.65;
    car.setup.heaveSpring[1].value=GfParmGetNum(car._carHandle,"Rear Heave Spring","spring",nullptr,0);
    tTrack track{}; track.name = const_cast<char*>("Fuji Speedway"); previewRace.track=&track;
    BuildPages(); editorOpen = garageOpen = true;
    int checks=0, failures=0;
    auto check = [&](bool pass, const char* message) { ++checks; if(!pass) { ++failures; std::printf("FAIL %s\n", message); } };
    const int rearHeave=pages[18].fields[0];
    check(fields[rearHeave].target==&car.setup.heaveSpring[1],"third spring targets rear native heave setup");
    check(fields[rearHeave].value==285 && fields[rearHeave].scale==.001,"rear third spring displays 285 kN/m");
    ChangeField(rearHeave,300);
    check(fields[rearHeave].value/fields[rearHeave].scale==300000,"third spring editor converts kN/m to N/m");
    ChangeField(rearHeave,fields[rearHeave].original);
    osggraph::osgLoader loader;
    osggraph::osgLoader::loadedModels loaded;
    const std::string directory=data+"cars/models/williams-nineteen-ninety-seven/";
    loader.AddSearchPath(directory);
    osg::ref_ptr<osg::MatrixTransform> root=new osg::MatrixTransform;
    root->setMatrix(osg::Matrix::translate(1195,506,33));
    osg::ref_ptr<osg::Node> body=loader.Load3dFile(loaded,directory+"williams-nineteen-ninety-seven.acc",true);
    check(body.valid(),"production ACC body loaded");
    root->addChild(body);
    const char* wheelSections[]={"Front Right Wheel","Front Left Wheel","Rear Right Wheel","Rear Left Wheel"};
    const char* wheelFiles[]={"front_right_wheel_0.acc","front_left_wheel_0.acc","rear_right_wheel_0.acc","rear_left_wheel_0.acc"};
    for(int i=0;i<4;++i) {
        const double width=GfParmGetNum(car._carHandle,wheelSections[i],"tire width",nullptr,.3);
        const double radius=GfParmGetNum(car._carHandle,wheelSections[i],"rim diameter",nullptr,.33)/2
            +width*GfParmGetNum(car._carHandle,wheelSections[i],"tire height-width ratio",nullptr,.47);
        const double ypos=GfParmGetNum(car._carHandle,wheelSections[i],"ypos",nullptr,i%2?.83:-.83);
        const double xpos=GfParmGetNum(car._carHandle,i<2?"Front Axle":"Rear Axle","xpos",nullptr,i<2?1.5:-1.5);
        osg::ref_ptr<osg::Node> wheel=loader.Load3dFile(loaded,directory+wheelFiles[i],true);
        check(wheel.valid(),"production ACC wheel loaded");
        osg::ref_ptr<osg::MatrixTransform> transform=new osg::MatrixTransform;
        transform->setMatrix(osg::Matrix::scale(radius*2,width,radius*2)*osg::Matrix::translate(xpos,ypos,radius-.05));
        transform->addChild(wheel); root->addChild(transform);
    }
    check(previewRenderer.mesh.capture(root),"real vehicle geometry captured");
    check(previewRenderer.mesh.triangleCount()>1000,"preview contains detailed car geometry");
    check(previewRenderer.mesh.maximum().x()<10 && previewRenderer.mesh.minimum().x()>-10,"world placement excluded");
    const auto triangleCount=previewRenderer.mesh.triangleCount();
    std::printf("preview triangles=%u dimensions=%.4f,%.4f,%.4f m\n",(unsigned)triangleCount,
        (previewRenderer.mesh.maximum()-previewRenderer.mesh.minimum()).x(),
        (previewRenderer.mesh.maximum()-previewRenderer.mesh.minimum()).y(),
        (previewRenderer.mesh.maximum()-previewRenderer.mesh.minimum()).z());
    const osg::Matrix originalPlacement=root->getMatrix();
    const auto* originalMaterial=body->getStateSet();
    osg::ref_ptr<osg::Switch> driver=new osg::Switch; driver->setName("DRIVER"); driver->addChild(body);
    root->addChild(driver); previewRenderer.mesh.capture(root);
    check(previewRenderer.mesh.triangleCount()==triangleCount,"driver branch excluded");
    root->removeChild(driver);
    osg::ref_ptr<osg::Switch> cockpit=new osg::Switch;
    cockpit->setName("COCK"); cockpit->addChild(new osg::Group,true); cockpit->addChild(body,false);
    root->removeChild(body); root->addChild(cockpit);
    previewRenderer.mesh.capture(root);
    check(previewRenderer.mesh.triangleCount()==triangleCount,"exterior visible despite racing cockpit selection");
    check(cockpit->getValue(0) && !cockpit->getValue(1),"live cockpit switch is unchanged");
    GfModule* module=GfModule::load(std::string(argv[3])+"/lib/games/speed-dreams-2/modules/graphic/osggraph.dll");
    check(module!=nullptr,"compiled graphics DLL loads");
    auto* graphics=dynamic_cast<IGraphicsEngine*>(module);
    auto* capability=dynamic_cast<ICarPreviewRenderer*>(graphics);
    check(capability!=nullptr,"optional preview interface works across DLL boundary");
    if(capability) {
        capability->resetGarageCarPreview();
        check(!capability->drawGarageCarPreview(&car,0,0,100,100,false),"missing live race scene safely rejected");
    }
    if(module) check(GfModule::unload(module),"graphics DLL releases preview resources");
    viewportWidth=1280; viewportHeight=800; viewportLeft=viewportBottom=0;
    glViewport(0,0,1280,800); glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0,1280,0,800);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    std::vector<bool> reachable(fields.size(),false);
    for(int tab=0; tab<4; ++tab) {
        selectedPage=tab; controls.clear(); BuildEditorControls();
        for(size_t i=0; i<controls.size(); ++i) {
            const auto& c=controls[i];
            check(c.left>=0 && c.bottom>=0 && c.left+c.width<=1280 && c.bottom+c.height<=800,"control inside canvas");
            if(c.field>=0) reachable[c.field]=true;
            for(size_t j=i+1;j<controls.size();++j) {
                const auto& d=controls[j];
                check(!(c.left<d.left+d.width && d.left<c.left+c.width && c.bottom<d.bottom+d.height && d.bottom<c.bottom+c.height),"hit targets do not overlap");
            }
        }
        for(int next=0;next<4;++next) {
            auto action=controls[next].action; action(); check(selectedPage==next,"tab action");
        }
        selectedPage=tab;
        SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,1280,800,24,SDL_PIXELFORMAT_RGB24);
        glPixelStorei(GL_PACK_ALIGNMENT,1);
        glReadPixels(0,0,1280,800,GL_RGB,GL_UNSIGNED_BYTE,surface->pixels);
        // BMP row order is corrected by the Python driver below.
        std::string output=std::string(argv[2])+"/tab-"+std::to_string(tab)+".bmp";
        SDL_SaveBMP(surface,output.c_str()); SDL_FreeSurface(surface);
        std::printf("tab=%d controls=%zu\n",tab,controls.size());
    }
    for(bool found:reachable) check(found,"all original setup fields remain reachable");
    check(previewRenderer.topCalls==1 && previewRenderer.sideCalls==1,"3D car only in suspension and aero tabs");
    check(imageTextures.count("engine-gray-v2.png") && imageTextures["engine-gray-v2.png"].identifier!=0,"gray engine silhouette loaded");
    check(imageTextures.count("strategy-gray-v2.png") && imageTextures["strategy-gray-v2.png"].identifier!=0,"gray pit crew silhouette loaded");
    check(root->getMatrix()==originalPlacement && body->getStateSet()==originalMaterial,"live scene transform and state preserved");
    GLint viewportBefore[4],viewportAfter[4]; glGetIntegerv(GL_VIEWPORT,viewportBefore);
    GLdouble projectionBefore[16],projectionAfter[16]; glGetDoublev(GL_PROJECTION_MATRIX,projectionBefore);
    previewRenderer.mesh.draw(300,200,280,200,true);
    glGetIntegerv(GL_VIEWPORT,viewportAfter); glGetDoublev(GL_PROJECTION_MATRIX,projectionAfter);
    check(std::equal(viewportBefore,viewportBefore+4,viewportAfter),"preview restores viewport");
    check(std::equal(projectionBefore,projectionBefore+16,projectionAfter),"preview restores projection");
    check(!previewRenderer.mesh.draw(0,0,0,0,false),"zero-size preview safely rejected");
    glEnable(GL_SCISSOR_TEST); glScissor(11,17,113,129); glDepthMask(GL_FALSE);
    using ActiveTexture=void(APIENTRY*)(GLenum);
    auto activeTexture=reinterpret_cast<ActiveTexture>(SDL_GL_GetProcAddress("glActiveTexture"));
    activeTexture(GL_TEXTURE1); glEnable(GL_TEXTURE_2D);
    previewRenderer.mesh.draw(300,200,280,200,false);
    GLint unit=0,scissor[4]; GLboolean depthMask=GL_TRUE;
    glGetIntegerv(GL_ACTIVE_TEXTURE,&unit); glGetIntegerv(GL_SCISSOR_BOX,scissor); glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);
    check(unit==GL_TEXTURE1 && glIsEnabled(GL_TEXTURE_2D),"multitexture state restored");
    check(glIsEnabled(GL_SCISSOR_TEST) && scissor[0]==11 && scissor[1]==17 && scissor[2]==113 && scissor[3]==129,"scissor restored");
    check(depthMask==GL_FALSE,"depth write mask restored");
    glDisable(GL_TEXTURE_2D); activeTexture(GL_TEXTURE0); glDisable(GL_SCISSOR_TEST); glDepthMask(GL_TRUE);
    selectedPage=0;
    const int spring=pages[5].fields[0], peer=pages[5].fields[1], unrelated=pages[6].fields[1];
    const double previous=fields[unrelated].value;
    ChangeField(spring,fields[spring].value+fields[spring].step);
    check(fields[spring].value==fields[peer].value,"symmetry mirrors same parameter");
    check(fields[unrelated].value==previous,"symmetry does not modify another parameter family");
    symmetricFront=false; const double before=fields[peer].value;
    ChangeField(spring,fields[spring].value+fields[spring].step);
    check(fields[peer].value==before,"disabled symmetry preserves other side");
    controls.clear(); BuildEditorControls();
    for(size_t i=0;i<controls.size();++i) if(controls[i].field>=0) {
        auto c=controls[i]; SetSliderFromMouse((int)i,c.left+6,c.bottom);
        check(fields[c.field].value==fields[c.field].minimum,"slider low endpoint maps to minimum");
        SetSliderFromMouse((int)i,c.left+c.width-6,c.bottom);
        check(fields[c.field].value==fields[c.field].maximum,"slider high endpoint maps to maximum");
        selectedControl=(int)i; Keyboard(SDLK_LEFT,0,0,0);
        check(fields[c.field].value<=fields[c.field].maximum,"keyboard adjustment stays bounded");
    }
    Keyboard(SDLK_PAGEDOWN,0,0,0); check(selectedPage==1,"controller shoulder uses four tabs");
    Keyboard(SDLK_F5,0,0,0); check(applied==1,"apply shortcut routes to production action");
    Keyboard(SDLK_ESCAPE,0,0,0); check(cancelled==1,"cancel shortcut routes to production action");
    for(auto dimensions : std::vector<std::pair<int,int>>{{1920,1080},{1280,720},{1024,768}}) {
        previewScreenWidth=dimensions.first; previewScreenHeight=dimensions.second;
        const double scale=std::min(previewScreenWidth/1280.0,previewScreenHeight/800.0);
        viewportWidth=(int)(1280*scale); viewportHeight=(int)(800*scale);
        viewportLeft=(previewScreenWidth-viewportWidth)/2; viewportBottom=(previewScreenHeight-viewportHeight)/2;
        selectedPage=0; draggedControl=-1; controls.clear(); BuildEditorControls();
        int plus=-1;
        for(size_t i=0;i<controls.size();++i) if(controls[i].text=="+") { plus=(int)i; break; }
        const auto c=controls[plus];
        const int px=viewportLeft+(int)((c.left+c.width/2)*viewportWidth/1280);
        const int py=previewScreenHeight-viewportBottom-(int)((c.bottom+c.height/2)*viewportHeight/800);
        const int first=pages[4].fields[1]; ChangeField(first,fields[first].minimum);
        const double before=fields[first].value;
        MouseButton(SDL_BUTTON_LEFT,SDL_PRESSED,px,py);
        check(fields[first].value>before,"scaled mouse click hits visible plus button");
        MouseButton(SDL_BUTTON_LEFT,SDL_RELEASED,px,py);
        MouseMotion(viewportLeft,previewScreenHeight-viewportBottom);
        check(mousePositionX==0 && mousePositionY==0,"letterboxed mouse origin");
    }
    selectedPage=3; controls.clear(); BuildEditorControls();
    for(const auto& c:controls) if(c.text=="RESTABLECER") { auto action=c.action; action(); break; }
    for(const auto& f:fields) check(f.value==f.original,"reset restores original live setup");
    for(const auto& f:garageFonts) check(f.second!=nullptr,"HTML reference font loaded without fallback");
    for(int i=0;i<520;++i) Text("Cache regression "+std::to_string(i),0,0);
    check(textures.size()<=512,"dynamic text cache remains bounded");
fields.clear(); pages.clear(); rustGarage=true;
{ GarageField f; f.path="suspension.front.resting_ratio"; f.name="Front ride height ratio"; f.unit="ratio"; f.scale=1; f.value=f.original=0.27142857142857146; f.minimum=0.20000000000000001; f.maximum=0.80000000000000004; f.step=0.0050000000000000001; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.rear.resting_ratio"; f.name="Rear ride height ratio"; f.unit="ratio"; f.scale=1; f.value=f.original=0.33661538461538459; f.minimum=0.20000000000000001; f.maximum=0.80000000000000004; f.step=0.0050000000000000001; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.front.damping_ratio"; f.name="Front damper ratio"; f.unit="ratio"; f.scale=1; f.value=f.original=0.73999999999999999; f.minimum=0.050000000000000003; f.maximum=1.2; f.step=0.01; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.rear.damping_ratio"; f.name="Rear damper ratio"; f.unit="ratio"; f.scale=1; f.value=f.original=0.70999999999999996; f.minimum=0.050000000000000003; f.maximum=1.2; f.step=0.01; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.front.bump_damp_multiplier"; f.name="Front bump multiplier"; f.unit="ratio"; f.scale=1; f.value=f.original=1; f.minimum=0.20000000000000001; f.maximum=3; f.step=0.01; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.rear.bump_damp_multiplier"; f.name="Rear bump multiplier"; f.unit="ratio"; f.scale=1; f.value=f.original=0.90000000000000002; f.minimum=0.20000000000000001; f.maximum=3; f.step=0.01; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.front.rebound_damp_multiplier"; f.name="Front rebound multiplier"; f.unit="ratio"; f.scale=1; f.value=f.original=1.1200000000000001; f.minimum=0.20000000000000001; f.maximum=3; f.step=0.01; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.rear.rebound_damp_multiplier"; f.name="Rear rebound multiplier"; f.unit="ratio"; f.scale=1; f.value=f.original=1.3500000000000001; f.minimum=0.20000000000000001; f.maximum=3; f.step=0.01; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.geometry_physical.front_arb.ratio"; f.name="Front anti-roll ratio"; f.unit="ratio"; f.scale=1; f.value=f.original=0.14999999999999999; f.minimum=0; f.maximum=2; f.step=0.01; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.geometry_physical.rear_arb.ratio"; f.name="Rear anti-roll ratio"; f.unit="ratio"; f.scale=1; f.value=f.original=0.089999999999999997; f.minimum=0; f.maximum=2; f.step=0.01; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.front.toe"; f.name="Front toe (local)"; f.unit="deg"; f.scale=57.295779513082323; f.value=f.original=-0.1000000000003249; f.minimum=-2.8647889756541165; f.maximum=2.8647889756541165; f.step=0.057295779513082325; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.rear.toe"; f.name="Rear toe (local)"; f.unit="deg"; f.scale=57.295779513082323; f.value=f.original=0; f.minimum=-2.8647889756541165; f.maximum=2.8647889756541165; f.step=0.057295779513082325; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.front.camber"; f.name="Front camber"; f.unit="deg"; f.scale=57.295779513082323; f.value=f.original=-4.0000000000129958; f.minimum=-5.729577951308233; f.maximum=2.8647889756541165; f.step=0.057295779513082325; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.rear.camber"; f.name="Rear camber"; f.unit="deg"; f.scale=57.295779513082323; f.value=f.original=-1.5000000000048734; f.minimum=-5.729577951308233; f.maximum=2.8647889756541165; f.step=0.057295779513082325; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.front.bump_stop_multiplier"; f.name="Front bump stop"; f.unit="ratio"; f.scale=1; f.value=f.original=2.6000000000000001; f.minimum=0; f.maximum=4; f.step=0.050000000000000003; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.rear.bump_stop_multiplier"; f.name="Rear bump stop"; f.unit="ratio"; f.scale=1; f.value=f.original=2.7999999999999998; f.minimum=0; f.maximum=4; f.step=0.050000000000000003; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="brakes.front_brake_bias"; f.name="Front brake bias"; f.unit="fraction"; f.scale=1; f.value=f.original=0.58999999999999997; f.minimum=0.29999999999999999; f.maximum=0.80000000000000004; f.step=0.0050000000000000001; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="brakes.max_brake_torque"; f.name="Maximum brake torque"; f.unit="N m"; f.scale=1; f.value=f.original=4000; f.minimum=500; f.maximum=8000; f.step=50; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="powertrain.differential.preload_nm"; f.name="Differential preload"; f.unit="N m"; f.scale=1; f.value=f.original=90; f.minimum=0; f.maximum=120; f.step=1; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="powertrain.differential.power_ramp_angle_deg"; f.name="Differential power ramp"; f.unit="deg"; f.scale=1; f.value=f.original=65; f.minimum=5; f.maximum=80; f.step=1; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="powertrain.differential.coast_ramp_angle_deg"; f.name="Differential coast ramp"; f.unit="deg"; f.scale=1; f.value=f.original=60; f.minimum=5; f.maximum=80; f.step=1; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="powertrain.differential.clutches"; f.name="Differential clutches"; f.unit="count"; f.scale=1; f.value=f.original=6; f.minimum=1; f.maximum=8; f.step=1; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="powertrain.final_drive"; f.name="Final drive"; f.unit="ratio"; f.scale=1; f.value=f.original=4.6500000000000004; f.minimum=2; f.maximum=6; f.step=0.01; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="powertrain.max_rpm"; f.name="Rev limiter"; f.unit="rpm"; f.scale=1; f.value=f.original=17500; f.minimum=6000; f.maximum=22000; f.step=100; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="aero.model.front_wing.incidence_deg"; f.name="Front wing incidence"; f.unit="deg"; f.scale=1; f.value=f.original=18; f.minimum=0; f.maximum=20; f.step=0.5; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="aero.model.rear_wing.incidence_deg"; f.name="Rear wing incidence"; f.unit="deg"; f.scale=1; f.value=f.original=16; f.minimum=0; f.maximum=20; f.step=0.5; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="steering.max_steering_angle"; f.name="Maximum steering angle"; f.unit="deg"; f.scale=57.295779513082323; f.value=f.original=17.990874767107851; f.minimum=8.5943669269623477; f.maximum=40.10704565915762; f.step=0.28647889756541162; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="steering.steering_speed"; f.name="Steering speed"; f.unit="1/s"; f.scale=1; f.value=f.original=8; f.minimum=1; f.maximum=12; f.step=0.050000000000000003; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="fuel.initial_kg"; f.name="Fuel load"; f.unit="kg"; f.scale=1; f.value=f.original=7.5999999999999996; f.minimum=0; f.maximum=120; f.step=0.5; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="fuel.density_kg_per_litre"; f.name="Fuel density (conversion)"; f.unit="kg/L"; f.scale=1; f.value=f.original=0.75; f.minimum=0; f.maximum=0; f.step=0; f.editable=false; fields.push_back(f); }
{ GarageField f; f.path="tires.pressure.cold_kpa_gauge.FL"; f.name="Front left cold pressure"; f.unit="kPa gauge"; f.scale=1; f.value=f.original=130; f.minimum=80; f.maximum=220; f.step=1; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="tires.pressure.cold_kpa_gauge.FR"; f.name="Front right cold pressure"; f.unit="kPa gauge"; f.scale=1; f.value=f.original=130; f.minimum=80; f.maximum=220; f.step=1; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="tires.pressure.cold_kpa_gauge.RL"; f.name="Rear left cold pressure"; f.unit="kPa gauge"; f.scale=1; f.value=f.original=120; f.minimum=80; f.maximum=220; f.step=1; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="tires.pressure.cold_kpa_gauge.RR"; f.name="Rear right cold pressure"; f.unit="kPa gauge"; f.scale=1; f.value=f.original=120; f.minimum=80; f.maximum=220; f.step=1; f.editable=true; fields.push_back(f); }
{ GarageField f; f.path="suspension.front.spring_length"; f.name="Front spring length (design)"; f.unit="m"; f.scale=1; f.value=f.original=0.34999999999999998; f.minimum=0; f.maximum=0; f.step=0; f.editable=false; fields.push_back(f); }
{ GarageField f; f.path="suspension.rear.spring_length"; f.name="Rear spring length (design)"; f.unit="m"; f.scale=1; f.value=f.original=0.32500000000000001; f.minimum=0; f.maximum=0; f.step=0; f.editable=false; fields.push_back(f); }
{ GarageField f; f.path="tires.front.radius"; f.name="Front tire radius (design)"; f.unit="m"; f.scale=1; f.value=f.original=0.31194899999999998; f.minimum=0; f.maximum=0; f.step=0; f.editable=false; fields.push_back(f); }
{ GarageField f; f.path="tires.rear.radius"; f.name="Rear tire radius (design)"; f.unit="m"; f.scale=1; f.value=f.original=0.32201600000000002; f.minimum=0; f.maximum=0; f.step=0; f.editable=false; fields.push_back(f); }

    viewportWidth=1280; viewportHeight=800; viewportLeft=viewportBottom=0;
    for(int tab=0;tab<4;++tab) {
        selectedPage=tab; controls.clear(); BuildEditorControls();
        for(const auto& c:controls) check(c.left>=0 && c.bottom>=0 && c.left+c.width<=1280 && c.bottom+c.height<=800,"Rust control inside original canvas");
        glFinish(); std::vector<unsigned char> pixels(1280*800*3);
        glReadPixels(0,0,1280,800,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        SDL_Surface* shot=SDL_CreateRGBSurfaceFrom(pixels.data(),1280,800,24,1280*3,0xff,0xff00,0xff0000,0);
        SDL_SaveBMP(shot,(std::string(argv[2])+"/tab-rust-"+std::to_string(tab)+".bmp").c_str()); SDL_FreeSurface(shot);
        if(tab==2) check(imageTextures["engine-gray-v2.png"].identifier!=0,"original engine drawing restored");
        if(tab==3) check(imageTextures["strategy-gray-v2.png"].identifier!=0,"original strategy drawing restored");
    }

    controls.clear(); editorOpen=false;
    glViewport(0,0,1280,800); glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0,640,0,480);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glClear(GL_COLOR_BUFFER_BIT);
    BuildPitControls();
    check(controls.size()==2,"pit window retains two actions");
    for(const auto& c:controls) check(c.left>=120 && c.bottom>=168 && c.left+c.width<=520 && c.bottom+c.height<=383,"pit actions fit styled panel");
    glFinish(); std::vector<unsigned char> pitPixels(1280*800*3);
    glReadPixels(0,0,1280,800,GL_RGB,GL_UNSIGNED_BYTE,pitPixels.data());
    SDL_Surface* pitShot=SDL_CreateRGBSurfaceFrom(pitPixels.data(),1280,800,24,1280*3,0xff,0xff00,0xff0000,0);
    SDL_SaveBMP(pitShot,(std::string(argv[2])+"/tab-pit.bmp").c_str()); SDL_FreeSurface(pitShot);
    ClearGarageText(); check(textures.empty(),"cached OpenGL resources released");
    check(glGetError()==GL_NO_ERROR,"OpenGL rendering has no error");
    std::printf("checks=%d failures=%d\n",checks,failures);
    return failures ? 1 : 0;
}
