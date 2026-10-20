/***************************************************************************

	file                 : urbanski.cpp
	created              : Mon Nov 3 01:13:44 CET 2014
	copyright            : (C) 2014 Patryk Urbanski

 ***************************************************************************/

#include <portability.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>

#include <tgf.h>
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robottools.h>
#include <robot.h>

#include "driver.h"

struct ident
{
	std::string name, desc;
};

static std::vector<Driver> driver;
static std::vector<ident> idents;

static void initTrack(int index, tTrack *track, void *carHandle, void **carParmHandle, tSituation *s);
static void newrace(int index, tCarElt *car, tSituation *s);
static void drive(int index, tCarElt *car, tSituation *s);
static void endrace(int index, tCarElt *car, tSituation *s);
static void shutdown(int index);
static int  InitFuncPt(int index, void *pt);

extern "C" int moduleInitialize(tModInfo *modInfo) {
	driver.clear();
	memset(modInfo, 0, idents.size() * sizeof(tModInfo));

	for (size_t i = 0; i < idents.size(); i++, modInfo++) {
		const ident &id = idents.at(i);

		modInfo->name    = id.name.c_str();		/* name of the module (short) */
		modInfo->desc    = id.desc.c_str();	/* description of the module (can be long) */
		modInfo->fctInit = InitFuncPt;		/* init function */
		modInfo->gfId    = ROB_IDENT;		/* supported framework version */
		modInfo->index   = i;
		driver.push_back(Driver());
	}

	GfOut("Initialized urbanski\n");
	return 0;
}

extern "C" int moduleWelcome(const tModWelcomeIn* welcomeIn, tModWelcomeOut* welcomeOut) {
	int ret = -1;
	static const char path[] = "drivers/urbanski/urbanski.xml";
	void *h = GfParmReadFileLocal(path, GFPARM_RMODE_STD);
	int n;

	if (!h)
	{
		ret = 0;
		goto end;
	}

	n = GfParmGetEltNb(h, ROB_SECT_ROBOTS "/" ROB_LIST_INDEX);
	idents.clear();

	for (int i = 0; i < n; i++)
	{
		std::string section = ROB_SECT_ROBOTS "/" ROB_LIST_INDEX "/";
		const char *s;

		section += std::to_string(i);
		s = section.c_str();

		const char *name = GfParmGetStr(h, s, ROB_ATTR_NAME, nullptr);

		if (!name)
		{
			GfLogError("%s: GfParmGetStr %s failed\n", path, s);
			goto end;
		}

		const char *desc = GfParmGetStr(h, s, ROB_ATTR_DESC, "");
		ident id = {name, desc};

		idents.push_back(id);
	}

	ret = 0;

end:
	if (h)
		GfParmReleaseHandle(h);

	welcomeOut->maxNbItf = idents.size();
	return ret;
}

extern "C" int moduleTerminate() {
	GfOut("Terminated urbanski\n");
	return 0;
}

/*
 * Module entry point
 */
extern "C" int urbanski(tModInfo *modInfo) {
	return moduleInitialize(modInfo);
}

/*
 * Module exit point
 */
extern "C" int urbanskiShut() {
	return moduleTerminate();
}

/* Module interface initialization. */
static int InitFuncPt(int index, void *pt) {
	tRobotItf *itf  = (tRobotItf *)pt;

	itf->rbNewTrack = initTrack; /* Give the robot the track view called */
				 /* for every track change or new race */
	itf->rbNewRace  = newrace; 	 /* Start a new race */
	itf->rbDrive    = drive;	 /* Drive during race */
	itf->rbPitCmd   = NULL;
	itf->rbEndRace  = endrace;	 /* End of the current race */
	itf->rbShutdown = shutdown;	 /* Called before the module is unloaded */
	itf->index      = index; 	 /* Index used if multiple interfaces */
	return 0;
}

/* Called for every track change or new race. */
static void initTrack(int index, tTrack *track, void *carHandle, void **carParmHandle, tSituation *s) {
	driver[index].setTrack(track, carParmHandle);
}

/* Start a new race. */
static void newrace(int index, tCarElt *car, tSituation *s) {
	driver[index].setCar(car);
}

/* Drive during race. */
static void drive(int index, tCarElt *car, tSituation *s) {
	driver[index].drive(s);
}

/* End of the current race */
static void endrace(int index, tCarElt *car, tSituation *s) {
	driver[index].endRace();
}

/* Called before the module is unloaded */
static void shutdown(int index) {
}
