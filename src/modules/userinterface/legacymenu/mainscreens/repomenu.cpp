/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024-2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "repomenu.h"
#include "downloadservers.h"
#include <tgfclient.h>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

static void deinit(void *args)
{
    delete static_cast<RepoMenu *>(args);
}

static void add(void *args)
{
    static_cast<RepoMenu *>(args)->add();
}

static void del(void *args)
{
    static_cast<RepoMenu *>(args)->del();
}

void RepoMenu::add()
{
    const char *s = GfuiEditboxGetString(hscr, url);
    char *dup = NULL;
    size_t n;
    int el;

    if (!s)
    {
        GfLogError("GfuiEditboxGetString failed\n");
        goto failure;
    }
    else if (!*s)
        goto failure;

    for (const auto &r : repos)
        if (r == s)
        {
            GfLogWarning("Repository %s already added\n", r.c_str());
            goto failure;
        }

    n = strlen(s) + 1;

    if (!(dup = static_cast<char *>(malloc(n))))
    {
        GfLogError("malloc(3): %s\n", strerror(errno));
        goto failure;
    }

    memcpy(dup, s, n);

    if ((el = GfuiScrollListGetNumberOfElements(hscr, list)) < 0)
    {
        GfLogError("GfuiScrollListGetNumberOfElements failed\n");
        return;
    }
    else if (GfuiScrollListInsertElement(hscr, list, dup, el, NULL))
    {
        GfLogError("GfuiScrollListInsertElement %s failed\n", dup);
        return;
    }

    // As opposed to most other GUI elements, edit boxes only hold a
    // reference to the text. This is why the text must be duplicated
    // somewhere else before clearing the edit box.
    GfuiEditboxSetString(hscr, url, "");
    repos.push_back(dup);
    elements.push_back(dup);
    return;

failure:
    free(dup);
}

void RepoMenu::del()
{
    void *args;
    const char *s = GfuiScrollListExtractSelectedElement(hscr, list, &args);

    if (!s)
    {
        GfLogError("GfuiScrollListExtractSelectedElement failed\n");
        return;
    }

    for (auto r = repos.begin(); r != repos.end(); r++)
    {
        if (*r == s)
        {
            repos.erase(r);
            break;
        }
    }
}

RepoMenu::~RepoMenu()
{
    for (auto e : elements)
        free(e);

    GfuiScreenRelease(hscr);
    GfuiScreenActivate(prev);
    GfuiApp().eventLoop().setRecomputeCB(recompute, args);
    cb(repos, args);
}

RepoMenu::RepoMenu(void *prevMenu,
    void (*recompute)(unsigned ms, void *args),
    void (*cb)(const std::vector<std::string> &, void *), void *args) :
    hscr(GfuiScreenCreate()),
    prev(prevMenu),
    args(args),
    recompute(recompute),
    cb(cb)
{
    if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");

    void *param = GfuiMenuLoad("repomenu.xml");

    if (!param)
        throw std::runtime_error("GfuiMenuLoad failed");
    else if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");
    else if (!GfuiMenuCreateStaticControls(hscr, param))
        throw std::runtime_error("GfuiMenuCreateStaticControls failed");
    else if (GfuiMenuCreateButtonControl(hscr, param, "back", this,
        ::deinit) < 0)
        throw std::runtime_error("GfuiMenuCreateButtonControl back failed");
    else if (GfuiMenuCreateButtonControl(hscr, param, "delete", this,
        ::del) < 0)
        throw std::runtime_error("GfuiMenuCreateButtonControl delete failed");
    else if (GfuiMenuCreateButtonControl(hscr, param, "add", this,
        ::add) < 0)
        throw std::runtime_error("GfuiMenuCreateButtonControl delete failed");
    else if ((list = GfuiMenuCreateScrollListControl(hscr, param, "repos",
        this, NULL)) < 0)
        throw std::runtime_error("GfuiMenuCreateScrollListControl failed");
    else if ((url = GfuiMenuCreateEditControl(hscr, param, "url",
        NULL, NULL, NULL)) < 0)
        throw std::runtime_error("GfuiMenuCreateEditControl failed");

    GfParmReleaseHandle(param);
    GfuiMenuDefaultKeysAdd(hscr);
    GfuiAddKey(hscr, GFUIK_ESCAPE, "Back to previous menu", this, ::deinit,
        NULL);
    GfuiScreenActivate(hscr);
    GfuiApp().eventLoop().setRecomputeCB(recompute, args);

    if (downloadservers_get(repos))
        throw std::runtime_error("downloadservers_get failed");

    for (const auto &r : repos)
    {
        int n = GfuiScrollListGetNumberOfElements(hscr, list);

        if (n < 0)
            throw std::runtime_error("GfuiScrollListGetNumberOfElements failed");
        else if (GfuiScrollListInsertElement(hscr, list, r.c_str(), n, NULL))
            throw std::runtime_error("GfuiScrollListInsertElement failed");
    }
}
