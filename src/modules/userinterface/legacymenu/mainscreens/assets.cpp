/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "assets.h"
#include "asset.h"
#include <tgf.h>
#include <cjson/cJSON.h>
#include <string>
#include <vector>

int Assets::parse(const cJSON *c, const char *key, enum Asset::type type)
{
    const cJSON *array = cJSON_GetObjectItem(c, key);

    if (!array)
        // Nothing to do.
        return 0;
    else if (!cJSON_IsArray(array))
    {
        GfLogError("Expected JSON array\n");
        return -1;
    }

    const cJSON *item;

    cJSON_ArrayForEach(item, array)
    {
        Asset a(type);

        if (a.parse(item))
        {
            GfLogError("parse failed\n");
            return -1;
        }

        this->assets.push_back(a);
    }

    return 0;
}

int Assets::parse(const char *s, size_t n)
{
    cJSON *c = cJSON_ParseWithLength(s, n);

    if (!c)
    {
        GfLogError("cJSON_Parse failed\n");
        return -1;
    }
    else if (parse(c, "cars", Asset::car))
    {
        GfLogError("parse cars failed\n");
        return -1;
    }
    else if (parse(c, "tracks", Asset::track))
    {
        GfLogError("parse tracks failed\n");
        return -1;
    }
    else if (parse(c, "drivers", Asset::driver))
    {
        GfLogError("parse drivers failed\n");
        return -1;
    }

    cJSON_Delete(c);
    return 0;
}

const std::vector<Asset> &Assets::get() const
{
    return assets;
}

Assets::Assets(const std::string &url) :
    url(url)
{
}
