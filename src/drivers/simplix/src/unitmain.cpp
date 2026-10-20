//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitmain.cpp
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version	2.X	simuV4
//--------------------------------------------------------------------------*
// Interface to	SPEED-DREAMS
//
// File			:	unitmain.cpp
// Created		: 2008.01.27
// Last	changed	: 2014.11.29
// Copyright	: � 2007-2014 Wolf-Dieter Beelitz
// eMail		:	wdbee@users.sourceforge.net
// Version		: 4.05.000
//--------------------------------------------------------------------------*
// V4.05.000:
// New code	for	additional features	of simuv4
//--------------------------------------------------------------------------*
// V4.01.000:
// New code	for	avoiding and overtaking
//--------------------------------------------------------------------------*
// V4.00.002:
// Modifications for Supercars
// Slow	down at	narrow turns more than normal (Needed for Aalborg)
// Individual parameter	SlowRadius
// Limit side use to inner side	to halve of	outer side
//--------------------------------------------------------------------------*
// V4.00.000 (SimuV4)(Single Wheel Braking,	Air	Brake):
// New Logging used
// Rescaled	braking	to allow lower max brake pressure
//--------------------------------------------------------------------------*
// V3.06.000 (SimuV2.1)(Genetic	Parameter Optimisation):
// Additional parameter	to control loading of racinglines
// Renamed parameter "start	fuel" to "initial fuel"
// Deleted old teammanager
// Clear old racingline	if optimisation	is working
// Switch off skilling if optimisation is working
// Deleted old TORCS related code
//--------------------------------------------------------------------------*
// V3.05.001 (SimuV2.1):
// Separated hairpin calculations
// Driving while rain with ls2-bavaria-g3gtr
//--------------------------------------------------------------------------*
// V3.04.000 (SimuV2.1):
// Skilling	for	career mode
//--------------------------------------------------------------------------*
// V3.03.000 (SimuV2.1):
// Braking for GP36	cars
//--------------------------------------------------------------------------*
// V3.02.000 (SimuV2.1):
// Reworked	racingline calculation
//--------------------------------------------------------------------------*
// V3.01.001 (SimuV2.1):
// Reworked	racingline structure to	reduce file	size (83% less).
// Support for ls2 cars
//
// V3.01.000 (SimuV2.1):
// Needed changes to be	able to	control	cars for simuV2.1
// Totally reworked	pitting
// - Still work	in progress
//--------------------------------------------------------------------------*
// V2.00.01	(Speed Dreams -	Career mode):
// Uses	new	Speed Dreams Interfaces	and	was	extended to	use	career mode
//--------------------------------------------------------------------------*
// V2.00 (Speed	Dreams):
// Uses	new	Speed Dreams Interfaces
//--------------------------------------------------------------------------*
// V1.10:
// Features	of the advanced	TORCS Interface:
// Initialization runs once	only, see "simplix(tModInfo	*ModInfo)"
// Allways gives back the names	of drivers as defined in teams xml file!
// Checks and handles pitsharing state enabled/disabled	for	endurance races.
//
// Eigenschaften des erweiterten TORCS Interfaces:
// Die Initialisierung wird	nur	einmal ausgef�hrt, siehe dazu
// "simplix(tModInfo *ModInfo)"
// Die DLL gibt	die	Namen der Fahrer immer so an TORCS zur�ck, wie sie in
// der XML-Datei des Teams angegeben sind!
// Wertet den Pitsharing-Status	aus	und	handelt	danach bei Endurance Rennen
//--------------------------------------------------------------------------*
// This	program	was	developed and tested on	windows	XP
// There are no	known Bugs,	but:
// Who uses	the	files accepts, that	no responsibility is adopted
// for bugs, dammages, aftereffects	or consequential losses.
//
// Das Programm	wurde unter	Windows	XP entwickelt und getestet.
// Fehler sind nicht bekannt, dennoch gilt:
// Wer die Dateien verwendet erkennt an, dass f�r Fehler, Sch�den,
// Folgefehler oder	Folgesch�den keine Haftung �bernommen wird.
//--------------------------------------------------------------------------*
// This	program	is free	software; you can redistribute it and/or modify
// it under	the	terms of the GNU General Public	License	as published by
// the Free	Software Foundation; either	version	2 of the License, or
// (at your	option)	any	later version.
//
// Im �brigen gilt f�r die Nutzung und/oder	Weitergabe die
// GNU GPL (General	Public License)
// Version 2 oder nach eigener Wahl	eine sp�tere Version.
//--------------------------------------------------------------------------*

#include <tgf.h>
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robottools.h>
#include <timeanalysis.h>
#include <robot.h>

#include "unitglobal.h"
#include "unitcommon.h"

#include "unitdriver.h"
#include <string>
#include <vector>

//==========================================================================*
// Prototypes of routines(functions/procedures), provided
// for communication with TORCS	using the traditional Interface
// Prototypen der Routinen(Funktionen/Prozeduren), die wir f�r die
// Kommunikation mit TORCS �ber	das	traditionale Interface bereitstellen
//--------------------------------------------------------------------------*
static void	InitTrack
  (int index,
  tTrack* track,
  void *carHandle,
  void **carParmHandle,
  tSituation *s);
static void	NewRace
  (int index,
  tCarElt* car,
  tSituation *s);
static void	Drive
  (int index,
  tCarElt* car,
  tSituation *s);
static int PitCmd
  (int index,
  tCarElt* car,
  tSituation *s);
static void	Shutdown
  (int index);
static int InitFuncPt
  (int index,
  void *pt);
static void	EndRace
  (int index,
  tCarElt *car,
  tSituation *s);
static void	Shutdown
  (int index);
//==========================================================================*

//==========================================================================*
// Speed Dreams-Interface
//--------------------------------------------------------------------------*
static const int BUFSIZE = 256;

struct Ident
{
    std::string name, desc;
};

// Pointer to buffer for driver's names	defined	in robot's xml-file
// Pointer to buffer for driver's descriptions defined in robot's xml-file
std::vector<Ident> idents;

// Number of drivers defined in	robot's	xml-file
static char	BufName[BUFSIZE];					 // Buffer for robot's name
static const char* RobName = BufName;			 //	Pointer	to robot's name
// Robot's relative	dir
static char	BufPathDirRel[BUFSIZE];				 // Robot's dir relative
static const char* RobPathDirRel = BufPathDirRel;//	to installation	dir
// Robot's relative	xml-filename
static char	BufPathXMLRel[BUFSIZE];				 // Robot's xml-filename
static const char* RobPathXMLRel = BufPathXMLRel;//	relative to	install. dir
// Robot's absolute	dir
static char	BufPathDir[BUFSIZE];				 //	Robot's	dir
// Robot's absolute	xml-filename
static char	BufPathXML[BUFSIZE];				 //	Robot's	xml-filename
static const char* RobPathXML =	BufPathXML;		 // Pointer to	xml-filename

// The "Simplix" logger	instance
GfLogger* PLogSimplix =	0;
//==========================================================================*

//==========================================================================*
//	Robot of	this modul
//	Roboter des Moduls
//--------------------------------------------------------------------------*

struct tInstanceInfo
{
    tInstanceInfo(const std::string &name, const std::string &car,
        const std::string &category, int index) :
        cRobot(TDriver(name, car, category, index)),
        cTicks(0),
        cMinTicks(FLT_MAX),
        cMaxTicks(0),
        cTickCount(0),
        cLongSteps(0),
        cCriticalSteps(0),
        cUnusedCount(0)
    {}

    // TClothoidPath contains a pointer to the TDriver,
    // so tInstanceInfo cannot be copied around.
    TDriver cRobot;
    double	cTicks;
    double	cMinTicks;
    double	cMaxTicks;
    int cTickCount;
    int cLongSteps;
    int cCriticalSteps;
    int cUnusedCount;
};

class Drivers
{
public:
    ~Drivers()
    {
        clear();
    }

    void push_back(tInstanceInfo *d)
    {
        v.push_back(d);
    }

    void clear()
    {
        for (auto d : v)
            delete d;

        v.clear();
    }

    tInstanceInfo *operator[](int index)
    {
        return v[index];
    }

    std::vector<tInstanceInfo *> v;
};

static class Drivers drivers;

//==========================================================================*

//==========================================================================*
// Get filehandle for robot's xml-file
//--------------------------------------------------------------------------*
void* GetFileHandle(const char*	RobotName)
{
    void* RobotSettings = NULL;

    if	(BufName !=	RobotName)					  //	Don't copy if same names
    {
        strncpy(BufName, RobotName, BUFSIZE -	1);		  // Save robot's name
        BufName[BUFSIZE -	1] = '\0';
    }
    snprintf(BufPathDirRel, BUFSIZE,			  // Robot's directory
        "drivers/%s",RobotName);				   //	relative to	installation
    snprintf(BufPathXMLRel, BUFSIZE,			  // Robot's xml-filename
        "drivers/%s/%s.xml",RobotName,RobotName);// relative to installation

    std::string dirstr = std::string(GfLocalDir()) + "drivers/" + RobotName;
    const char *dir = dirstr.c_str();

    if (GfDirCreate(dir) != GF_DIR_CREATED)
      return NULL;

    //	Test local installation	path
    snprintf(BufPathXML, BUFSIZE, "%s%s",
        GetLocalDir(), RobPathXMLRel);
    snprintf(BufPathDir, BUFSIZE, "%s%s",
        GetLocalDir(), RobPathDirRel);
    RobotSettings = GfParmReadFile
        (RobPathXML, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

    if	(!RobotSettings)
    {
      // If not found,	use	global installation	path
      snprintf(BufPathXML,	BUFSIZE, "%s%s",
          GetDataDir(), RobPathXMLRel);
      snprintf(BufPathDir,	BUFSIZE, "%s%s",
          GetDataDir(), RobPathDirRel);
      RobotSettings = GfParmReadFile
          (RobPathXML, GFPARM_RMODE_STD );
    }

    if (GfParmWriteFile(nullptr, RobotSettings, "simplix"))
    {
      GfParmReleaseHandle(RobotSettings);
      return NULL;
    }

    return	RobotSettings;
}
//==========================================================================*

//==========================================================================*

static int loadIdentities(void *handle)
{
    static const char section[] = ROB_SECT_ROBOTS "/" ROB_LIST_INDEX;

    idents.clear();

    for (int i = 0; i < GfParmGetEltNb(handle, section); i++)
    {
        std::string index = ROB_SECT_ROBOTS;

        index += "/";
        index += ROB_LIST_INDEX;
        index += "/";
        index += std::to_string(i);

        const char *cidx = index.c_str();
        const char *name = GfParmGetStr(handle, cidx,
            ROB_ATTR_NAME, nullptr);

        if (!name)
        {
            LogSimplix.error("#GfParmGetStr %s:%s/%s failed\n",
                RobPathXMLRel, cidx, ROB_ATTR_NAME);
            idents.clear();
            return -1;
        }

        const char *desc = GfParmGetStr(handle, cidx,
            ROB_ATTR_DESC, "no description");

        Ident id = {name, desc};

        idents.push_back(id);
        LogSimplix.debug("#Driver %d: %s (%s)\n", i, name, desc);
    }

    return 0;
}

//==========================================================================*
// Handle module entry for Speed Dreams	Interface V1.00	(new fixed name	scheme)
//--------------------------------------------------------------------------*
int	moduleWelcomeV1_00
  (const tModWelcomeIn*	welcomeIn, tModWelcomeOut* welcomeOut)
{
    void *RobotSettings = GetFileHandle(welcomeIn->name);

    idents.clear();

    PLogSimplix = GfLogger::instance("Simplix");
    LogSimplix.debug("\n#Interface	Version: %d.%d\n",
        welcomeIn->itfVerMajor,welcomeIn->itfVerMinor);

    if (!RobotSettings)
        goto end;

    LogSimplix.debug("#Robot name: %s\n", RobName);
    LogSimplix.debug("#Robot directory: %s\n", RobPathDirRel);
    LogSimplix.debug("#Robot XML-file: %s\n", RobPathXMLRel);

    if (loadIdentities(RobotSettings))
    {
        LogSimplix.error("#Failed to load identities\n");
        goto end;
    }

end:
    if (RobotSettings)
        GfParmReleaseHandle(RobotSettings);

    welcomeOut->maxNbItf = idents.size();
    return 0;
}
//==========================================================================*

//==========================================================================*
// Module entry	point (new fixed name scheme).
// Extended	for	use	with schismatic	robots and checked interface versions
//--------------------------------------------------------------------------*
extern "C" int moduleWelcome
  (const tModWelcomeIn*	welcomeIn, tModWelcomeOut* welcomeOut)
{
    if	(welcomeIn->itfVerMajor	>= 1)
    {
        if (welcomeIn->itfVerMinor > 0)
            // For future use add updated versions here
            return moduleWelcomeV1_00(welcomeIn,	welcomeOut);
        else
            // Initial version
            return moduleWelcomeV1_00(welcomeIn,	welcomeOut);
    }

    LogSimplix.debug("\n#Unhandled	Interface Version: %d.%d\n",
        welcomeIn->itfVerMajor,welcomeIn->itfVerMinor);
    welcomeOut->maxNbItf =	0;
    return	-1;
}
//==========================================================================*

static int getCar(void *handle, int index, std::string &out)
{
    const char *car;
    std::string section = ROB_SECT_ROBOTS "/" ROB_LIST_INDEX "/";

    section += std::to_string(index);

    if (!(car = GfParmGetStr(handle, section.c_str(), ROB_ATTR_CAR, nullptr)))
    {
        LogSimplix.error("Missing attribute for driver %u: " ROB_ATTR_CAR, index);
        return -1;
    }

    out = car;
    return 0;
}

static int getCategory(const std::string &car, std::string &out)
{
    int ret = -1;
    std::string path = "cars/models/";
    const char *category, *cpath;
    void *h;

    path += car;
    path += "/";
    path += car;
    path += PARAMEXT;
    cpath = path.c_str();

    if (!(h = GfParmReadFileBoth(cpath, GFPARM_RMODE_STD)))
    {
        LogSimplix.error("Failed to open %s\n", cpath);
        goto end;
    }
    else if (!(category = GfParmGetStr(h, "Car", ROB_ATTR_CATEGORY, nullptr)))
    {
        LogSimplix.error("%s: failed to get car category\n", cpath);
        goto end;
    }

    out = category;
    ret = 0;

end:
    if (h)
        GfParmReleaseHandle(h);

    return ret;
}

//==========================================================================*
// Module entry	point (new fixed name scheme).
// Tells TORCS,	who	we are,	how	we want	to be called and
// what	we are able	to do.
// Teilt TORCS mit,	wer	wir	sind, wie wir angesprochen werden wollen und
// was wir k�nnen.
//--------------------------------------------------------------------------*
extern "C" int moduleInitialize(tModInfo *ModInfo)
{
    int ret = -1;
    void *handle = GetFileHandle("simplix");

    LogSimplix.debug("\n#Initialize from %s ...\n",RobPathXML);
    drivers.clear();

    if (!handle)
    {
        LogSimplix.warning("Failed to get file handle\n");
        ret = 0;
        goto end;
    }

    for (size_t i = 0; i < idents.size(); i++)
    {
        std::string car, category = "";

        if (getCar(handle, i, car))
        {
            LogSimplix.error("Failed to get car name for driver %u\n",
                static_cast<unsigned>(i));
            goto end;
        }
        else if (getCategory(car, category))
            LogSimplix.warning("Failed to get category for driver %u\n",
                static_cast<unsigned>(i));

        const Ident &id = idents.at(i);
        const std::string &name = id.name;

        ModInfo[i].name = id.name.c_str();
        ModInfo[i].desc = id.desc.c_str();
        ModInfo[i].fctInit	= InitFuncPt;
        ModInfo[i].gfId = ROB_IDENT;
        ModInfo[i].index =	i;

        drivers.push_back(new tInstanceInfo(name, car, category, i));
    }

    LogSimplix.debug("# ... Initialized\n\n");
    ret = 0;

end:
    if (ret)
        drivers.clear();

    if (handle)
        GfParmReleaseHandle(handle);

    return ret;
}
//==========================================================================*

//==========================================================================*
// Module exit point (new fixed	name scheme).
//--------------------------------------------------------------------------*
extern "C" int moduleTerminate()
{
    LogSimplix.debug("#Terminated %s\n\n",RobName);

  return 0;
}
//==========================================================================*

//==========================================================================*
// Module entry	point (Torcs backward compatibility	scheme).
//--------------------------------------------------------------------------*
int	simplixEntryPoint(tModInfo *ModInfo, void *RobotSettings)
{
    LogSimplix.debug("\n#Torcs	backward compatibility scheme used\n");

    if (loadIdentities(RobotSettings))
    {
        LogSimplix.error("#Failed to load identities\n");
        return -1;
    }

    GfParmReleaseHandle(RobotSettings);

    return	moduleInitialize(ModInfo);
}
//==========================================================================*

//==========================================================================*
// Module exit point (Torcs	backward compatibility scheme).
//--------------------------------------------------------------------------*
extern "C" int simplixShut()
{
    return	moduleTerminate();
}
//==========================================================================*

//==========================================================================*
// TORCS: Initialization
// TOCRS: Initialisierung
//
// After clarification of the general calling (calling this	func.),
// we tell TORCS our functions to provide the requested	services:
//
// Nach	Kl�rung	der	generellen Ansprache (Aufruf dieser	Fkt), teilen wir
// TORCS nun noch mit, mit welchen Funktionen wir die angeforderten
// Leistungen erbringen	werden:
//
// Die geforderten Leistungen m�ssen erbracht werden ...
// RbNewTrack: ... wenn	Torcs eine neue	Rennstrecke	bereitstellt
// RbNewRace:  ... wenn	Torcs ein neues	Rennen startet
// RbDrive:	   ... wenn	das	Rennen gefahren	wird
// RbPitCmd:   ... wenn	wir	einen Boxenstop	machen
// RbEndRace:  ... wenn	das	Rennen ist beendet
// RbShutDown: ... wenn	der	ggf. angefallene Schrott beseitigt werden muss
//--------------------------------------------------------------------------*
static int InitFuncPt(int Index, void *Pt)
{
  tRobotItf	*Itf = (tRobotItf *)Pt;				 // Get typed pointer

  Itf->rbNewTrack =	InitTrack;					 // Store function pointers
  Itf->rbNewRace  =	NewRace;
  Itf->rbDrive	  = Drive;
  Itf->rbPitCmd	  =	PitCmd;
  Itf->rbEndRace  =	EndRace;
  Itf->rbShutdown =	Shutdown;
  Itf->index	  = Index;						  // Store	index

  return 0;
}
//==========================================================================*

//==========================================================================*
// TORCS: New track
// TOCRS: Neue Rennstrecke
//--------------------------------------------------------------------------*
static void	InitTrack(int Index,
  tTrack* Track,void *CarHandle,void **CarParmHandle, tSituation *S)
{
  drivers[Index]->cRobot.InitTrack
      (Track,CarHandle,CarParmHandle, S);
}
//==========================================================================*

//==========================================================================*
// TORCS: New Race starts
// TOCRS: Neues	Rennen beginnt
//--------------------------------------------------------------------------*
static void	NewRace(int	Index, tCarElt*	Car, tSituation	*S)
{
  RtInitTimer(); //	Check existance	of Performance Counter Hardware
  tInstanceInfo *d = drivers[Index];

  d->cRobot.NewRace(Car, S);
  d->cRobot.CurrSimTime	= -10.0;
}
//==========================================================================*

//==========================================================================*
// TORCS-Callback: Drive
// TOCRS-Callback: Rennen fahren
//
// Attention: This procedure is	called very	frequent and fast in succession!
// Therefore we	don't throw	debug messages here!
// To find basic bugs, it may be usefull to	do it anyhow!

// Achtung:	Diese Prozedur wird	sehr h�ufig	und	schnell	nacheinander
// aufgerufen. Deshalb geben wir hier in der Regel keine Debug-Texte aus!
// Zur Fehlersuche kann	das	aber mal sinnvoll sein.
//--------------------------------------------------------------------------*
static void	Drive(int Index, tCarElt* Car, tSituation *S)
{
  tInstanceInfo *d = drivers[Index];
  TDriver &robot = d->cRobot;

  //LogSimplix.debug("#>>> TDriver::Drive\n");
  if (robot.CurrSimTime	< S->currentTime)
//	if (robot.CurrSimTime + 0.03	< S->currentTime)
  {
    //LogSimplix.debug("#Drive\n");
    double	StartTimeStamp = RtTimeStamp();

    robot.CurrSimTime =	 // Update	current	time
        S->currentTime;
    robot.Update(Car,S);	 // Update info about	opp.
    if	(robot.IsStuck())	 // Check	if we are stuck
      robot.Unstuck();		 //   Unstuck
    else										  //	or
      robot.Drive();		 //	 Drive

    double	Duration = RtDuration(StartTimeStamp);

    if	(d->cTickCount >	0)		 //	Collect	used time
    {
      if (Duration	> 1.0)
        d->cLongSteps++;
      if (Duration	> 2.0)
        d->cCriticalSteps++;
      if (d->cMinTicks > Duration)
        d->cMinTicks =	Duration;
      if (d->cMaxTicks < Duration)
        d->cMaxTicks =	Duration;
    }
    d->cTickCount++;
    d->cTicks += Duration;
  }
  else
  {
    //LogSimplix.debug("#DriveLast\n");
    d->cUnusedCount++;
    robot.DriveLast();		 // Use last drive	commands
  }
  //LogSimplix.debug("#<<< TDriver::Drive\n");
}
//==========================================================================*

//==========================================================================*
// TORCS: Pitstop (Car is in pit!)
// TOCRS: Boxenstop	(Wagen steht in	der	Box!)
//--------------------------------------------------------------------------*
static int PitCmd(int Index, tCarElt* Car, tSituation *S)
{
  // Dummy:	use	parameters
  if ((Index < 0) || (Car == NULL) || (S ==	NULL))
    LogSimplix.debug("PitCmd\n");
  return drivers[Index]->cRobot.PitCmd();
}
//==========================================================================*

//==========================================================================*
// TORCS: Race ended
// TOCRS: Rennen ist beendet
//--------------------------------------------------------------------------*
static void	EndRace(int	Index, tCarElt *Car, tSituation	*S)
{
  // Dummy:	use	parameters
  if ((Index < 0) || (Car == NULL) || (S ==	NULL))
      Index = 0;

  LogSimplix.debug("EndRace\n");
  drivers[Index]->cRobot.EndRace();
}
//==========================================================================*

//==========================================================================*
// TORCS: Cleanup
// TOCRS: Aufr�umen
//--------------------------------------------------------------------------*
static void	Shutdown(int Index)
{
  tInstanceInfo *d = drivers[Index];

  LogSimplix.debug("\n\n#Clock\n");
  LogSimplix.debug("#Total Time	used: %g sec\n",d->cTicks/1000.0);
  LogSimplix.debug("#Min   Time	used: %g msec\n",d->cMinTicks);
  LogSimplix.debug("#Max   Time	used: %g msec\n",d->cMaxTicks);
  LogSimplix.debug("#Mean  Time	used: %g msec\n",d->cTicks/d->cTickCount);
  LogSimplix.debug("#Long Time Steps: %d\n",d->cLongSteps);
  LogSimplix.debug("#Critical Steps	: %d\n",d->cCriticalSteps);
  LogSimplix.debug("#Unused	Steps	:	%d\n",d->cUnusedCount);
  LogSimplix.debug("\n");
  LogSimplix.debug("\n");

  d->cRobot.Shutdown();
}
//==========================================================================*

//==========================================================================*
// Module entry	point (Torcs backward compatibility	scheme).
//--------------------------------------------------------------------------*
extern "C" int simplix(tModInfo	*ModInfo)
{
  void *RobotSettings =	GetFileHandle("simplix");
  if (!RobotSettings)
      return 0;

  return simplixEntryPoint(ModInfo,RobotSettings);
}
//==========================================================================*

//--------------------------------------------------------------------------*
// end of file unitmain.cpp
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
