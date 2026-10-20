/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef LANGUAGECONFIG_H
#define LANGUAGECONFIG_H

#include <tgfclient.h>

class LanguageConfig
{
public:
    LanguageConfig(void *prevMenu);

private:
    ~LanguageConfig();
    void get_languages(void *param);
    void set_active();
    static void on_change(tComboBoxInfo *args);
    static void apply(void *args);
    static void deinit(void *args);
    struct lang
    {
        std::string code, name;
    };

    void *const hscr, *const prev;
    std::vector<lang> languages;
    int combo;
};

#endif
