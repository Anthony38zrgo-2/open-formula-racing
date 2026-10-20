/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "writefile.h"
#include "tgf.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>

writefile::writefile(const char *path, size_t max, progress cb, void *args) :
    sink(max),
    path(path),
    args(args),
    cb(cb),
    f(path, std::ios::binary)
{
}

int writefile::append(const void *src, size_t len)
{
    if (check(len))
        return -1;

    f.write(static_cast<const char *>(src), len);
    n += len;

    if (cb)
        return cb(n, max, args);

    return 0;
}

void writefile::flush()
{
    f.flush();
}
