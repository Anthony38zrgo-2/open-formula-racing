/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef INFOMENU_H
#define INFOMENU_H

#include "entry.h"

class InfoMenu
{
public:
    InfoMenu(void *prevMenu, void (*recompute)(unsigned, void *),
        void *args, const entry *entry);
    ~InfoMenu();

private:
    void set_info(void *param, const Asset &a);

    void *const hscr, *const prev, *const args;
    void (*const recompute)(unsigned, void *);
    const entry *e;
    int name, img, category, author, license, revision, url, hashtype, hash;
};

#endif
