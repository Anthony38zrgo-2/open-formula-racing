/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024-2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "languageconfig.h"
#include "legacymenu.h"
#include <tgfclient.h>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

static const char path[] = "config/language" PARAMEXT;

void LanguageConfig::apply(void *args)
{
    int ret = -1;
    LanguageConfig *lm = static_cast<LanguageConfig *>(args);
    void *f = nullptr;
    int flags = GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT;
    const char *lang = GfuiComboboxGetText(lm->hscr, lm->combo),
        *code = nullptr;

    if (!lang)
    {
        GfLogError("GfuiComboboxGetText failed\n");
        goto end;
    }

    for (const auto &l : lm->languages)
        if (l.name == lang)
        {
            code = l.code.c_str();
            break;
        }

    if (!code)
    {
        GfLogWarning("No language code found for language: %s\n", lang);
        goto end;
    }
    else if (!(f = GfParmReadFileLocal(path, flags)))
    {
        GfLogError("GfParmReadFileLocal failed\n");
        goto end;
    }
    else if (GfParmSetStr(f, "Language", "language", code))
    {
        GfLogError("GfParmSetStr failed\n");
        goto end;
    }
    else if (GfParmWriteFileLocal(path, f, "language"))
    {
        GfLogError("GfParmWriteFileLocal failed\n");
        goto end;
    }

    ret = 0;

end:
    if (f)
        GfParmReleaseHandle(f);

    delete lm;

    if (!ret)
    {
        LegacyMenu::self().shutdown();
        GfuiApp().restart();
    }
}
void LanguageConfig::deinit(void *args)
{
    delete static_cast<LanguageConfig *>(args);
}

void LanguageConfig::get_languages(void *param)
{
    for (int i = 1; i <= GfParmGetEltNb(param, "Languages"); i++)
    {
        const char *code, *name, *cpath;
        std::string path = "Languages/";
        lang l;

        path += std::to_string(i);
        cpath = path.c_str();
        code = GfParmGetStr(param, cpath, "code", nullptr);
        name = GfParmGetStr(param, cpath, "name", nullptr);

        if (!code)
            throw std::runtime_error("Empty language code found in list");
        else if (!name)
            throw std::runtime_error("Empty language name found in list");

        l.code = code;
        l.name = name;
        languages.push_back(l);
    }

    if (languages.empty())
    {
        lang l;

        l.code = "en";
        l.name = "English";
        languages.push_back(l);
    }
}

void LanguageConfig::set_active()
{
    const char *lang;
    void *h = GfParmReadFileLocal(path, GFPARM_RMODE_STD);

    if (!h)
    {
        GfLogError("GfParmReadFileLocal failed: %s\n", path);
        goto end;
    }

    if ((lang = GfParmGetStr(h, "Language", "language", nullptr)))
        for (size_t i = 0; i < languages.size(); i++)
        {
            if (languages.at(i).code == lang)
            {
                GfuiComboboxSetSelectedIndex(hscr, combo, i);
                break;
            }
        }

end:
    if (h)
        GfParmReleaseHandle(h);
}

LanguageConfig::~LanguageConfig()
{
    GfuiScreenRelease(hscr);
    GfuiScreenActivate(prev);
}

LanguageConfig::LanguageConfig(void *prevMenu) :
    hscr(GfuiScreenCreate()),
    prev(prevMenu)
{
    if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");

    void *param = GfuiMenuLoad("languagemenu.xml");

    if (!param)
        throw std::runtime_error("GfuiMenuLoad failed");
    else if (!hscr)
        throw std::runtime_error("GfuiScreenCreate failed");
    else if (!GfuiMenuCreateStaticControls(hscr, param))
        throw std::runtime_error("GfuiMenuCreateStaticControls failed");
    else if ((combo = GfuiMenuCreateComboboxControl(hscr, param,
        "languagecombo", this, nullptr)) < 0)
        throw std::runtime_error("GfuiMenuCreateComboboxControl failed");
    else if (GfuiMenuCreateButtonControl(hscr, param, "cancel", this, deinit) < 0)
        throw std::runtime_error("GfuiMenuCreateButtonControl cancel failed");
    else if (GfuiMenuCreateButtonControl(hscr, param, "apply", this, apply) < 0)
        throw std::runtime_error("GfuiMenuCreateButtonControl next failed");

    GfuiMenuDefaultKeysAdd(hscr);
    GfuiAddKey(hscr, GFUIK_RETURN, "Apply changes", this, apply, NULL);
    GfuiAddKey(hscr, GFUIK_ESCAPE, "Back to previous menu", this, deinit, NULL);
    get_languages(param);

    for (const auto &l : languages)
        GfuiComboboxAddText(hscr, combo, l.name.c_str());

    set_active();
    GfParmReleaseHandle(param);
    GfuiScreenActivate(hscr);
}
