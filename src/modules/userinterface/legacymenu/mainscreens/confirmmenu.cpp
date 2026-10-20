/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "confirmmenu.h"
#include "asset.h"
#include "entry.h"
#include <tgfclient.h>
#include <tgf.hpp>
#include <stdexcept>

static void deinit(void *args)
{
    delete static_cast<ConfirmMenu *>(args);
}

void ConfirmMenu::on_accept(void *args)
{
    ConfirmMenu *m = static_cast<ConfirmMenu *>(args);
    void (*cb)(void *) = m->accept;
    void *data = m->args;

    delete m;
    cb(data);
}

ConfirmMenu::~ConfirmMenu()
{
    GfuiScreenRelease(hscr);
    GfuiScreenActivate(prev);
    GfuiApp().eventLoop().setRecomputeCB(recompute, args);
}

ConfirmMenu::ConfirmMenu(void *prevMenu, void (*recompute)(unsigned, void *),
    void (*accept)(void *), void *args) :
    hscr(GfuiScreenCreate()),
    prev(prevMenu),
    args(args),
    recompute(recompute),
    accept(accept)
{
    if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");

    void *param = GfuiMenuLoad("confirmmenu.xml");

    if (!param)
        throw std::runtime_error("GfuiMenuLoad failed");
    else if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");
    else if (!GfuiMenuCreateStaticControls(hscr, param))
        throw std::runtime_error("GfuiMenuCreateStaticControls failed");
    else if (GfuiMenuCreateButtonControl(hscr, param, "back", this, ::deinit) < 0)
        throw std::runtime_error("GfuiMenuCreateButtonControl back failed");
    else if (GfuiMenuCreateButtonControl(hscr, param, "accept", this, on_accept) < 0)
        throw std::runtime_error("GfuiMenuCreateButtonControl accept failed");

    GfuiMenuDefaultKeysAdd(hscr);
    GfuiAddKey(hscr, GFUIK_ESCAPE, "Back to previous menu", this, ::deinit, NULL);
    GfParmReleaseHandle(param);
    GfuiScreenActivate(hscr);
    GfuiApp().eventLoop().setRecomputeCB(recompute, args);
}
