/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef WRITEFILE_H
#define WRITEFILE_H

#include "sink.h"
#include <cstddef>
#include <fstream>
#include <string>

class writefile : public sink
{
public:
    typedef int (*progress)(size_t n, size_t max, void *args);
    writefile(const char *path, size_t max, progress cb = nullptr,
        void *args = nullptr);
    int append(const void *buf, size_t n);
    void flush();
    const std::string path;
    void *const args;

private:
    const progress cb;
    std::ofstream f;
};

#endif
