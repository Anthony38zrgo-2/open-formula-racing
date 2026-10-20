/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef CONFIRMMENU_H
#define CONFIRMMENU_H

#include "entry.h"

class ConfirmMenu
{
public:
    ConfirmMenu(void *prevMenu,
        void (*recompute)(unsigned, void *),
        void (*accept)(void *),
        void *args);
    ~ConfirmMenu();

private:
    static void on_accept(void *args);

    void *const hscr, *const prev, *const args;
    void (*const recompute)(unsigned, void *);
    void (*const accept)(void *);
};

#endif
