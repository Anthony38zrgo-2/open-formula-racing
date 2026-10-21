#include <glfeatures.h>
#include "formula_garage_menu.h"
#include "legacymenu.h"
#include "racescreens.h"
#include <tgfclient.h>
#include <robottools.h>
#include <iphysicsengine.h>
#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
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
float turquoise[4] = {0.27f, 0.72f, 0.75f, 1.0f};
float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
float gold[4] = {1.0f, 0.84f, 0.05f, 1.0f};
void* garageScreen = nullptr;
tCarElt* garageCar = nullptr;
bool initialArrivalPending = false;
bool arrivalArmed = false;
bool garageOpen = false;
bool editorOpen = false;
bool symmetricFront = true;
bool symmetricRear = true;
bool replaceTires = false;
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
void Text(const std::string& text, double horizontal, double vertical, int font = GFUI_FONT_SMALL,
    float* color = turquoise, int alignment = GFUI_ALIGN_HC)
{
    GfuiDrawString(text.c_str(), color, font, (int)horizontal, (int)vertical, 0, alignment);
}
void Image(const std::string& name, double left, double bottom, double width, double height)
{
    if (!textures.count(name))
    {
        GarageTexture texture;
        std::string path = std::string(GfDataDir()) + "data/img/formula-garage/" + name;
        texture.identifier = GfTexReadTexture(path.c_str(), &texture.width, &texture.height,
            &texture.paddedWidth, &texture.paddedHeight);
        textures[name] = texture;
    }
    const GarageTexture& texture = textures[name];
    if (!texture.identifier) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture.identifier);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1, 1, 1, 1);
    const double maximumHorizontal = (double)texture.width / texture.paddedWidth;
    const double minimumVertical = 1.0 - (double)texture.height / texture.paddedHeight;
    glBegin(GL_QUADS);
    glTexCoord2d(0, minimumVertical); glVertex2d(left, bottom);
    glTexCoord2d(maximumHorizontal, minimumVertical); glVertex2d(left + width, bottom);
    glTexCoord2d(maximumHorizontal, 1); glVertex2d(left + width, bottom + height);
    glTexCoord2d(0, 1); glVertex2d(left, bottom + height);
    glEnd();
    glDisable(GL_TEXTURE_2D);
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
}
void ChangeField(int fieldIndex, double value)
{
    GarageField& field = fields[fieldIndex];
    field.value = std::max(field.minimum, std::min(field.maximum,
        std::round(value / field.step) * field.step));
    if (field.wheel >= 0 && (field.wheel < 2 ? symmetricFront : symmetricRear))
    {
        for (int counterpart : pages[selectedPage].fields)
            if (fields[counterpart].wheel == (field.wheel ^ 1))
                fields[counterpart].value = std::max(fields[counterpart].minimum,
                    std::min(fields[counterpart].maximum, field.value));
    }
}
void DrawSlider(int fieldIndex, double horizontal, double vertical, double length, bool upright)
{
    GarageField& field = fields[fieldIndex];
    const double proportion = (field.value - field.minimum) / std::max(field.maximum - field.minimum, 1.0e-9);
    if (upright)
    {
        Rectangle(horizontal - 1, vertical, 2, length, turquoise);
        Rectangle(horizontal - 7, vertical, 14, 2, turquoise);
        Rectangle(horizontal - 7, vertical + length, 14, 2, turquoise);
        Image("SimuRegleCurseurH.png", horizontal - 8, vertical + length * proportion - 5, 16, 10);
        controls.push_back({horizontal - 13, vertical - 6, 26, length + 12, field.name, fieldIndex, true, {}});
        Text(field.name, horizontal, vertical + length + 29, GFUI_FONT_SMALL_C);
        Text(Number(field.maximum, ""), horizontal, vertical + length + 10, GFUI_FONT_SMALL_C);
        Text(Number(field.value, field.unit), horizontal, vertical - 26, GFUI_FONT_SMALL_C);
    }
    else
    {
        Rectangle(horizontal, vertical - 1, length, 2, turquoise);
        Rectangle(horizontal, vertical - 7, 2, 14, turquoise);
        Rectangle(horizontal + length, vertical - 7, 2, 14, turquoise);
        Image("SimuRegleCurseur__simureglecurseur.png", horizontal + length * proportion - 5, vertical - 9, 10, 18);
        controls.push_back({horizontal - 6, vertical - 12, length + 12, 24, field.name, fieldIndex, false, {}});
        Text(field.name, horizontal + length * 0.5, vertical + 19, GFUI_FONT_SMALL);
        Text(Number(field.value, field.unit), horizontal + length * 0.5, vertical - 28, GFUI_FONT_MEDIUM_C);
    }
}
void BuildEditorControls()
{
    Image("SimuFondGarage.png", 0, 0, 640, 480);
    const GaragePage& page = pages[selectedPage];
    Text("OK", 51, 426, GFUI_FONT_MEDIUM, turquoise);
    AddButton(24, 418, 58, 40, "Apply setup", ApplyEditor);
    const char* categoryIcons[] = {"Aero", "Steering", "Brakes", "Gear", "Tyres", "Quant"};
    const int categoryPages[] = {0, 12, 13, 14, 15, 16};
    for (int category = 0; category < 6; ++category)
    {
        const int pageIndex = categoryPages[category];
        Image(std::string("SimuIcon") + categoryIcons[category] + (pageIndex == selectedPage ? "Ena.png" : "Dis.png"),
            35, 330 - category * 52, 35, 35);
        AddButton(30, 325 - category * 52, 46, 44, pages[pageIndex].title,
            [pageIndex]() { selectedPage = pageIndex; selectedControl = 0; });
    }
    const int secondaryPages[] = {0, 1, 2, 4, 5, 11};
    for (int secondary = 0; secondary < 6; ++secondary)
    {
        const int pageIndex = secondaryPages[secondary];
        Image("SimuIcon" + pages[pageIndex].icon + (pageIndex == selectedPage ? "Ena.png" : "Dis.png"),
            96, 365 - secondary * 52, 37, 37);
        AddButton(92, 360 - secondary * 52, 46, 45, pages[pageIndex].title,
            [pageIndex]() { selectedPage = pageIndex; selectedControl = 0; });
    }
    Image(page.image, 185, 70, 415, 320);
    Text(page.title, 600, 426, GFUI_FONT_BIG, white, GFUI_ALIGN_HR);
    if (page.vertical)
    {
        const double horizontal[4] = {300, 237, 539, 475};
        for (size_t wheel = 0; wheel < page.fields.size(); ++wheel)
            DrawSlider(page.fields[wheel], horizontal[wheel], 185, 112, true);
        Text("symmetry", 386, 257, GFUI_FONT_SMALL_C, white);
        Text(symmetricFront ? "Front: ON" : "Front: OFF", 386, 235, GFUI_FONT_SMALL_C);
        Text(symmetricRear ? "Rear: ON" : "Rear: OFF", 386, 211, GFUI_FONT_SMALL_C);
        AddButton(345, 230, 85, 23, "Front symmetry", []() { symmetricFront = !symmetricFront; });
        AddButton(345, 204, 85, 23, "Rear symmetry", []() { symmetricRear = !symmetricRear; });
    }
    else
    {
        const bool many = page.fields.size() > 3;
        for (size_t index = 0; index < page.fields.size(); ++index)
        {
            if (many)
                DrawSlider(page.fields[index], 222, 350 - index * 37, 170, false);
            else if (selectedPage == 0)
                DrawSlider(page.fields[index], index == 0 ? 215 : 445, index == 0 ? 350 : 190, 120, false);
            else
                DrawSlider(page.fields[index], 250, 335 - index * 87, 280, false);
        }
    }
    if (selectedPage == 15)
    {
        Text(replaceTires ? "FIT NEW TYRES: YES" : "FIT NEW TYRES: NO", 450, 117, GFUI_FONT_SMALL_C, white);
        AddButton(370, 105, 190, 28, "Fit new tyres", []() { replaceTires = !replaceTires; });
        char state[160];
        std::snprintf(state, sizeof(state), "Current tread: %.0f / %.0f / %.0f / %.0f C",
            garageCar->_tyreT_mid(0) - 273.15, garageCar->_tyreT_mid(1) - 273.15,
            garageCar->_tyreT_mid(2) - 273.15, garageCar->_tyreT_mid(3) - 273.15);
        Text(state, 400, 89, GFUI_FONT_SMALL_C, white);
    }
    Text("<", 165, 421, GFUI_FONT_MEDIUM, gold);
    Text(">", 195, 421, GFUI_FONT_MEDIUM, gold);
    AddButton(150, 410, 30, 30, "Previous setup page", []() { selectedPage = (selectedPage + pages.size() - 1) % pages.size(); selectedControl = 0; });
    AddButton(182, 410, 30, 30, "Next setup page", []() { selectedPage = (selectedPage + 1) % pages.size(); selectedControl = 0; });
    Text("RESET", 395, 63, GFUI_FONT_SMALL_C);
    AddButton(360, 55, 70, 24, "Reset this page", []() { for (int field : pages[selectedPage].fields) fields[field].value = fields[field].original; });
    Text("B", 602, 75, GFUI_FONT_MEDIUM, white);
    Text("back", 602, 58, GFUI_FONT_SMALL_C);
    Text("A", 602, 41, GFUI_FONT_MEDIUM, gold);
    Text("valid", 602, 23, GFUI_FONT_SMALL_C);
    AddButton(581, 55, 42, 39, "Discard changes", CancelEditor);
    AddButton(581, 15, 42, 39, "Apply setup", ApplyEditor);
    Text(page.information, 380, 6, GFUI_FONT_SMALL_C, white);
}
void DrawGarage()
{
    if (!garageOpen) return;
    if (!editorOpen && LegacyMenu::self().graphicsEngine())
        LegacyMenu::self().redrawGraphicsView(LmRaceEngine().inData()->s);
    int screenWidth, screenHeight, viewWidth, viewHeight;
    GfScrGetSize(&screenWidth, &screenHeight, &viewWidth, &viewHeight);
    const double scale = std::min(viewWidth / 640.0, viewHeight / 480.0);
    viewportWidth = (int)(640 * scale); viewportHeight = (int)(480 * scale);
    viewportLeft = (screenWidth - viewportWidth) / 2; viewportBottom = (screenHeight - viewportHeight) / 2;
    static auto selectProgram = reinterpret_cast<void(APIENTRY*)(GLuint)>(SDL_GL_GetProcAddress("glUseProgram"));
    static auto selectTexture = reinterpret_cast<void(APIENTRY*)(GLenum)>(SDL_GL_GetProcAddress("glActiveTexture"));
    if (selectProgram) selectProgram(0);
    if (selectTexture) selectTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_CULL_FACE); glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (editorOpen) { glViewport(0, 0, screenWidth, screenHeight); glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT); }
    glViewport(viewportLeft, viewportBottom, viewportWidth, viewportHeight);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0, 640, 0, 480);
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
    if (!statusMessage.empty()) Text(statusMessage, 320, 465, GFUI_FONT_SMALL_C, white);
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
    mousePositionX = (horizontal - viewportLeft) * 640 / std::max(viewportWidth, 1);
    mousePositionY = (screenHeight - vertical - viewportBottom) * 480 / std::max(viewportHeight, 1);
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
        if (editorOpen) { selectedPage = (selectedPage + (key == SDLK_PAGEUP ? pages.size() - 1 : 1)) % pages.size(); selectedControl = 0; }
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
    if (!garageScreen) garageScreen = GfuiScreenCreate(nullptr, nullptr, nullptr, nullptr, nullptr, 0);
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
    editorOpen = false; selectedControl = 0; draggedControl = -1;
    ActivateGarageScreen();
    GfLogInfo("Formula garage state: PIT, controls locked, x=%.3f y=%.3f\n", garageCar->_pos_X, garageCar->_pos_Y);
}
void OpenEditor()
{
    BuildPages(); replaceTires = false;
    editorOpen = true; selectedPage = 0; selectedControl = 0; draggedControl = -1;
    statusMessage.clear();
    GfLogInfo("Formula garage state: GARAGE, reading current live setup\n");
}
void ApplyEditor()
{
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
    initialArrivalPending = FormulaGarageAvailable();
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
    garageOpen = editorOpen = initialArrivalPending = arrivalArmed = false;
    garageCar = nullptr;
    fields.clear(); pages.clear(); controls.clear();
    if (garageScreen) { GfuiScreenRelease(garageScreen); garageScreen = nullptr; }
    for (const auto& entry : textures) if (entry.second.identifier) GfTexFreeTexture(entry.second.identifier);
    textures.clear();
    if (menuController) { SDL_GameControllerClose(menuController); menuController = nullptr; }
    previousControllerButtons = 0;
    statusMessage.clear();
}
