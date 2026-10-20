/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "entry.h"
#include "asset.h"
#include <tgf.h>
#include <tgfclient.h>
#include <cerrno>
#include <string>

entry::entry(const Asset &a) :
    a(a),
    state(init),
    thumbnail_fetched(false),
    progress(0.0f)
{
}

entry::~entry()
{
    if (!data.empty() && remove(data.c_str()))
        GfLogError("remove(3) %s: %s\n", data.c_str(), strerror(errno));

    if (!thumbnail.empty() && remove(thumbnail.c_str()))
        GfLogError("remove(3) %s: %s\n", thumbnail.c_str(), strerror(errno));
}
