/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef WRITEBUF_H
#define WRITEBUF_H

#include "sink.h"
#include <cstddef>

class writebuf : public sink
{
public:
    explicit writebuf(size_t max);
    ~writebuf();
    int append(const void *buf, size_t n);
    void *data() const;

private:
    void *buf;
};

#endif
