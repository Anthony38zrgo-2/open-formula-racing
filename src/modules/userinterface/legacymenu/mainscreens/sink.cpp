/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "sink.h"
#include <cstddef>
#include <iostream>

sink::sink(size_t max) :
    cleanup(false),
    max(max),
    n(0)
{
}

int sink::append(const void *buf, size_t n)
{
    return 0;
}

size_t sink::size() const
{
    return n;
}

void sink::flush()
{
}

int sink::check(size_t len) const
{
    if (len > max || n > max - len)
    {
        std::cerr << "exceeded maximum payload size: " << len
            << ", max: " << max << std::endl;
        return -1;
    }

    return 0;
}
