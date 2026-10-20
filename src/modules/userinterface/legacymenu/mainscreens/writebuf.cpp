/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "writebuf.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>

writebuf::writebuf(size_t max) :
    sink(max),
    buf(NULL)
{
}

writebuf::~writebuf()
{
    free(buf);
}

void *writebuf::data() const
{
    return buf;
}

int writebuf::append(const void *src, size_t len)
{
    void *dst = NULL;

    if (check(len))
        goto failure;
    else if (!(dst = realloc(buf, n + len)))
    {
        std::cerr << "realloc(3): " << strerror(errno) << std::endl;
        goto failure;
    }

    memmove(static_cast<char *>(dst) + n, src, len);
    buf = dst;
    n += len;
    return 0;

failure:
    free(dst);
    return -1;
}
