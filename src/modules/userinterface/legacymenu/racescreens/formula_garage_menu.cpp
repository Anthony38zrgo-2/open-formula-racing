#include <glfeatures.h>
#include "formula_garage_menu.h"
#include "formula_garage_music.h"
#include "legacymenu.h"
#include "racescreens.h"
#include <tgfclient.h>
#include <robottools.h>
#include <iphysicsengine.h>
#include <icarpreview.h>
#include <musicplayer.h>
#include <sound.h>
#include <SDL.h>
#include <SDL_ttf.h>
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

namespace
{
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

void DrawGarage();
void ShowPit();
void OpenEditor();
void BeginDriving();
void ApplyEditor();
void CancelEditor();

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
    float* color = turquoise, int alignment = GFUI_ALIGN_HC)
{
    if (editorOpen && !text.empty())
    {
        if (!garageFonts.count(font))
        {
            const char* file = font == GFUI_FONT_BIG || font == GFUI_FONT_MEDIUM
                ? "ChakraPetch-Bold.ttf" : "Rajdhani-SemiBold.ttf";
            const int size = font == GFUI_FONT_BIG ? 36 : font == GFUI_FONT_MEDIUM ? 19 : 15;
            const std::string path = std::string(GfDataDir()) + "data/fonts/formula-garage/" + file;
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
            const double width = texture.width * .5, height = texture.height * .5;
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
    GfuiDrawString(text.c_str(), color, font, (int)horizontal, (int)vertical, 0, alignment);
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
    Text(title, x + 14, y + height - 28, GFUI_FONT_MEDIUM, gold, GFUI_ALIGN_HL);
    Text(tag, x + width - 14, y + height - 26, GFUI_FONT_SMALL_C, muted, GFUI_ALIGN_HR);
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
        CarDiagram(638, selectedPage == 0 ? 287 : 315);
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
void DrawGarage()
{
    if (!garageOpen) return;
    garageMusic.update(getMusicVolume());
    if (!editorOpen && LegacyMenu::self().graphicsEngine())
        LegacyMenu::self().redrawGraphicsView(LmRaceEngine().inData()->s);
    int screenWidth, screenHeight, viewWidth, viewHeight;
    GfScrGetSize(&screenWidth, &screenHeight, &viewWidth, &viewHeight);
    const int canvasWidth = editorOpen ? editorWidth : 640;
    const int canvasHeight = editorOpen ? editorHeight : 480;
    const double scale = std::min((double)viewWidth / canvasWidth, (double)viewHeight / canvasHeight);
    viewportWidth = (int)(canvasWidth * scale); viewportHeight = (int)(canvasHeight * scale);
    viewportLeft = (screenWidth - viewportWidth) / 2; viewportBottom = (screenHeight - viewportHeight) / 2;
    static auto selectProgram = reinterpret_cast<void(APIENTRY*)(GLuint)>(SDL_GL_GetProcAddress("glUseProgram"));
    static auto selectTexture = reinterpret_cast<void(APIENTRY*)(GLenum)>(SDL_GL_GetProcAddress("glActiveTexture"));
    if (selectProgram) selectProgram(0);
    if (selectTexture) selectTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (editorOpen) { glViewport(0, 0, screenWidth, screenHeight); glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT); }
    glViewport(viewportLeft, viewportBottom, viewportWidth, viewportHeight);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0, canvasWidth, 0, canvasHeight);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glMatrixMode(GL_TEXTURE); glLoadIdentity(); glMatrixMode(GL_MODELVIEW);
    controls.clear();
    if (editorOpen) BuildEditorControls();
    else
    {
        float panel[4] = {0, 0, 0, 0.72f};
        Rectangle(105, 210, 430, 170, panel);
        Text("PIT", 320, 350, GFUI_FONT_BIG, white);
        for (int side = 0; side < 2; ++side)
            for (int segment = 0; segment < 6; ++segment)
            {
                const int widths[6] = {40, 24, 16, 11, 7, 3};
                int offset = 0; for (int previous = 0; previous < segment; ++previous) offset += widths[previous] + 4;
                Rectangle(side == 0 ? 126 + offset : 514 - offset - widths[segment], 354, widths[segment], 13, gold);
            }
        Text("BEGIN", 320, 312, GFUI_FONT_MEDIUM, turquoise);
        Text("GARAGE", 320, 280, GFUI_FONT_MEDIUM, turquoise);
        AddButton(265, 306, 110, 27, "Begin", BeginDriving);
        AddButton(265, 273, 110, 27, "Garage", OpenEditor);
    }
    selectedControl = std::max(0, std::min(selectedControl, (int)controls.size() - 1));
    if (!controls.empty())
    {
        const GarageControl& control = controls[selectedControl];
        Outline(control.left, control.bottom, control.width, control.height, gold);
    }
    if (!statusMessage.empty()) Text(statusMessage, canvasWidth / 2, canvasHeight - 18, GFUI_FONT_SMALL_C, white);
    glDisable(GL_TEXTURE_2D);
    glColor4fv(white);
    glBegin(GL_TRIANGLES);
    glVertex2i(mousePositionX, mousePositionY);
    glVertex2i(mousePositionX + 9, mousePositionY - 5);
    glVertex2i(mousePositionX + 3, mousePositionY - 11);
    glEnd();
    GfuiSwapBuffers();
    GfuiApp().eventLoop().postRedisplay();
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
    GfScrGetSize(&screenWidth, &screenHeight, &viewWidth, &viewHeight);
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
void PollController()
{
    if (!garageOpen) return;
    if (!menuController)
        for (int index = 0; index < SDL_NumJoysticks(); ++index)
            if (SDL_IsGameController(index)) { menuController = SDL_GameControllerOpen(index); break; }
    if (!menuController) return;
    SDL_GameControllerUpdate();
    unsigned buttons = 0;
    for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button)
        if (SDL_GameControllerGetButton(menuController, (SDL_GameControllerButton)button)) buttons |= 1u << button;
    const unsigned pressed = buttons & ~previousControllerButtons;
    previousControllerButtons = buttons;
    const struct { int button, key; } mappings[] = {
        {SDL_CONTROLLER_BUTTON_A, SDLK_RETURN}, {SDL_CONTROLLER_BUTTON_B, SDLK_ESCAPE},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, SDLK_UP}, {SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDLK_DOWN},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDLK_LEFT}, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDLK_RIGHT},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDLK_PAGEUP}, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDLK_PAGEDOWN},
        {SDL_CONTROLLER_BUTTON_START, SDLK_F5}
    };
    for (const auto& mapping : mappings)
        if (pressed & (1u << mapping.button)) Keyboard(mapping.key, 0, 0, 0);
    const unsigned now = SDL_GetTicks();
    if (now - lastControllerEvent > 150)
    {
        const int horizontal = SDL_GameControllerGetAxis(menuController, SDL_CONTROLLER_AXIS_LEFTX);
        const int vertical = SDL_GameControllerGetAxis(menuController, SDL_CONTROLLER_AXIS_LEFTY);
        if (std::abs(horizontal) > 20000) { Keyboard(horizontal < 0 ? SDLK_LEFT : SDLK_RIGHT, 0, 0, 0); lastControllerEvent = now; }
        else if (std::abs(vertical) > 20000) { Keyboard(vertical < 0 ? SDLK_UP : SDLK_DOWN, 0, 0, 0); lastControllerEvent = now; }
    }
}
void ActivateGarageScreen()
{
    if (!garageScreen)
    {
        garageScreen = GfuiScreenCreate(nullptr, nullptr, nullptr, nullptr, nullptr, 0);
        GfuiScreenAddMusic(garageScreen, "None");
    }
    GfuiScreenActivate(garageScreen);
    GfuiApp().eventLoop().setKeyboardDownCB(Keyboard);
    GfuiApp().eventLoop().setKeyboardUpCB(nullptr);
    GfuiApp().eventLoop().setMouseButtonCB(MouseButton);
    GfuiApp().eventLoop().setMouseMotionCB(MouseMotion);
    GfuiApp().eventLoop().setMousePassiveMotionCB(MouseMotion);
    GfuiApp().eventLoop().setRedisplayCB(DrawGarage);
    GfuiApp().eventLoop().setRecomputeCB(PollController);
    GfuiApp().eventLoop().postRedisplay();
}
void ResetPhysics(bool relocate)
{
    tRmInfo* information = LmRaceEngine().inData();
    LmRaceEngine().stop();
    if (relocate)
    {
        garageCar->_DynGC = {};
        garageCar->_pos_X = garagePositionX;
        garageCar->_pos_Y = garagePositionY;
        garageCar->_pos_Z = garageGroundHeight + garageCar->_statGC_z + 0.05;
        garageCar->_yaw = garageHeading;
        RtTrackGlobal2Local(information->track->seg, garagePositionX, garagePositionY, &garageCar->_trkPos, TR_LPOS_MAIN);
    }
    else
    {
        garageCar->_DynGC.vel = {};
        garageCar->_DynGC.acc = {};
    }
    garageCar->ctrl = {};
    garageCar->ctrl.brakeCmd = 1.0;
    garageCar->ctrl.clutchCmd = 1.0;
    garageCar->_gear = garageCar->_gearNext = 0;
    garageCar->_state &= ~(RM_CAR_STATE_PIT | RM_CAR_STATE_BROKEN | RM_CAR_STATE_OUT | RM_CAR_STATE_OUTOFGAS | RM_CAR_STATE_ELIMINATED);
    garageCar->_commitBestLapTime = false;
    information->_reCarInfo[garageCar->index].prevTrkPos = garageCar->_trkPos;
    information->_reCarInfo[garageCar->index].lapFlag = 0;
    garageCar->pitcmd.fuel = 0;
    garageCar->pitcmd.repair = 0;
    garageCar->pitcmd.setupChanged = true;
    garageCar->pitcmd.tireChange = replaceTires ? tCarPitCmd::ALL : tCarPitCmd::NONE;
    LmRaceEngine().applyGarageSetup(garageCar->index);
    information->_rePitRequester = nullptr;
    if (garageCar->_pit) garageCar->_pit->pitCarIndex = TR_PIT_STATE_FREE;
}
void ShowPit()
{
    garageMusic.stop();
    editorOpen = false; selectedControl = 0; draggedControl = -1;
    ActivateGarageScreen();
    GfLogInfo("Formula garage state: PIT, controls locked, x=%.3f y=%.3f\n", garageCar->_pos_X, garageCar->_pos_Y);
}
void OpenEditor()
{
    if (auto* preview = dynamic_cast<ICarPreviewRenderer*>(LegacyMenu::self().graphicsEngine()))
        preview->resetGarageCarPreview();
    BuildPages(); replaceTires = false;
    editorOpen = true; selectedPage = 0; selectedControl = 0; draggedControl = -1;
    statusMessage.clear();
    GfLogInfo("Formula garage state: GARAGE, reading current live setup\n");
    void* sound = GfParmReadFileLocal(SND_PARAM_FILE, GFPARM_RMODE_STD);
    const bool enabled = !std::strcmp(GfParmGetStr(sound, SND_SCT_MUSIC,
        SND_ATT_MUSIC_STATE, SND_VAL_MUSIC_STATE_DISABLED), SND_VAL_MUSIC_STATE_ENABLED);
    if (sound) GfParmReleaseHandle(sound);
    if (enabled) garageMusic.start(std::string(GfDataDir()) + "data/music/formula-garage/run.mp3", getMusicVolume());
}
void ApplyEditor()
{
    if (rustGarage) {
        IPhysicsEngine* physics = LmRaceEngine().garagePhysics();
        std::vector<std::pair<std::string, double> > values;
        for (const GarageField& field : fields)
            if (field.editable && field.value != field.original) values.push_back({field.path, field.value / field.scale});
        std::string error;
        if (!physics || !physics->applyGarageFields(garageCar->index, values, error)) {
            statusMessage = error.empty() ? "Rust setup unavailable" : error;
            GfLogWarning("Rust garage rejected: %s\n", statusMessage.c_str());
            return;
        }
        ResetPhysics(false);
        BuildPages();
        statusMessage = "Rust setup applied";
        ShowPit();
        return;
    }
    const char* pressureNames[4] = {"front right", "front left", "rear right", "rear left"};
    for (GarageField& field : fields)
    {
        if (!std::isfinite(field.value) || field.value < field.minimum || field.value > field.maximum)
        { statusMessage = "Setup rejected: invalid " + field.name; return; }
    }
    for (GarageField& field : fields)
    {
        if (field.target)
        {
            field.target->min = field.minimum / field.scale;
            field.target->max = field.maximum / field.scale;
            field.target->desired_value = field.value / field.scale;
            field.target->changed = true;
        }
        else if (field.kind == 1)
            GfParmSetNum(garageCar->_carHandle, "Formula Tires", (std::string("cold pressure ") + pressureNames[field.wheel]).c_str(), "Pa", field.value * 1000);
        else if (field.kind == 2)
            GfParmSetNum(garageCar->_carHandle, SECT_REARDIFFERENTIAL, "preload torque", nullptr, field.value);
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        tCarSetupItem& packer = garageCar->setup.suspPacker[wheel];
        packer.desired_value = std::min((double)packer.desired_value, (double)garageCar->setup.rideHeight[wheel].desired_value - 0.005);
        packer.changed = true;
    }
    ResetPhysics(false);
    statusMessage = "Setup applied";
    GfLogInfo("Formula garage setup: APPLIED fields=%d fresh_tyres=%d\n", (int)fields.size(), replaceTires ? 1 : 0);
    ShowPit();
}
void CancelEditor()
{
    fields.clear(); pages.clear(); replaceTires = false; statusMessage = "Changes discarded"; ShowPit();
}
void BeginDriving()
{
    garageMusic.stop();
    garageOpen = false; editorOpen = false; arrivalArmed = false;
    garageCar->ctrl = {};
    garageCar->_state &= ~RM_CAR_STATE_PIT;
    LmRaceEngine().inData()->_reState = RE_STATE_RACE;
    statusMessage.clear();
    GfLogInfo("Formula garage state: DRIVING, controls unlocked\n");
    LegacyMenu::self().activateGameScreen();
}
}

bool FormulaGarageAvailable()
{
    const tRmInfo* information = LmRaceEngine().inData();
    return information && information->track && information->s && information->s->_raceType == RM_TYPE_PRACTICE
        && information->track->internalname && std::strcmp(information->track->internalname, "fuji-seventy-six-seventy-seven") == 0
        && information->s->_ncars == 1 && information->s->cars[0]->_driverType == RM_DRV_HUMAN;
}
void FormulaGarageSessionStart()
{
    FormulaGarageShutdown();
    // Scripted sessions can start directly on the track: the pit garage is then only
    // opened on demand, not on the initial arrival check.
    bool openOnStart = true;
    const std::string path = std::string(GfLocalDir()) + "config/rust_physics.xml";
    void* configuration = GfParmReadFile(path.c_str(), GFPARM_RMODE_STD);
    if (configuration)
    {
        openOnStart = GfParmGetNum(configuration, "RustPhysics", "open garage at session start",
                                   nullptr, 1.0) > 0.0;
        GfParmReleaseHandle(configuration);
    }
    initialArrivalPending = openOnStart && FormulaGarageAvailable();
    arrivalArmed = false;
}
bool FormulaGarageCheckArrival()
{
    if (!FormulaGarageAvailable() || garageOpen) return false;
    tCarElt* car = LmRaceEngine().inData()->s->cars[0];
    const double distance = std::hypot(car->_pos_X - garagePositionX, car->_pos_Y - garagePositionY);
    if (distance > 8.0) arrivalArmed = true;
    const bool stoppedInSlot = arrivalArmed && distance < 3.2 && std::hypot(car->_speed_X, car->_speed_Y) < 0.8;
    if (!initialArrivalPending && !stoppedInSlot) return false;
    initialArrivalPending = false;
    FormulaGarageReturnToPit(nullptr);
    return true;
}
void FormulaGarageReturnToPit(void*)
{
    if (!FormulaGarageAvailable()) return;
    garageCar = LmRaceEngine().inData()->s->cars[0];
    LmRaceEngine().stop();
    if (LegacyMenu::self().soundEngine()) LegacyMenu::self().soundEngine()->mute(true);
    replaceTires = false; garageOpen = true; arrivalArmed = false; initialArrivalPending = false;
    ResetPhysics(true);
    LmRaceEngine().inData()->_reState = RE_STATE_RACE;
    ShowPit();
}
void FormulaGarageShutdown()
{
    garageMusic.shutdown();
    garageOpen = editorOpen = initialArrivalPending = arrivalArmed = false;
    garageCar = nullptr;
    fields.clear(); pages.clear(); controls.clear();
    if (garageScreen) { GfuiScreenRelease(garageScreen); garageScreen = nullptr; }
    ClearGarageText();
    for (const auto& entry : imageTextures)
        if (entry.second.identifier) GfTexFreeTexture(entry.second.identifier);
    imageTextures.clear();
    for (const auto& entry : garageFonts) if (entry.second) TTF_CloseFont(entry.second);
    garageFonts.clear();
    if (menuController) { SDL_GameControllerClose(menuController); menuController = nullptr; }
    previousControllerButtons = 0;
    statusMessage.clear();
}
