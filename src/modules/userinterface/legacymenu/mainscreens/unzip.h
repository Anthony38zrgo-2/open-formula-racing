/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef unzip_H
#define unzip_H

#include <minizip/unzip.h>
#include <fstream>
#include <string>

class unzip
{
public:
    unzip(const std::string &src, const std::string &dst,
        const std::string &dir);
    ~unzip();
    int run() const;

private:
    int once() const;
    int next(bool &end) const;
    int extract(const std::string &path) const;
    int filename(std::string &out) const;
    const std::string &src, &dst, &dir;
    const unzFile f;
};

#endif
