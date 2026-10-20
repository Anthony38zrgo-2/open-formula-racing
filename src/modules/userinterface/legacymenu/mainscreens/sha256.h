/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SHA256_H
#define SHA256_H

#include "hash.h"
#include <string>

class sha256 : public hash
{
public:
    sha256();
    int run(const std::string &path, std::string &hash);

private:
    static bool init;
};

#endif
