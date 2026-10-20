/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "hash.h"
#include "sha256.h"
#include <tgfclient.h>
#include <rhash.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

bool sha256::init;

sha256::sha256()
{
    if (!init)
    {
        rhash_library_init();
        init = true;
    }
}

int sha256::run(const std::string &path, std::string &out)
{
    unsigned char result[32];

    if (rhash_file(RHASH_SHA256, path.c_str(), result))
    {
        GfLogError("rhash_file: %s\n", strerror(errno));
        return -1;
    }

    for (size_t i = 0; i < sizeof result; i++)
    {
        char s[sizeof "00"];
        int n = snprintf(s, sizeof s, "%02x", result[i]);

        if (n < 0 || n >= static_cast<int>(sizeof s))
        {
            GfLogError("snprintf(3) failed with %d\n", n);
            return -1;
        }

        out += s;
    }

    return 0;
}
