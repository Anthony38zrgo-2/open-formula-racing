/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef ENTRY_H
#define ENTRY_H

#include "asset.h"
#include <string.h>

struct entry
{
    entry(const Asset &a);
    ~entry();
    const Asset a;
    std::string thumbnail;
    enum {init, download, update, fetching, done} state;
    bool thumbnail_fetched;
    float progress;
    std::string data;

private:
    int rename();
};

#endif
