/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SINK_H
#define SINK_H

#include <cstddef>

class sink
{
public:
    virtual int append(const void *buf, size_t n);
    virtual ~sink() = default;
    size_t size() const;
    virtual void flush();
    bool cleanup;

protected:
    explicit sink(size_t max);
    int check(size_t len) const;
    const size_t max;
    size_t n;
};

#endif
