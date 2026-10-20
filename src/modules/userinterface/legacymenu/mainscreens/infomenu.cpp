/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "infomenu.h"
#include "asset.h"
#include "entry.h"
#include <tgfclient.h>
#include <tgf.hpp>
#include <stdexcept>

static void deinit(void *args)
{
    delete static_cast<InfoMenu *>(args);
}

void InfoMenu::set_info(void *param, const Asset &a)
{
    static const std::string::size_type max = 32, smallmax = 96;

    struct
    {
        int &id;
        const char *key;
        const std::string &value;
        std::string::size_type max;
    } fields[] =
    {
        {name, "name", a.name, max},
        {category, "category", a.category, max},
        {author, "author", a.author, max},
        {license, "license", a.license, max},
        {revision, "revision", std::to_string(a.revision), max},
        {hashtype, "hashtype", a.hashtype, max},
        {hash, "hash", a.hash, smallmax},
        {url, "url", a.url, smallmax}
    };

    for (const auto &f : fields)
    {
        if ((f.id = GfuiMenuCreateLabelControl(hscr, param, f.key)) < 0)
            throw std::runtime_error(
                std::string("GfuiMenuCreateLabelControl failed: ") + f.key);

        std::string value = f.value;

        if (value.length() > f.max)
        {
            value = value.substr(0, f.max);
            value += "...";
        }

        GfuiLabelSetText(hscr, f.id, value.c_str());
    }

    GfuiLabelSetText(hscr, name, a.name.c_str());
}

InfoMenu::~InfoMenu()
{
    GfuiScreenRelease(hscr);
    GfuiScreenActivate(prev);
    GfuiApp().eventLoop().setRecomputeCB(recompute, args);
}

InfoMenu::InfoMenu(void *prevMenu, void (*recompute)(unsigned, void *),
    void *args, const entry *e) :
    hscr(GfuiScreenCreate()),
    prev(prevMenu),
    args(args),
    recompute(recompute),
    e(e)
{
    if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");

    void *param = GfuiMenuLoad("infomenu.xml");

    if (!param)
        throw std::runtime_error("GfuiMenuLoad failed");
    else if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");
    else if (!GfuiMenuCreateStaticControls(hscr, param))
        throw std::runtime_error("GfuiMenuCreateStaticControls failed");
    else if (GfuiMenuCreateButtonControl(hscr, param, "back", this, ::deinit) < 0)
        throw std::runtime_error("GfuiMenuCreateButtonControl back failed");
    else if ((img = GfuiMenuCreateStaticImageControl(hscr, param, "img")) < 0)
        throw std::runtime_error("GfuiMenuCreateStaticImageControl img failed");

    const Asset &a = e->a;

    GfuiMenuDefaultKeysAdd(hscr);
    GfuiAddKey(hscr, GFUIK_ESCAPE, "Back to previous menu", this, ::deinit, NULL);
    GfuiStaticImageSet(hscr, img, e->thumbnail.c_str());
    set_info(param, a);
    GfParmReleaseHandle(param);
    GfuiScreenActivate(hscr);
    GfuiApp().eventLoop().setRecomputeCB(recompute, args);
}
