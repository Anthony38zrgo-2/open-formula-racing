/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef ASSET_H
#define ASSET_H

#include <cjson/cJSON.h>
#include <string>

class Asset
{
public:
    const enum type {car, track, driver} type;
    explicit Asset(enum type type);
    bool operator==(const Asset &other) const;
    int parse(const cJSON *c);
    std::string name, category, url, author, license, hash, hashtype,
        thumbnail, directory;
    size_t size;
    unsigned long long revision;
    std::string path() const;
    std::string basedir() const;
    std::string dstdir() const;
    int needs_update(bool &out) const;

private:
    int parse(const std::string &s, unsigned long long &size);
    int check_dir(const std::string &d) const;
    int needs_update(const std::string &path, bool &out) const;
    int needs_update_drv(bool &out) const;
};

#endif
