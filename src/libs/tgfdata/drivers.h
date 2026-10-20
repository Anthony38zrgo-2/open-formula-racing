/***************************************************************************

    file                 : drivers.h
    created              : Sun Nov 21 19:00:00 CET 2010
    copyright            : (C) 2010 by Jean-Philippe MEURET
    web                  : speed-dreams.sourceforge.net

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
		Singleton holding information on the available drivers
    @defgroup	tgfdata	Data manager for the client gaming framework.
*/

#ifndef __TGFDRIVERS_H__
#define __TGFDRIVERS_H__

#include <string>
#include <vector>
#include <map>
#include <variant>

#include "tgf.h"
#include "tgfdata.h"

class GfCar;


class TGFDATA_API GfDriverSkin
{
public:

	GfDriverSkin(const std::string& strName = "");

public:

	int getTargets() const;
	const std::string& getName() const;
	const std::string& getCarPreviewFileName() const;

	void setTargets(int bfTargets); // Overwrite any previous target.
	void addTargets(int nTargets); // Bit-or.
	void setName(const std::string& strName);
	void setCarPreviewFileName(const std::string& strFileName);

protected:

	int          _bfTargets; // Skin targets bit-field (see car.h for possible values)
	std::string  _strName;  // Skin name (empty if standard skin)
	std::string  _strCarPreviewFileName;  // Car preview for this skin name (empty if none)
};

class TGFDATA_API GfDriver
{
public:

	GfDriver(const std::string& strModuleName, int nItfIndex,
			 const std::string& strName, void* hparmRobot);

	void load(void* hparmRobot);

	const std::string& getName() const;
	const std::string& getModuleName() const;
	int getInterfaceIndex() const;
	bool isHuman() const;
	bool isNetwork() const;
	const GfCar* getCar() const;
	const std::string& getType() const;
	const GfDriverSkin& getSkin() const;

	bool matchesTypeAndCategory(const std::string& strType = "",
								const std::string& strCarCatId = "") const;

	int getSupportedFeatures() const;

	double getSkillLevel() const;

	void setCar(const GfCar* pCar);
	void setSkin(const GfDriverSkin& skin);

	//! Compute a driver type from a driver module name.
	static std::string getType(const std::string& strModName);

	//! Get the possible skins using the same search path as the graphics engine.
	std::vector<GfDriverSkin> getPossibleSkins(const std::string& strAltCarId = "") const;

	//! Retrieve the skin with given name is the given list
	static std::vector<GfDriverSkin>::iterator findSkin(std::vector<GfDriverSkin>& vecSkins,
														const std::string& strName);

	//! Get the possible skins in a given folder.
	static void getPossibleSkinsInFolder(const std::string& strCarId,
								  const std::string& strFolderPath,
								  std::vector<GfDriverSkin>& vecPossSkins);

	static void processSkins(std::vector<GfDriverSkin> &skins,
		const std::string &car);

protected:

	std::string  _strName;      // User friendly name (ex: "Yuuki Kyousou").
	std::string  _strModName;   // Module shared library / folder name (ex: "simplix_sc")
	int          _nItfIndex;    // Index of associated interface in the module
    bool         _bIsHuman;	    // true for humans
	const GfCar* _pCar;         // Car
	GfDriverSkin _skin;         // Skin

	mutable std::string _strType;     // Type ~ module type (ex: "simplix", "usr")

	double _fSkillLevel; // From 0 (pro) to 10 (rookie).
	int _nFeatures; // Bit mask built with RM_FEATURE_*
};

class GfXMLDriver
{
public:
	GfXMLDriver(unsigned index) : index(index) {}
	int read(void *h);

	struct attr
	{
		attr() {}
		attr(const char *s): type(type::string), s(s) {}
		attr(tdble d): type(type::num), num(d) {}

		enum class type {num, string} type;
		std::string s;
		tdble num;
	};

	unsigned index;
	std::string tmpdir;
	std::map<std::string, attr> map;
};

class TGFDATA_API GfDrivers
{
public:

	// Accessor to the unique instance of the singleton.
	static GfDrivers* self();
	static void shutdown();

	// Reload drivers data from files.
	int reload();

	unsigned getCount() const;
 	const std::vector<std::string>& getTypes() const;

 	GfDriver* getDriver(const std::string& strModName, int nItfIndex) const;
	GfDriver* getDriverWithName(const std::string& strName, const char *mod = nullptr) const;
 	int getDriverIdx(void *h, const char *path, const char *mod) const;
	int getDriverIdx(const std::string &mod, const std::string &name) const;

 	std::vector<GfDriver*> getDriversWithTypeAndCategory(const std::string& strType = "",
														 const std::string& strCarCatId = "") const;

 	void print() const;
	int gen(const std::string &driver, const std::string &category,
		const std::string &car = "") const;
	int gen(const std::vector<std::string> &robots,
		const std::string &category, unsigned n) const;
	int robots(std::vector<std::string> &out) const;
	int ensure_min();
	int ensure_min(const std::string &path, void *args) const;
	int del(const std::string &mod, const std::string &driver) const;

protected:

	// Protected constructor and destructor : clients can not use them.
	GfDrivers();
	~GfDrivers();

	typedef int (GfDrivers::*action)(const std::string &dir, void *args) const;
	int extract(const std::string &dir, void *args) const;
	int extract_driver(const std::string &dir, void *args) const;
	int rename(const std::string &dir, std::string &tmpdir) const;
	int drvdir(std::string &dir) const;
	int iter(const std::string &dir, action a, void *args,
		enum FList::type type = FList::type::dir) const;

	// Clear all data.
	void clear();

	struct identity
	{
		std::string name, short_name, code_name, nationality, team;
	};

	typedef std::vector<GfXMLDriver> GfXMLDrivers;
	typedef std::map<std::string, GfXMLDrivers> GfXMLDriversMap;
	int regen() const;
	int basename(const std::string &path, std::string &out) const;
	int parent(const std::string &path, std::string &out) const;
	int gendir(const std::string &dir) const;
	int read(void *h, GfXMLDriver &drv) const;
	int dump(const std::string &dir, void *args) const;
	int dump(const GfXMLDriver &d, void *h, const std::string &path) const;
	int sort(const std::string &dir, void *args) const;
	bool isindex(const std::string &dir) const;
	bool human(const std::string &name) const;
	int genparams(const std::string &driver, const std::string &category,
		const std::string &car, const std::string &dir) const;
	int genident(struct identity &ident) const;
	int genskill(const std::string &driver, const std::string &dir) const;
	bool supports_aggression(const std::string &driver) const;
	int pickdriver(std::string &seldriver) const;
	int pickcar(const std::string &category, std::string &car,
		std::string &livery) const;
	static std::vector<GfDriverSkin> getskins(const std::string &car);

protected:

	// The singleton itself.
	static GfDrivers* _pSelf;

	// Its private data.
	class Private;
	Private* _pPrivate;

	static const char *const teams[];
	static const size_t n_teams;

	static const struct names
	{
		const char *nation, *const *names, *const *surnames;
		size_t n_names, n_surnames;
	} names[];

	static const size_t n_names;
	enum nationality {JA, EN, DE, CRO, IT, FR};
};

#endif /* __TGFDRIVERS_H__ */
