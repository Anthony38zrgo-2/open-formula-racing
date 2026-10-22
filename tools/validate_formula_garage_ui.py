"""Render production garage widgets in a hidden SDL context; check input/layout.

This isolates UI rendering from the race engine. It does not certify application
of a setup to running physics or replace manual in-game driving validation.
"""
import argparse
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, default=Path('C:/Users/bill/open-formula-build/engine'))
parser.add_argument('--toolchain', type=Path, default=Path('C:/Users/bill/open-formula-build-tools/msys64/ucrt64'))
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
args.output.mkdir(parents=True, exist_ok=True)
source = (root / 'src/modules/userinterface/legacymenu/racescreens/formula_garage_menu.cpp').read_text()
fonts = (root / 'src/libs/tgfclient/guifont.cpp').read_text()
objects = (root / 'src/libs/tgfclient/guiobject.cpp').read_text()
prefix = '''
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
'''
font_code = fonts[fonts.index('GfuiFontClass::GfuiFontClass('):]
draw_string = objects[objects.index('void GfuiDrawString('):objects.index('int GfuiFontHeight(')]
draw_string = draw_string.replace('GfuiDrawString(', 'PreviewDrawString(')
globals_code = source[source.index('constexpr double radiansPerDegree'):source.index('void DrawGarage();')]
primitives = source[source.index('void Rectangle('):source.index('std::string Number(')]
primitives = primitives.replace('GfuiDrawString(', 'PreviewDrawString(')
widgets = source[source.index('std::string Number('):source.index('void DrawGarage()\n{')]
mouse = source[source.index('void SetSliderFromMouse('):source.index('void MouseMotion(')]
keyboard = source[source.index('void Keyboard('):source.index('void PollController(')]
mouse_events = source[source.index('void MouseMotion('):source.index('void Keyboard(')]
mouse_events = mouse_events.replace('GfScrGetSize(', 'PreviewScreenSize(')
fake = '''
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
struct PreviewEngine { tRmInfo* inData() { return &previewRace; } };
PreviewEngine& LmRaceEngine() { static PreviewEngine engine; return engine; }
static int applied = 0, cancelled = 0;
void ApplyEditor() { ++applied; }
void CancelEditor() { ++cancelled; }
void OpenEditor() {}
static int previewScreenWidth=1280, previewScreenHeight=800;
void PreviewScreenSize(int* sw,int* sh,int* vw,int* vh) {
    *sw=*vw=previewScreenWidth; *sh=*vh=previewScreenHeight;
}
'''
main = r'''
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
    ClearGarageText(); check(textures.empty(),"cached OpenGL resources released");
    check(glGetError()==GL_NO_ERROR,"OpenGL rendering has no error");
    std::printf("checks=%d failures=%d\n",checks,failures);
    return failures ? 1 : 0;
}
'''
cpp = args.output / 'garage_ui_bench.cpp'
cpp.write_text(prefix + font_code + draw_string + globals_code + fake + primitives + widgets + mouse + mouse_events + keyboard + main)
env = os.environ.copy()
env['PATH'] = os.pathsep.join([str(args.toolchain / 'bin'), str(args.build / 'bin'), env.get('PATH', '')])
include = [args.build, root/'src/interfaces', root/'src/libs/math', root/'src/libs/tgf', root/'src/libs/tgfclient',
           root/'src/modules/userinterface/legacymenu/racescreens',
           root/'src/modules/graphic/osggraph/Car',root/'src/modules/graphic/osggraph/Loader',
           root/'src/libs/portability', args.build.parent/'dependencies/include', args.toolchain/'include/SDL2']
exe = args.output / 'garage_ui_bench.exe'
loader_objects = args.build/'src/modules/graphic/osggraph/CMakeFiles/osggraph.dir/Loader'
command = [str(args.toolchain/'bin/g++.exe'), '-std=c++11', '-DHAVE_CONFIG_H', '-DTRACE_LEVEL=5',
           *['-I'+str(p) for p in include], str(cpp),
           str(root/'src/modules/graphic/osggraph/Car/OsgGaragePreview.cpp'),
           str(root/'src/modules/userinterface/legacymenu/racescreens/formula_garage_music.cpp'),
           *[str(loader_objects/(name+'.cpp.obj')) for name in ['OsgLoader','ReaderWriterACC','AccGeode','AccException']],
           str(args.build/'src/libs/tgf/libtgf.dll.a'),str(args.build/'src/libs/tgfclient/libtgfclient.dll.a'),
           '-losgDB','-losgUtil','-losg','-lOpenThreads',
           '-lSDL2_ttf', '-lSDL2_mixer', '-lSDL2', '-lopengl32', '-lglu32', '-o', str(exe)]
subprocess.run(command, env=env, check=True)
result = subprocess.run([str(exe), str(root), str(args.output.resolve()), str(args.build)], env=env, capture_output=True, text=True)
(args.output/'validation.log').write_text(result.stdout + result.stderr)
print(result.stdout, end='')
if result.returncode:
    raise SystemExit(result.returncode)
from PIL import Image
for bmp in args.output.glob('tab-*.bmp'):
    with Image.open(bmp) as im:
        im.transpose(Image.Transpose.FLIP_TOP_BOTTOM).save(bmp.with_suffix('.png'))
