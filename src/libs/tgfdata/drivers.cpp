/***************************************************************************

    file                 : drivers.cpp
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

#include <portability.h>

#include <cctype>
#include <map>
#include <sstream>
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <vector>

#include <robot.h>

#include "cars.h"
#include "drivers.h"

static const struct rattr
{
    const char *key;
    enum GfXMLDriver::attr::type type;
} drvattrs[] =
{
    {ROB_ATTR_NAME, GfXMLDriver::attr::type::string},
    {ROB_ATTR_SNAME, GfXMLDriver::attr::type::string},
    {ROB_ATTR_CODE, GfXMLDriver::attr::type::string},
    {ROB_ATTR_NATION, GfXMLDriver::attr::type::string},
    {ROB_ATTR_TEAM, GfXMLDriver::attr::type::string},
    {ROB_ATTR_AUTHOR, GfXMLDriver::attr::type::string},
    {ROB_ATTR_CAR, GfXMLDriver::attr::type::string},
    {ROB_ATTR_LIVERY, GfXMLDriver::attr::type::string},
    {ROB_ATTR_RACENUM, GfXMLDriver::attr::type::num},
    {ROB_ATTR_RED, GfXMLDriver::attr::type::num},
    {ROB_ATTR_GREEN, GfXMLDriver::attr::type::num},
    {ROB_ATTR_BLUE, GfXMLDriver::attr::type::num}
};

// Private data for GfDrivers
class GfDrivers::Private
{
public:

    // One GfDriver structure for each driver (order = sorted directory one).
    std::vector<GfDriver*> vecDrivers;

    // Map for quick access to GfDriver by { module name, interface index }
    typedef std::map<std::pair<std::string, int>, GfDriver*> TMapDriversByKey;
    TMapDriversByKey mapDriversByKey;

    // Vector of driver types.
    std::vector<std::string> vecTypes;

    // Vector of driver car categories.
    std::vector<std::string> vecCarCategoryIds;
};


GfDrivers* GfDrivers::_pSelf = 0;

GfDrivers *GfDrivers::self()
{
    if (!_pSelf)
        _pSelf = new GfDrivers;

    return _pSelf;
}

void GfDrivers::shutdown()
{
    delete _pSelf;
    _pSelf = 0;
}

GfDrivers::~GfDrivers()
{
    clear();

    delete _pPrivate;
    _pPrivate = 0;
}

void GfDrivers::clear()
{
    _pPrivate->mapDriversByKey.clear();
    _pPrivate->vecTypes.clear();
    _pPrivate->vecCarCategoryIds.clear();

    std::vector<GfDriver*>::const_iterator itDriver;
    for (itDriver = _pPrivate->vecDrivers.begin();
         itDriver != _pPrivate->vecDrivers.end(); ++itDriver)
        delete *itDriver;
    _pPrivate->vecDrivers.clear();
}

int GfDrivers::drvdir(std::string &dir) const
{
    const char *data = GfLocalDir();

    if (!data)
    {
        GfLogError("GfDataDir failed\n");
        return -1;
    }

    dir = data;
    dir += "drivers/";
    return 0;
}

int GfDrivers::iter(const std::string &dir, GfDrivers::action a, void *args,
    enum FList::type type) const
{
    tFList *l = GfDirGetList(dir.c_str());
    const tFList *f = l;

    if (!l)
        return 0;

    do
    {
        const char *name = f->name;
        std::string fname = dir + f->name;

        if (!strcmp(name, ".")
            || !strcmp(name, "..")
            || (type != FList::unknown
                && f->type != type))
            continue;
        else if (f->type == FList::dir)
            fname += '/';

        if ((this->*a)(fname, args))
        {
            GfLogError("%s: failed\n", fname.c_str());
            continue;
        }
    } while ((f = f->next) != l);

    GfDirFreeList(l, nullptr, true, true);
    return 0;
}

int GfDrivers::basename(const std::string &path, std::string &out) const
{
    const char *p = path.c_str();
    std::string::size_type end = path.rfind('/');

    if (end == std::string::npos)
    {
        GfLogError("%s: failed to extract ending '/'\n", p);
        return -1;
    }

    std::string::size_type start = path.rfind('/', end - 1);

    if (start == std::string::npos)
    {
        GfLogError("%s: failed to extract start '/'\n", p);
        return -1;
    }

    start++;
    out = path.substr(start, end - start);
    return 0;
}

int GfDrivers::parent(const std::string &path, std::string &out) const
{
    const char *p = path.c_str();
    std::string::size_type end = path.rfind('/');

    if (end == std::string::npos)
    {
        GfLogError("%s: failed to extract ending '/'\n", p);
        return -1;
    }

    std::string::size_type start = path.rfind('/', end - 1);

    if (start == std::string::npos)
    {
        GfLogError("%s: failed to extract start '/'\n", p);
        return -1;
    }

    out = path.substr(0, start + 1);
    return 0;
}

bool GfDrivers::isindex(const std::string &name) const
{
    char *end;

    errno = 0;
    ::strtoul(name.c_str(), &end, 10);
    return !errno && !*end;
}

int GfDrivers::extract_driver(const std::string &dir, void *args) const
{
    std::string name, parent;

    if (basename(dir, name))
    {
        GfLogError("%s: failed to extract basename\n", dir.c_str());
        return -1;
    }
    else if (!isindex(name))
        return 0;

    int ret = -1;
    std::string path = dir + "driver.xml", tmpdir;
    const char *p = path.c_str();
    void *h = GfParmReadFile(p, GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
    GfXMLDrivers *drivers = static_cast<GfXMLDrivers *>(args);
    GfXMLDriver d(drivers->size());

    if (!h)
    {
        GfLogError("GfDrivers::extract_driver: GfParmReadFile %s failed\n", p);
        goto end;
    }
    else if (d.read(h))
    {
        GfLogError("Failed to read %s\n", p);
        goto end;
    }
    else if (GfDrivers::rename(dir, d.tmpdir))
        goto end;

    drivers->push_back(d);
    ret = 0;

end:
    if (h)
        GfParmReleaseHandle(h);

    return ret;
}

int GfDrivers::extract(const std::string &dir, void *args) const
{
    const char *d = dir.c_str();
    std::string name;
    GfXMLDrivers drivers;

    if (basename(dir, name))
    {
        GfLogError("%s: Could not extract basename\n", d);
        return -1;
    }
    else if (human(name))
        /* Human drivers do not define any subdirectories. */
        return 0;
    else if (iter(dir, &GfDrivers::extract_driver, &drivers))
    {
        GfLogError("%s: failed to extract driver parameters\n", d);
        return -1;
    }

    GfXMLDriversMap *map = static_cast<GfXMLDriversMap *>(args);

    (*map)[dir] = drivers;
    return 0;
}

int GfXMLDriver::read(void *h)
{
    for (size_t i = 0; i < sizeof drvattrs / sizeof *drvattrs; i++)
    {
        static const char path[] = "driver";
        const struct rattr &a = drvattrs[i];

        switch (a.type)
        {
            case attr::type::string:
            {
                const char *key = a.key;
                const char *val = GfParmGetStr(h, path, key, nullptr);

                if (!val)
                {
                    // Maintain backwards compatibility with == 2.4.0 robots.
                    if (!strcmp(key, ROB_ATTR_LIVERY))
                        val = "standard";
                    else
                    {
                        GfLogError("Failed to extract parameter: %s\n", key);
                        return -1;
                    }
                }

                map[key] = attr(val);
                break;
            }

            case attr::type::num:
            {
                const char *key = a.key;
                tdble val = GfParmGetNum(h, path, key, nullptr, 0);

                map[key] = attr(val);
                break;
            }
        }
    }

    return 0;
}

static int randname(std::string &name)
{
    static const size_t len = 32;

    for (size_t i = 0; i < len; i++)
    {
        unsigned char b;

        if (portability::rand(&b, sizeof b))
        {
            GfLogError("%s: portability::rand failed\n", __func__);
            return -1;
        }

        char hex[sizeof "00"];
        int n = snprintf(hex, sizeof hex, "%02hhx", b);

        if (n < 0 || n >= static_cast<int>(sizeof hex))
        {
            GfLogError("snprintf(3) failed with %d\n", n);
            return -1;
        }

        name += hex;
    }

    return 0;
}

int GfDrivers::rename(const std::string &dir, std::string &tmpdir) const
{
    const char *old = dir.c_str();

    if (GfDrivers::parent(dir, tmpdir))
    {
        GfLogError("%s: Failed to determine parent directory\n", old);
        return -1;
    }
    else if (randname(tmpdir))
    {
        GfLogError("Failed to generate random directory name\n");
        return -1;
    }

    tmpdir += '/';

    const char *newd = tmpdir.c_str();

    if (::rename(old, newd))
    {
        GfLogError("Failed to rename %s to %s: %s\n",
            old, newd, strerror(errno));
        return -1;
    }

    return 0;
}

int GfDrivers::dump(const GfXMLDriver &d, void *h, const std::string &path) const
{
    for (size_t i = 0; i < sizeof drvattrs / sizeof *drvattrs; i++)
    {
        const char *p = path.c_str();
        const struct rattr &a = drvattrs[i];

        switch (a.type)
        {
            case GfXMLDriver::attr::type::string:
            {
                const std::string &val = d.map.at(a.key).s;

                if (GfParmSetStr(h, p, a.key, val.c_str()))
                {
                    GfLogError("Failed to set parameter: %s\n", a.key);
                    return -1;
                }
            }
                break;

            case GfXMLDriver::attr::type::num:
            {
                tdble val = d.map.at(a.key).num;

                if (GfParmSetNum(h, p, a.key, nullptr, val))
                {
                    GfLogError("Failed to set parameter: %s\n", a.key);
                    return -1;
                }
            }
        }
    }

    return 0;
}

bool GfDrivers::human(const std::string &name) const
{
    return name == "human" || name == "networkhuman";
}

int GfDrivers::dump(const std::string &dir, void *args) const
{
    int ret = -1;
    const GfXMLDriversMap *map = static_cast<const GfXMLDriversMap *>(args);
    std::string name, params;
    void *h = nullptr;
    const char *p;

    if (basename(dir, name))
    {
        GfLogError("%s: failed to get basename\n", dir.c_str());
        goto end;
    }
    else if (human(name))
        /* Human drivers do not define any entries. */
        return 0;

    params = dir + name + PARAMEXT;
    p = params.c_str();

    if (!(h = GfParmReadFile(p, GFPARM_RMODE_NOREREAD | GFPARM_RMODE_CREAT)))
    {
        GfLogError("GfDrivers::dump: GfParmReadFile %s failed\n", p);
        goto end;
    }

    for (const auto &d : map->at(dir))
    {
        std::string path = ROB_SECT_ROBOTS "/" ROB_LIST_INDEX "/";

        path += std::to_string(d.index);

        if (dump(d, h, path))
        {
            GfLogError("Failed to dump driver\n");
            goto end;
        }
    }

    if (GfParmWriteFile(nullptr, h, name.c_str()))
    {
        GfLogError("GfParmWriteFile %s failed\n", p);
        goto end;
    }

    ret = 0;

end:
    if (h)
        GfParmReleaseHandle(h);

    return ret;
}

int GfDrivers::sort(const std::string &dir, void *args) const
{
    std::string name;

    if (basename(dir, name))
    {
        GfLogError("%s: failed to extract basename\n", dir.c_str());
        return -1;
    }
    else if (human(name))
        /* Human drivers do not define any entries. */
        return 0;

    const GfXMLDriversMap *map = static_cast<const GfXMLDriversMap *>(args);

    for (const auto &d: map->at(dir))
    {
        std::string newdir = dir + std::to_string(d.index) + '/';
        const char *old = d.tmpdir.c_str(), *newd = newdir.c_str();

        if (::rename(old, newd))
        {
            GfLogError("Failed to rename %s to %s: %s\n", old, newd,
                strerror(errno));
            return -1;
        }
    }

    return 0;
}

int GfDrivers::regen() const
{
    std::string dir;
    GfXMLDriversMap map;

    if (drvdir(dir))
    {
        GfLogError("Failed to get drivers directory\n");
        return -1;
    }
    else if (iter(dir, &GfDrivers::extract, &map))
    {
        GfLogError("Failed to extract driver data\n");
        return -1;
    }
    else if (iter(dir, &GfDrivers::dump, &map))
    {
        GfLogError("Failed to dump driver data\n");
        return -1;
    }
    else if (iter(dir, &GfDrivers::sort, &map))
    {
        GfLogError("Failed to sort driver data\n");
        return -1;
    }

    return 0;
}

int GfDrivers::reload()
{
    if (regen())
    {
        GfLogError("Failed to regenerate drivers list\n");
        return -1;
    }

    // Clear all.
    clear();

    // (Re)Load robot modules from the "drivers" installed folder.
    std::string strDriversDirName(GfLibDir());
    strDriversDirName += "drivers";

    tModList* lstDriverModules = 0;
    const int nDriverModules = GfModInfoDir(CAR_IDENT, strDriversDirName.c_str(), 1, &lstDriverModules);
    if (nDriverModules <= 0 || !lstDriverModules)
    {
        GfLogFatal("Could not load any driver module from %s", strDriversDirName.c_str());
        return -1;
    }

    // For each module found, load drivers information.
    const tModList *pCurModule = lstDriverModules;
    do
    {
        pCurModule = pCurModule->next;

        // Determine the module name.
        std::string strModName(pCurModule->sopath);
        strModName.erase(strModName.size() - DLLEXTLEN); // Truncate file ext.
        const size_t nLastSlashInd = strModName.rfind('/');
        if (nLastSlashInd != std::string::npos)
            strModName = strModName.substr(nLastSlashInd+1); // Remove heading folder path.

        // Load the module XML descriptor file (try  user settings first, and then installed one)
        std::string strRobotFileName = "drivers/" + strModName + '/' + strModName + PARAMEXT;
        void *hparmRobot = GfParmReadFileLocal(strRobotFileName, GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
        if (!hparmRobot)
            hparmRobot = GfParmReadFile(strRobotFileName, GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);

        if (!hparmRobot)
        {
            GfLogError("No usable '%s' driver (%s.xml not found or not readable)\n",
                       strModName.c_str(), strModName.c_str());
            continue;
        }

        _pPrivate->vecTypes.push_back(strModName);

        // For each driver (= interface) "in" the module
        for (int nItfInd = 0; nItfInd < pCurModule->modInfoSize; nItfInd++)
        {
            // Ignore undefined drivers or showing an empty name
            if (!pCurModule->modInfo[nItfInd].name || pCurModule->modInfo[nItfInd].name[0] == '\0')
            {
                GfLogInfo("Ignoring '%s' driver #%d (not defined or empty name)\n",
                             strModName.c_str(), nItfInd);
                continue;
            }

            // Create the driver and load info from the XML file.
            GfDriver* pDriver = new GfDriver(strModName, pCurModule->modInfo[nItfInd].index,
                                             pCurModule->modInfo[nItfInd].name, hparmRobot);

            // For human drivers, if the car was not found, select the 1st possible one.
            if (!pDriver->getCar() && pDriver->isHuman())
            {
                GfCar* pSubstCar = GfCars::self()->getCarsInCategory()[0]; // Should never fail.
                if (pSubstCar)
                {
                    std::ostringstream ossDrvSecPath;
                    ossDrvSecPath << ROB_SECT_ROBOTS << '/' << ROB_LIST_INDEX << '/'
                                  << pCurModule->modInfo[nItfInd].index;
                    const char* pszCarId =
                        GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_CAR, "");
                    GfLogWarning("Changing '%s' driver '%s' (#%d) 's car to %s (default one '%s' not available)\n",
                                 strModName.c_str(), pCurModule->modInfo[nItfInd].name,
                                 pCurModule->modInfo[nItfInd].index, pSubstCar->getId().c_str(),
                                 pszCarId);
                    pDriver->setCar(pSubstCar);
                }
            }

            // Keep the driver only if he drives an existing car.
            if (pDriver->getCar())
            {
                // Update the GfDrivers singleton.
                _pPrivate->vecDrivers.push_back(pDriver);
                const std::pair<std::string, int> driverKey(pDriver->getModuleName(),
                                                            pDriver->getInterfaceIndex());
                _pPrivate->mapDriversByKey[driverKey] = pDriver;

                if (std::find(_pPrivate->vecCarCategoryIds.begin(), _pPrivate->vecCarCategoryIds.end(),
                              pDriver->getCar()->getCategoryId()) == _pPrivate->vecCarCategoryIds.end())
                    _pPrivate->vecCarCategoryIds.push_back(pDriver->getCar()->getCategoryId());
            }
            else
            {
                delete pDriver;

                std::ostringstream ossDrvSecPath;
                ossDrvSecPath << ROB_SECT_ROBOTS << '/' << ROB_LIST_INDEX << '/'
                              << pCurModule->modInfo[nItfInd].index;
                const char* pszCarId =
                    GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_CAR, "");
                GfLogInfo("Ignoring '%s' driver '%s' (#%d) (not defined or default car '%s' not available)\n",
                             strModName.c_str(), pCurModule->modInfo[nItfInd].name,
                             pCurModule->modInfo[nItfInd].index, pszCarId);
            }
        }

        // Close driver module descriptor file if open
        GfParmReleaseHandle(hparmRobot);

    } while (pCurModule != lstDriverModules);

    // Free the module list.
    GfModFreeInfoList(&lstDriverModules);

    // Sort the car category ids and driver types vectors.
    std::sort(_pPrivate->vecCarCategoryIds.begin(), _pPrivate->vecCarCategoryIds.end());
    std::sort(_pPrivate->vecTypes.begin(), _pPrivate->vecTypes.end());

    // Trace what we got.
    print();
    return 0;
}

int GfDrivers::robots(std::vector<std::string> &out) const
{
    int ret = -1;
    std::string dir;
    tModList *mods = nullptr;
    const tModList *mod;
    const char *lib = GfLibDir(), *dirp;

    if (!lib)
    {
        GfLogError("GfLibDir failed\n");
        goto end;
    }

    dir = lib;
    dir += "drivers/";
    dirp = dir.c_str();

    if (GfModInfoDir(CAR_IDENT, dirp, 1, &mods) < 0)
    {
        GfLogError("GfModInfoDir %s failed\n", dirp);
        goto end;
    }

    mod = mods;

    do
    {
        std::string name = mod->sopath;
        std::string::size_type ext = name.rfind('.'), sep = name.rfind('/');

        if (ext == std::string::npos || sep == std::string::npos)
        {
            GfLogError("Invalid sopath: %s\n", mod->sopath);
            goto end;
        }

        sep++;
        name = name.substr(sep, ext - sep);
        out.push_back(name);
    } while ((mod = mod->next) != mods);

    ret = 0;

end:
    GfModFreeInfoList(&mods);
    return ret;
}

int GfDrivers::gen(const std::vector<std::string> &robots,
    const std::string &category, unsigned n) const
{
    for (unsigned i = 0; i < n; i++)
    {
        unsigned index;

again:
        if (portability::rand(&index, sizeof index))
        {
            GfLogError("Failed to calculate random index\n");
            return -1;
        }

        index %= robots.size();

        const std::string &driver = robots.at(index);
        const char *d = driver.c_str();

        if (human(driver))
            goto again;
        else if (gen(driver, category))
        {
            GfLogError("Failed to generate driver: %s\n", d);
            return -1;
        }
    }

    return 0;
}

struct ensure_min
{
    const std::vector<std::string> &robots;
    bool reload;
};

int GfDrivers::ensure_min(const std::string &path, void *args) const
{
    int ret = -1;
    static const unsigned min = 5;
    const char *p = path.c_str(), *category;
    std::vector<GfDriver *> drivers;
    struct ensure_min *emin = static_cast<struct ensure_min *>(args);
    std::string::size_type ext = path.rfind('.');
    void *h = nullptr;

    if (ext == std::string::npos
        || path.substr(ext) != PARAMEXT)
    {
        ret = 0;
        goto end;
    }
    else if (!(h = GfParmReadFile(p, GFPARM_RMODE_STD)))
    {
        GfLogError("GfDrivers::ensure_min: GfParmReadFile %s failed\n", p);
        goto end;
    }
    else if (!(category = GfParmGetStr(h, SECT_CAR, RM_ATTR_CATEGORY, nullptr)))
    {
        GfLogError("%s: failed to get category name\n", p);
        goto end;
    }
    else if (GfCars::self()->getCarsInCategory(category).empty())
    {
        GfLogInfo("Skipping category without any cars: %s\n", category);
        ret = 0;
        goto end;
    }

    drivers = getDriversWithTypeAndCategory("", category);

    if (drivers.size() < min)
    {
        unsigned needed = min - drivers.size();

        GfLogInfo("Will generate %u drivers for category %s\n",
            needed, category);

        if (gen(emin->robots, category, needed))
        {
            GfLogError("Failed to generate %u driver(s) for category=%s\n",
                needed, category);
            goto end;
        }

        emin->reload = true;
    }

    ret = 0;

end:
    if (h)
        GfParmReleaseHandle(h);

    return ret;
}

int GfDrivers::ensure_min()
{
    std::string dir;
    const char *data = GfDataDir(), *dirp;
    std::vector<std::string> r;
    struct ensure_min args = {r};

    if (robots(r))
    {
        GfLogError("Failed to extract robots list\n");
        return -1;
    }
    else if (!data)
    {
        GfLogError("GfDataDir failed\n");
        return -1;
    }

    dir = data;
    dir += "cars/categories/";
    dirp = dir.c_str();

    if (iter(dirp, &GfDrivers::ensure_min, &args, FList::file))
    {
        GfLogError("Failed to ensure category minimum on %s", dirp);
        return -1;
    }
    else if (args.reload && GfDrivers::reload())
    {
        GfLogError("Failed to reload drivers\n");
        return -1;
    }

    return 0;
}

GfDrivers::GfDrivers()
{
    _pPrivate = new GfDrivers::Private;

    reload();
    ensure_min();
}

unsigned GfDrivers::getCount() const
{
    return _pPrivate->vecDrivers.size();
}

const std::vector<std::string>& GfDrivers::getTypes() const
{
    return _pPrivate->vecTypes;
}

GfDriver* GfDrivers::getDriver(const std::string& strModName, int nItfIndex) const
{
    const std::pair<std::string, int> driverKey(strModName, nItfIndex);
    Private::TMapDriversByKey::iterator itDriver =
        _pPrivate->mapDriversByKey.find(driverKey);
    if (itDriver != _pPrivate->mapDriversByKey.end())
        return itDriver->second;

    return 0;
}

GfDriver *GfDrivers::getDriverWithName(const std::string& strName, const char *mod) const
{
    for (const auto d : _pPrivate->vecDrivers)
        if (d->getName() == strName)
            if (!mod || !*mod || d->getModuleName() == mod)
                return d;

    return nullptr;
}

std::vector<GfDriver*> GfDrivers::getDriversWithTypeAndCategory(const std::string& strType,
                                                                const std::string& strCarCatId) const
{
    std::vector<GfDriver*> vecSelDrivers;
    std::vector<GfDriver*>::iterator itDriver;
    for (itDriver = _pPrivate->vecDrivers.begin();
         itDriver != _pPrivate->vecDrivers.end(); ++itDriver)
        if ((*itDriver)->matchesTypeAndCategory(strType, strCarCatId))
            vecSelDrivers.push_back(*itDriver);

    return vecSelDrivers;
}

void GfDrivers::print() const
{
    GfLogTrace("Driver base : %zu types, %zu car categories, %zu drivers\n",
               _pPrivate->vecTypes.size(), _pPrivate->vecCarCategoryIds.size(),
               _pPrivate->vecDrivers.size());

    std::vector<std::string>::const_iterator itType;
    for (itType = _pPrivate->vecTypes.begin(); itType != _pPrivate->vecTypes.end(); ++itType)
    {
        GfLogTrace("  '%s' type :\n", itType->c_str());
        std::vector<std::string>::const_iterator itCarCatId;
        for (itCarCatId = _pPrivate->vecCarCategoryIds.begin();
             itCarCatId != _pPrivate->vecCarCategoryIds.end(); ++itCarCatId)
        {
            const std::vector<GfDriver*> vecDrivers =
                getDriversWithTypeAndCategory(*itType, *itCarCatId);
            if (vecDrivers.empty())
                continue;
            GfLogTrace("      '%s' car category :\n", itCarCatId->c_str());
            std::vector<GfDriver*>::const_iterator itDriver;
            for (itDriver = vecDrivers.begin(); itDriver != vecDrivers.end(); ++itDriver)
                GfLogTrace("          %-24s : %s, %02X-featured\n",
                           (*itDriver)->getName().c_str(),
                           (*itDriver)->getCar()->getName().c_str(),
                           (*itDriver)->getSupportedFeatures());
        }
    }
}

// GfDriverSkin class ---------------------------------------------------------------

GfDriverSkin::GfDriverSkin(const std::string& strName)
: _bfTargets(0), _strName(strName)
{
}

int GfDriverSkin::getTargets() const
{
    return _bfTargets;
}

const std::string& GfDriverSkin::getName() const
{
    return _strName;
}

const std::string& GfDriverSkin::getCarPreviewFileName() const
{
    return _strCarPreviewFileName;
}

void GfDriverSkin::setTargets(int bfTargets)
{
    _bfTargets = bfTargets;
}

void GfDriverSkin::addTargets(int bfTargets)
{
    _bfTargets |= bfTargets;
}

void GfDriverSkin::setName(const std::string& strName)
{
    _strName = strName;
}

void GfDriverSkin::setCarPreviewFileName(const std::string& strFileName)
{
    _strCarPreviewFileName = strFileName;
}

// GfDriver class -------------------------------------------------------------------

// Skill level related constants.
static const char *ASkillLevelStrings[] = ROB_VALS_LEVEL;
static const int NbSkillLevels = sizeof(ASkillLevelStrings) / sizeof(ASkillLevelStrings[0]);
static const double ASkillLevelValues[NbSkillLevels] = { 30.0, 20.0, 10.0, 7.0, 3.0, 0.0 };

// Robot drivers features related constants.
struct RobotFeature
{
    const char *pszName;
    int nValue;
};

static RobotFeature RobotFeatures[] =
{
    { ROB_VAL_FEATURE_PENALTIES, RM_FEATURE_PENALTIES },
    { ROB_VAL_FEATURE_TIMEDSESSION, RM_FEATURE_TIMEDSESSION },
    { ROB_VAL_FEATURE_WETTRACK, RM_FEATURE_WETTRACK },
    { ROB_VAL_FEATURE_REALWEATHER, RM_FEATURE_REALWEATHER },

    /* Career mode features not yet resurrected (robots need work to support them).
       { ROB_VAL_FEATURE_SC, RM_FEATURE_SC | RM_FEATURE_YELLOW | RM_FEATURE_PENALTIES },
       { ROB_VAL_FEATURE_YELLOW, RM_FEATURE_YELLOW | RM_FEATURE_PENALTIES },
       { ROB_VAL_FEATURE_RED, RM_FEATURE_RED },
       { ROB_VAL_FEATURE_BLUE, RM_FEATURE_BLUE },
       { ROB_VAL_FEATURE_PITEXIT, RM_FEATURE_PITEXIT | RM_FEATURE_PENALTIES },
       { ROB_VAL_FEATURE_TIMEDSESSION, RM_FEATURE_TIMEDSESSION },
       { ROB_VAL_FEATURE_PENALTIES, RM_FEATURE_PENALTIES }
    */
};
static const int NRobotFeatures = sizeof(RobotFeatures) / sizeof(RobotFeatures[0]);


GfDriver::GfDriver(const std::string& strModName, int nItfIndex,
                   const std::string& strName, void* hparmRobot)
: _strName(strName), _strModName(strModName), _nItfIndex(nItfIndex),
  _bIsHuman(false), _pCar(0), _fSkillLevel(-1.0), _nFeatures(0)
{
    load(hparmRobot);
}

void GfDriver::load(void* hparmRobot)
{
    std::ostringstream ossDrvSecPath;
    ossDrvSecPath << ROB_SECT_ROBOTS << '/' << ROB_LIST_INDEX << '/' << _nItfIndex;

    // Humanity.
    _bIsHuman =
        strcmp(GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_TYPE, ROB_VAL_ROBOT),
               ROB_VAL_ROBOT) != 0;

    // Skill level.
    const char* pszKillLevel =
        GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_LEVEL, ROB_VAL_SEMI_PRO);
    for(int nLevelInd = 0; nLevelInd < NbSkillLevels; nLevelInd++)
    {
        if (!strcmp(ASkillLevelStrings[nLevelInd], pszKillLevel))
        {
            _fSkillLevel = ASkillLevelValues[nLevelInd];
            break;
        }
    }

    // Supported features.
    if (_bIsHuman)
    {
        _nFeatures = RM_FEATURE_TIMEDSESSION | RM_FEATURE_WETTRACK;
        if (_fSkillLevel <= ASkillLevelValues[3]) // Pro (TODO: Create enum for that !)
            _nFeatures |= RM_FEATURE_PENALTIES;
    }
    else
    {
        _nFeatures = 0;
        char* pszDrvFeatures =
            strdup(GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(),
                ROB_ATTR_FEATURES, ROB_VAL_FEATURE_WETTRACK));
        for (char* pszFeature = strtok(pszDrvFeatures, ";");
             pszFeature != 0; pszFeature = strtok(NULL, ";"))
        {
            for (int nFeatInd = 0; nFeatInd < NRobotFeatures; nFeatInd++)
                if (!strcmp(pszFeature, RobotFeatures[nFeatInd].pszName))
                {
                    _nFeatures |= RobotFeatures[nFeatInd].nValue;
                    break;
                }
        }
        free(pszDrvFeatures);
    }

    // Driven car.
    const char* pszCarId = GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_CAR, "");
    _pCar = GfCars::self()->getCar(pszCarId);

    const char* skin = GfParmGetStr(hparmRobot, ossDrvSecPath.str().c_str(), ROB_ATTR_LIVERY, "standard");
    std::vector<GfDriverSkin> skins = getPossibleSkins(pszCarId);
    std::vector<GfDriverSkin>::iterator it = findSkin(skins, skin);

    if (it != skins.end())
        setSkin(*it);
}

const std::string& GfDriver::getName() const
{
    return _strName;
}

const std::string& GfDriver::getModuleName() const
{
    return _strModName;
}

int GfDriver::getInterfaceIndex() const
{
    return _nItfIndex;
}

bool GfDriver::isHuman() const
{
    return _bIsHuman;
}

bool GfDriver::isNetwork() const
{
    return _strModName == "networkhuman";
}

const GfCar* GfDriver::getCar() const
{
    return _pCar;
}

// GfCar* GfDriver::getCar()
// {
// 	return _pCar;
// }

const GfDriverSkin& GfDriver::getSkin() const
{
    return _skin;
}

std::string GfDriver::getType(const std::string& strModName)
{
    std::string strType;

    // Parse module name for last '_' char : assumed to be the separator between type
    // and instance name for ubiquitous robots (ex: simplix)
    const size_t nTruncPos = strModName.rfind('_');
    if (nTruncPos == std::string::npos)
        strType = strModName; // Copy.
    else
        strType = strModName.substr(0, nTruncPos); // Copy + truncate.

    return strType;
}

const std::string& GfDriver::getType() const
{
    if (_strType.empty())
        _strType = getType(_strModName);

    return _strType;

}

bool GfDriver::matchesTypeAndCategory(const std::string& strType,
                                      const std::string& strCarCatId) const
{
    return (strType.empty() || getType() == strType)
           && (strCarCatId.empty() || getCar()->getCategoryId() == strCarCatId);
}

double GfDriver::getSkillLevel() const
{
    return _fSkillLevel;
}

int GfDriver::getSupportedFeatures() const
{
    return _nFeatures;
}

void GfDriver::setCar(const GfCar* pCar)
{
    _pCar = pCar;
}

void GfDriver::setSkin(const GfDriverSkin& skin)
{
    _skin = skin;
}

static const char* pszLiveryTexExt = ".png";
static const char* pszPreviewTexSufx = "-preview.jpg";
static const char* pszInteriorTexExt = ".png";
static const char* pszInteriorTexSufx = "-int";
static const char* pszDriverTexName = "driver"; // Warning: Must be consistent with <car>.ac/acc
static const char* pszDriverTexExt = ".png";
static const char* pszLogoTexName = "logo"; // Warning: Must be consistent with grscene.cpp
static const char* pszLogoTexExt = ".png";
static const char* pszWheel3DTexName = "wheel3d"; // Warning: Must be consistent with wheel<i>.ac/acc
static const char* pszWheel3DTexExt = ".png";

static const char* apszExcludedSkinNamePrefixes[] = { "rpm", "speed", "int" };
static const int nExcludedSkinNamePrefixes = sizeof(apszExcludedSkinNamePrefixes) / sizeof(char*);


std::vector<GfDriverSkin>::iterator GfDriver::findSkin(std::vector<GfDriverSkin>& vecSkins,
                                                       const std::string& strName)
{
    std::vector<GfDriverSkin>::iterator itSkin;
    for (itSkin = vecSkins.begin(); itSkin != vecSkins.end(); ++itSkin)
    {
        if (itSkin->getName() == strName)
            return itSkin;
    }

    return vecSkins.end();
}


void GfDriver::getPossibleSkinsInFolder(const std::string& strCarId,
                                        const std::string& strFolderPath,
                                        std::vector<GfDriverSkin>& vecPossSkins)
{
    //GfLogDebug("  getPossibleSkinsInFolder(%s, %s) ...\n",
    //		   strCarId.c_str(), strFolderPath.c_str());

    // Search for skinned livery files, and associated preview files if any.
    tFList *pLiveryFileList =
        GfDirGetListFiltered(strFolderPath.c_str(), strCarId.c_str(), pszLiveryTexExt);
    if (pLiveryFileList)
    {
        tFList *pCurLiveryFile = pLiveryFileList;
        do
        {
            pCurLiveryFile = pCurLiveryFile->next;

            // Extract the skin name from the livery file name.
            const int nSkinNameLen = // Expecting "<car name>-<skin name>.png"
                strlen(pCurLiveryFile->name) - strCarId.length() - 1 - strlen(pszLiveryTexExt);
            std::string strSkinName;
            if (nSkinNameLen > 0) // Otherwise, default/standard "<car name>.png"
            {
                strSkinName =
                    std::string(pCurLiveryFile->name)
                    .substr(strCarId.length() + 1, nSkinNameLen);

                // Ignore skins with an excluded prefix.
                int nExclPrfxInd = 0;
                for (; nExclPrfxInd < nExcludedSkinNamePrefixes; nExclPrfxInd++)
                    if (strSkinName.find(apszExcludedSkinNamePrefixes[nExclPrfxInd]) == 0)
                        break;
                if (nExclPrfxInd < nExcludedSkinNamePrefixes)
                    continue;
            }

            // Ignore skins that are already in the list (path search priority).
            if (findSkin(vecPossSkins, strSkinName) == vecPossSkins.end())
            {
                // Create the new skin.
                GfDriverSkin skin(strSkinName);

                // Add the whole car livery to the skin targets.
                skin.addTargets(RM_CAR_SKIN_TARGET_WHOLE_LIVERY);

                GfLogDebug("  Found %s%s livery\n",
                           strSkinName.empty() ? "standard" : strSkinName.c_str(),
                           strSkinName.empty() ? "" : "-skinned");

                // Add associated preview image, without really checking file existence
                // (warn only ; up to the client GUI to do what to do if it doesn't exist).
                std::ostringstream ossPreviewName;
                ossPreviewName << strFolderPath << '/' << strCarId;
                if (!strSkinName.empty())
                    ossPreviewName << '-' << strSkinName;
                ossPreviewName << pszPreviewTexSufx;
                skin.setCarPreviewFileName(ossPreviewName.str());

                if (!GfFileExists(ossPreviewName.str().c_str()))
                    GfLogWarning("Preview file not found for %s %s skin (%s)\n",
                                 strCarId.c_str(), strSkinName.c_str(), ossPreviewName.str().c_str());
                //else
                //	GfLogDebug("* found skin=%s, preview=%s\n",
                //			   strSkinName.c_str(), ossPreviewName.str().c_str());

                // Add the new skin to the list.
                vecPossSkins.push_back(skin);
            }

        }
        while (pCurLiveryFile != pLiveryFileList);
    }

    GfDirFreeList(pLiveryFileList, NULL, true, true);

    // Search for skinned interior files, if any.
    std::string strInteriorPrefix(strCarId);
    strInteriorPrefix += pszInteriorTexSufx;
    tFList *pIntFileList =
        GfDirGetListFiltered(strFolderPath.c_str(), strInteriorPrefix.c_str(), pszInteriorTexExt);
    if (pIntFileList)
    {
        tFList *pCurIntFile = pIntFileList;
        do
        {
            pCurIntFile = pCurIntFile->next;

            // Extract the skin name from the interior file name.
            const int nSkinNameLen = // Expecting "<car name>-int-<skin name>.png"
                strlen(pCurIntFile->name) - strInteriorPrefix.length()
                - 1 - strlen(pszInteriorTexExt);
            std::string strSkinName;
            if (nSkinNameLen > 0)
            {
                strSkinName =
                    std::string(pCurIntFile->name)
                    .substr(strInteriorPrefix.length() + 1, nSkinNameLen);

                // If a skin with such name already exists in the list, update it.
                std::vector<GfDriverSkin>::iterator itSkin =
                    findSkin(vecPossSkins, strSkinName);
                if (itSkin != vecPossSkins.end())
                {
                    itSkin->addTargets(RM_CAR_SKIN_TARGET_INTERIOR);
                    GfLogDebug("  Found %s-skinned interior (targets:%x)\n",
                               strSkinName.c_str(), itSkin->getTargets());
                }
            }
        }
        while (pCurIntFile != pIntFileList);
    }

    GfDirFreeList(pIntFileList, NULL, true, true);

    // Search for skinned logo files if any.
    tFList *pLogoFileList =
        GfDirGetListFiltered(strFolderPath.c_str(), pszLogoTexName, pszLogoTexExt);
    if (pLogoFileList)
    {
        tFList *pCurLogoFile = pLogoFileList;
        do
        {
            pCurLogoFile = pCurLogoFile->next;

            // Extract the skin name from the logo file name.
            const int nSkinNameLen = // Expecting "logo-<skin name>.png"
                strlen(pCurLogoFile->name) - strlen(pszLogoTexName)
                - 1 - strlen(pszLogoTexExt);
            if (nSkinNameLen > 0)
            {
                const std::string strSkinName =
                    std::string(pCurLogoFile->name)
                    .substr(strlen(pszLogoTexName) + 1, nSkinNameLen);

                // If a skin with such name already exists in the list, update it.
                std::vector<GfDriverSkin>::iterator itSkin = findSkin(vecPossSkins, strSkinName);
                if (itSkin != vecPossSkins.end())
                {
                    itSkin->addTargets(RM_CAR_SKIN_TARGET_PIT_DOOR);
                    GfLogDebug("  Found %s-skinned logo (targets:%x)\n",
                               strSkinName.c_str(), itSkin->getTargets());
                }
            }

        }
        while (pCurLogoFile != pLogoFileList);
    }

    GfDirFreeList(pLogoFileList, NULL, true, true);

    // Search for skinned 3D wheel files if any.
    tFList *pWheel3DFileList =
        GfDirGetListFiltered(strFolderPath.c_str(), pszWheel3DTexName, pszWheel3DTexExt);
    if (pWheel3DFileList)
    {
        tFList *pCurWheel3DFile = pWheel3DFileList;
        do
        {
            pCurWheel3DFile = pCurWheel3DFile->next;

            // Extract the skin name from the 3D wheel texture file name.
            const int nSkinNameLen = // Expecting "wheel3d-<skin name>.png"
                strlen(pCurWheel3DFile->name) - strlen(pszWheel3DTexName)
                - 1 - strlen(pszWheel3DTexExt);
            if (nSkinNameLen > 0)
            {
                const std::string strSkinName =
                    std::string(pCurWheel3DFile->name)
                    .substr(strlen(pszWheel3DTexName) + 1, nSkinNameLen);

                // If a skin with such name already exists in the list, update it.
                std::vector<GfDriverSkin>::iterator itSkin = findSkin(vecPossSkins, strSkinName);
                if (itSkin != vecPossSkins.end())
                {
                    itSkin->addTargets(RM_CAR_SKIN_TARGET_3D_WHEELS);
                    GfLogDebug("  Found %s-skinned 3D wheels (targets:%x)\n",
                               strSkinName.c_str(), itSkin->getTargets());
                }
            }
        }
        while (pCurWheel3DFile != pWheel3DFileList);
    }

    GfDirFreeList(pWheel3DFileList, NULL, true, true);

    // Search for skinned driver files if any.
    tFList *pDriverFileList =
        GfDirGetListFiltered(strFolderPath.c_str(), pszDriverTexName, pszDriverTexExt);
    if (pDriverFileList)
    {
        tFList *pCurDriverFile = pDriverFileList;
        do
        {
            pCurDriverFile = pCurDriverFile->next;

            // Extract the skin name from the 3D wheel texture file name.
            const int nSkinNameLen = // Expecting "driver-<skin name>.png"
                strlen(pCurDriverFile->name) - strlen(pszDriverTexName)
                - 1 - strlen(pszDriverTexExt);
            if (nSkinNameLen > 0)
            {
                const std::string strSkinName =
                    std::string(pCurDriverFile->name)
                    .substr(strlen(pszDriverTexName) + 1, nSkinNameLen);

                // If a skin with such name already exists in the list, update it.
                std::vector<GfDriverSkin>::iterator itSkin = findSkin(vecPossSkins, strSkinName);
                if (itSkin != vecPossSkins.end())
                {
                    itSkin->addTargets(RM_CAR_SKIN_TARGET_DRIVER);
                    GfLogDebug("  Found %s-skinned driver (targets:%x)\n",
                               strSkinName.c_str(), itSkin->getTargets());
                }
            }
        }
        while (pCurDriverFile != pDriverFileList);
    }

    GfDirFreeList(pDriverFileList, NULL, true, true);
}

void GfDriver::processSkins(std::vector<GfDriverSkin> &skins,
    const std::string &car)
{
    // If we have at least 1 skin, make sure that, if the standard one is inside,
    // it is the first one.
    if (!skins.empty())
    {
        std::vector<GfDriverSkin>::iterator itSkin;
        for (itSkin = skins.begin(); itSkin != skins.end(); ++itSkin)
        {
            if (itSkin->getName().empty() && itSkin != skins.begin())
            {
                GfDriverSkin stdSkin = *itSkin;
                skins.erase(itSkin);
                skins.insert(skins.begin(), stdSkin);
                break;
            }
        }
    }

    // If no skin was found, add the car's standard one
    // (that way, the skin list will never be empty, and that's safer)
    else
    {
        GfDriverSkin stdSkin;
        std::ostringstream ossPreviewName;
        ossPreviewName << "cars/models/" << car << '/' << car << pszPreviewTexSufx;
        stdSkin.setCarPreviewFileName(ossPreviewName.str());

        if (!GfFileExists(ossPreviewName.str().c_str()))
            GfLogWarning("No preview file %s found for dummy '%s' skin\n",
                         ossPreviewName.str().c_str(), "standard");

        skins.push_back(stdSkin);
    }
}

std::vector<GfDriverSkin> GfDriver::getPossibleSkins(const std::string& strAltCarId) const
{
    const std::string strCarId = strAltCarId.empty() ? _pCar->getId() : strAltCarId;

    GfLogDebug("Checking skins for %s ...\n", strCarId.c_str());

    // Clear the skin and preview lists.
    std::vector<GfDriverSkin> vecPossSkins;

    // Get/check skins/skin targets/previews from the directories in the search path
    // WARNING: Must be consistent with the search paths used in grcar.cpp, grboard.cpp,
    //          grscene.cpp ... etc ... but it is not currently 100% achieved
    //          (pit door logos are not searched by the graphics engine
    //           in the car-dedicated folders ... so they may be "over-detected" here).
    std::ostringstream ossDirPath;
    ossDirPath << GfLocalDir() << "drivers/" << _strModName
               << '/' << _nItfIndex << '/' << strCarId;
    getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

    ossDirPath.str("");
    ossDirPath << GfLocalDir() << "drivers/" << _strModName
               << '/' << _nItfIndex;
    getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

    ossDirPath.str("");
    ossDirPath << GfLocalDir() << "drivers/" << _strModName
               << '/' << strCarId;
    getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

    ossDirPath.str("");
    ossDirPath << GfLocalDir() << "drivers/" << _strModName;
    getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

    ossDirPath.str("");
    ossDirPath << "drivers/" << _strModName
               << '/' << _nItfIndex << '/' << strCarId;
    getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

    ossDirPath.str("");
    ossDirPath << "drivers/" << _strModName
               << '/' << _nItfIndex;
    getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

    ossDirPath.str("");
    ossDirPath << "drivers/" << _strModName
               << '/' << strCarId;
    getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

    ossDirPath.str("");
    ossDirPath << "drivers/" << _strModName;
    getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

    ossDirPath.str("");
    ossDirPath << GfLocalDir() << "cars/models/" << strCarId;
    getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

    ossDirPath.str("");
    ossDirPath << "cars/models/" << strCarId;
    getPossibleSkinsInFolder(strCarId, ossDirPath.str(), vecPossSkins);

    if (vecPossSkins.empty())
        GfLogError("No skin at all found for '%s/%d/%s' : "
            "adding dummy '%s' one\n", _strModName.c_str(), _nItfIndex,
            strCarId.c_str(), "standard");

    processSkins(vecPossSkins, strCarId);
    return vecPossSkins;
}

std::vector<GfDriverSkin> GfDrivers::getskins(const std::string &car)
{
    std::vector<GfDriverSkin> skins;
    std::string local = GfLocalDir(), data = "cars/models/";

    local += "cars/models/" + car;
    data += car;

    GfDriver::getPossibleSkinsInFolder(car, local, skins);
    GfDriver::getPossibleSkinsInFolder(car, data, skins);
    GfDriver::processSkins(skins, car);
    return skins;
}

int GfDrivers::genident(struct GfDrivers::identity &ident) const
{
    size_t index;

    if (portability::rand(&index, sizeof index))
    {
        GfLogError("Failed to get random identity index\n");
        return -1;
    }

    index %= n_teams;

    ident.team = teams[index];

    if (portability::rand(&index, sizeof index))
    {
        GfLogError("Failed to get random identity index\n");
        return -1;
    }

    index %= n_names;

    const struct names &n = names[index];

    ident.nationality = n.nation;

    if (portability::rand(&index, sizeof index))
    {
        GfLogError("Failed to get random identity index\n");
        return -1;
    }

    index %= n.n_names;
    ident.name = n.names[index];
    ident.short_name = n.names[index][0];
    ident.short_name += ".";

    if (portability::rand(&index, sizeof index))
    {
        GfLogError("Failed to get random identity index\n");
        return -1;
    }

    const char *surname = n.surnames[index %= n.n_surnames];
    std::string code_name = std::string(surname).substr(0, 3);

    for (auto &c: code_name)
        c = ::toupper(c);

    ident.name += " ";
    ident.short_name += " ";
    ident.code_name = code_name;
    ident.name += surname;
    ident.short_name += surname;
    return 0;
}

int GfDrivers::pickcar(const std::string &category, std::string &car,
    std::string &livery) const
{
    std::vector<GfCar*> cars = GfCars::self()->getCarsInCategory(category);
    unsigned index;

    if (cars.empty())
    {
        GfLogError("No cars on category %s\n", category.c_str());
        return -1;
    }
    else if (portability::rand(&index, sizeof index))
    {
        GfLogError("Failed to get random car index\n");
        return -1;
    }

    index %= cars.size();
    car = cars.at(index)->getId();

    std::vector<GfDriverSkin> skins = getskins(car);

    if (skins.empty())
    {
        GfLogError("No skins on car %s\n", car.c_str());
        return -1;
    }
    else if (portability::rand(&index, sizeof index))
    {
        GfLogError("Failed to get random skin index\n");
        return -1;
    }

    index %= skins.size();
    livery = skins.at(index).getName();
    return 0;
}

int GfDrivers::genparams(const std::string &driver, const std::string &category,
    const std::string &car, const std::string &dir) const
{
    int ret = -1;
    const char *catId = category.c_str();
    std::string params = dir + "driver" PARAMEXT, selcar = car, livery;
    const char *p = params.c_str();
    void *h = GfParmReadFile(p, GFPARM_RMODE_NOREREAD | GFPARM_RMODE_CREAT);
    struct identity ident;

    if (!h)
    {
        GfLogError("GfDrivers::genparams: GfParmReadFile %s failed\n", p);
        goto end;
    }

    if (car.empty() && pickcar(category, selcar, livery))
    {
        GfLogError("Failed to pick random car/livery from category: %s\n",
            catId);
        goto end;
    }

    if (genident(ident))
    {
        GfLogError("Failed to generate driver identitity\n");
        return -1;
    }

    GfParmSetStr(h, "driver", ROB_ATTR_NAME, ident.name.c_str());
    GfParmSetStr(h, "driver", ROB_ATTR_SNAME, ident.short_name.c_str());
    GfParmSetStr(h, "driver", ROB_ATTR_CODE, ident.code_name.c_str());
    GfParmSetStr(h, "driver", ROB_ATTR_DESC, "Bot generated");
    GfParmSetStr(h, "driver", ROB_ATTR_TEAM, ident.team.c_str());
    GfParmSetStr(h, "driver", ROB_ATTR_AUTHOR, "Automatically generated");
    GfParmSetStr(h, "driver", ROB_ATTR_CAR, selcar.c_str());
    GfParmSetStr(h, "driver", ROB_ATTR_LIVERY, livery.c_str());
    GfParmSetStr(h, "driver", ROB_ATTR_RACENUM, "1");
    GfParmSetStr(h, "driver", ROB_ATTR_RED, "1.0");
    GfParmSetStr(h, "driver", ROB_ATTR_GREEN, "1.0");
    GfParmSetStr(h, "driver", ROB_ATTR_BLUE, "1.0");
    GfParmSetStr(h, "driver", ROB_ATTR_NATION, ident.nationality.c_str());

    if (GfParmWriteFile(nullptr, h, driver.c_str()))
    {
        GfLogError("GfParmWriteFile %s failed\n", p);
        goto end;
    }

    GfLogInfo("Generated driver %s with robot %s for category %s\n",
        ident.name.c_str(), driver.c_str(), category.c_str());

    ret = 0;

end:
    if (h)
        GfParmReleaseHandle(h);

    return ret;
}

bool GfDrivers::supports_aggression(const std::string &driver) const
{
    return driver == "usr" || driver == "shadow";
}

int GfDrivers::genskill(const std::string &driver, const std::string &dir) const
{
    int ret = -1;
    std::string params = dir + "skill" PARAMEXT, car;
    const char *p = params.c_str();
    void *h = GfParmReadFile(p, GFPARM_RMODE_NOREREAD | GFPARM_RMODE_CREAT);
    unsigned value;
    float valuef;

    if (portability::rand(&value, sizeof value))
    {
        GfLogError("Failed to generate random skill value\n");
        goto end;
    }

    value %= 11;
    valuef = value / 10.0f;

    if (GfParmSetNum(h, "skill", "level", nullptr, valuef))
    {
        GfLogError("Failed to set skill value\n");
        goto end;
    }
    else if (supports_aggression(driver))
    {
        if (portability::rand(&value, sizeof value))
        {
            GfLogError("Failed to generate random skill value\n");
            goto end;
        }

        // Values range from -3.0 to 1.5. However, for shadow it is not
        // recommended to exceed 0.1 to avoid accidents.
        value %= 32;
        valuef = value / 10.0f - 3.0f;

        if (GfParmSetNum(h, "skill", "aggression", nullptr, valuef))
        {
            GfLogError("Failed to set agression value\n");
            goto end;
        }
    }

    if (GfParmWriteFile(nullptr, h, "Skill"))
    {
        GfLogError("GfParmWriteFile %s failed\n", p);
        goto end;
    }

    ret = 0;

end:
    if (h)
        GfParmReleaseHandle(h);

    return ret;
}

int GfDrivers::pickdriver(std::string &out) const
{
    do
    {
        unsigned index;

        if (portability::rand(&index, sizeof index))
        {
            GfLogError("Failed to generate random driver index\n");
            return -1;
        }

        const std::vector<std::string> &t = _pPrivate->vecTypes;

        index %= t.size();
        out = t.at(index);
    } while (human(out));

    return 0;
}

int GfDrivers::gen(const std::string &driver, const std::string &category,
    const std::string &car) const
{
    const char *dir = GfLocalDir();
    std::string seldriver = driver;

    if (!dir)
    {
        GfLogError("GfLocalDir failed\n");
        return -1;
    }
    else if (seldriver.empty() && pickdriver(seldriver))
    {
        GfLogError("Failed to pick random driver\n");
        return -1;
    }

    std::string drvdir = dir;

    drvdir += "drivers/" + seldriver + "/";

    const char *d = drvdir.c_str();

    if (GfDirCreate(d) != GF_DIR_CREATED)
    {
        GfLogError("Failed to created directory: %s\n", d);
        return -1;
    }

    unsigned index = 0;

    while (GfDirExists((drvdir + std::to_string(index)).c_str()))
        index++;

    drvdir += std::to_string(index) + "/";

    if (GfDirCreate(d) != GF_DIR_CREATED)
    {
        GfLogError("Failed to created directory: %s\n", d);
        return -1;
    }
    else if (genparams(seldriver, category, car, drvdir))
    {
        GfLogError("Failed to generate driver parameters\n");
        return -1;
    }
    else if (genskill(seldriver, drvdir))
    {
        GfLogError("Failed to generate driver skill\n");
        return -1;
    }

    return 0;
}

int GfDrivers::getDriverIdx(void *h, const char *path, const char *mod) const
{
    const char *name = GfParmGetStr(h, path, RM_ATTR_DRIVERNAME, NULL);

    if (!name)
    {
        GfLogError("Attribute \"" RM_ATTR_DRIVERNAME "\" missing\n");
        return -1;
    }

    const GfDriver *driver = getDriverWithName(name, mod);

    if (!driver)
    {
        GfLogError("Driver not found: %s\n", name);
        return -1;
    }

    return driver->getInterfaceIndex();
}

int GfDrivers::getDriverIdx(const std::string &mod, const std::string &name) const
{
    int ret = -1, n;
    std::string path = "drivers/" + mod + "/" + mod + PARAMEXT;
    const char *p = path.c_str(), *pname = name.c_str(), *pmod = mod.c_str();
    static const char *prefix = ROB_SECT_ROBOTS "/" ROB_LIST_INDEX;
    void *h = GfParmReadFileLocal(p, GFPARM_RMODE_STD);

    if (!h)
    {
        GfLogError("GfParmReadFileLocal %s failed\n", p);
        goto end;
    }
    else if (!(n = GfParmGetEltNb(h, prefix)))
    {
        GfLogError("GfParmGetEltNb %s failed\n", p);
        goto end;
    }

    for (int i = 0; i < n; i++)
    {
        std::string s = std::string(prefix) + "/" + std::to_string(i);
        const char *value = GfParmGetStr(h, s.c_str(), ROB_ATTR_NAME, nullptr);

        if (!value)
            GfLogError("Missing driver name, module=%s, index=%d\n",
                pmod, i);
        else if (name == value)
        {
            ret = i;
            goto end;
        }
    }

    GfLogError("No driver found: %s, module=%s\n", pname, pmod);

end:
    if (h)
        GfParmReleaseHandle(h);

    return ret;
}

int GfDrivers::del(const std::string &mod, const std::string &driver) const
{
    const char *dir = GfLocalDir();

    if (!dir)
    {
        GfLogError("GfLocalDir failed\n");
        return -1;
    }

    int index = getDriverIdx(mod, driver);

    if (index < 0)
    {
        GfLogError("GfDrivers::del: failed to get driver index\n");
        return -1;
    }

    std::string path = dir;

    path += "drivers/" + mod + "/" + std::to_string(index);

    if (portability::rmdir_r(path.c_str()))
    {
        GfLogError("Failed to remove directory: %s\n", path.c_str());
        return -1;
    }

    return 0;
}
