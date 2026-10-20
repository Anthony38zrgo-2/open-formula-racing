/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024-2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "thumbnail.h"
#include <tgfclient.h>
#include <tgf.hpp>
#include <stdexcept>
#include <string>

void thumbnail::pressed() const
{
    GfuiEnable(hscr, btn, GFUI_DISABLE);
    GfuiVisibilitySet(hscr, progress_bar, GFUI_VISIBLE);
    cb(args.t, args.args);
}

static void pressed(void *args)
{
    const thumbnail::cbargs *a = static_cast<const thumbnail::cbargs *>(args);

    a->t->pressed();
}

void thumbnail::progress(float p) const
{
    GfuiProgressbarSetValue(hscr, progress_bar, p);
}

void thumbnail::on_delete(void *args)
{
    const thumbnail::cbargs *a = static_cast<const thumbnail::cbargs *>(args);
    const thumbnail *t = a->t;

    t->delete_cb(t->args.t, t->args.args);
}

void thumbnail::on_info(void *args)
{
    const thumbnail::cbargs *a = static_cast<const thumbnail::cbargs *>(args);
    const thumbnail *t = a->t;

    t->info_cb(t->args.t, t->args.args);
}

thumbnail::thumbnail(void *hscr, void *param, const std::string &id,
    callback cb, callback delete_cb, callback info_cb, void *args) :
    args(this, args),
    img(GfuiMenuCreateStaticImageControl(hscr, param,
        (id + std::string("img")).c_str())),
    name(GfuiMenuCreateLabelControl(hscr, param,
        (id + std::string("name")).c_str())),
    size(GfuiMenuCreateLabelControl(hscr, param,
        (id + std::string("size")).c_str())),
    btn(GfuiMenuCreateButtonControl(hscr, param,
        (id + std::string("btn")).c_str(),
        const_cast<thumbnail::cbargs *>(&this->args), ::pressed)),
    progress_bar(GfuiMenuCreateProgressbarControl(hscr, param,
        (id + std::string("progress")).c_str())),
    deletebtn(GfuiMenuCreateImageButtonControl(hscr, param,
        (id + std::string("delete")).c_str(),
        const_cast<thumbnail::cbargs *>(&this->args), thumbnail::on_delete)),
    info(GfuiMenuCreateImageButtonControl(hscr, param,
        (id + std::string("info")).c_str(),
        const_cast<thumbnail::cbargs *>(&this->args), thumbnail::on_info)),
    cb(cb),
    delete_cb(delete_cb),
    info_cb(info_cb),
    hscr(hscr)
{
    if (img < 0)
        throw std::runtime_error("img failed");
    else if (name < 0)
        throw std::runtime_error("name failed");
    else if (size < 0)
        throw std::runtime_error("size failed");
    else if (btn < 0)
        throw std::runtime_error("btn failed");
    else if (progress_bar < 0)
        throw std::runtime_error("progress failed");
    else if (deletebtn < 0)
        throw std::runtime_error("deletebtn failed");
    else if (info < 0)
        throw std::runtime_error("info failed");

    clear();
}

void thumbnail::set(std::string path, const std::string &name,
    const std::string &size)
{
    GfuiVisibilitySet(hscr, img, GFUI_VISIBLE);

    if (path != this->path)
    {
        GfuiStaticImageSet(hscr, img, path.c_str());
        this->path = path;
    }

    GfuiLabelSetText(hscr, this->name, name.c_str());
    GfuiLabelSetText(hscr, this->size, size.c_str());
    GfuiEnable(hscr, info, GFUI_ENABLE);
}

void thumbnail::set(bool enable, bool show_progress,
    float p, bool show_delete) const
{
    GfuiEnable(hscr, btn, enable ? GFUI_ENABLE : GFUI_DISABLE);
    GfuiVisibilitySet(hscr, progress_bar, show_progress ? GFUI_VISIBLE : GFUI_INVISIBLE);
    GfuiVisibilitySet(hscr, deletebtn, show_delete ? GFUI_VISIBLE : GFUI_INVISIBLE);
    progress(p);
}

void thumbnail::clear() const
{
    GfuiVisibilitySet(hscr, img, GFUI_INVISIBLE);
    GfuiLabelSetText(hscr, name, "");
    GfuiLabelSetText(hscr, size, "");
    GfuiEnable(hscr, info, GFUI_DISABLE);
    set();
}
