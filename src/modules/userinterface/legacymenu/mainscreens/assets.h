/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef ASSETS_H
#define ASSETS_H

#include "asset.h"
#include <cjson/cJSON.h>
#include <cstddef>
#include <string>
#include <vector>

class Assets
{
public:
    Assets(const std::string &url);
    int parse(const char *s, size_t n);
    const std::vector<Asset> &get() const;
    const std::string url;

private:
    int parse(const cJSON *c, const char *key, enum Asset::type type);
    std::vector<Asset> assets;
};

#endif
