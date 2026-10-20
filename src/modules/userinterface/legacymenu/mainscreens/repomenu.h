/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef REPOMENU_H
#define REPOMENU_H

#include <string>
#include <vector>

class RepoMenu
{
public:
    RepoMenu(void *prevMenu,
        void (*recompute)(unsigned ms, void *args),
        void (*cb)(const std::vector<std::string > &, void *), void *args);
    ~RepoMenu();
    void add();
    void del();

private:
    void *const hscr, *const prev, *const args;
    void (*const recompute)(unsigned ms, void *args);
    void (*const cb)(const std::vector<std::string> &, void *);
    std::vector<std::string> repos;
    std::vector<char *> elements;
    int list, url;
};

#endif
