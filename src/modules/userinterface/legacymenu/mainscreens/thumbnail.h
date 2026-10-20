/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024-2025 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <tgfclient.h>
#include <string>

class thumbnail
{
public:
    typedef void (*callback)(thumbnail *t, void *args);
    thumbnail(void *hscr, void *param, const std::string &id, callback cb,
        callback delete_cb, callback info_cb, void *args);
    void pressed() const;
    void set(std::string path, const std::string &name,
        const std::string &size);
    void set(bool enable = false, bool show_progress = false,
        float progress = 0.0f, bool show_delete = false) const;
    void clear() const;
    void progress(float p) const;

    struct cbargs
    {
        cbargs(thumbnail *t, void *args) : t(t), args(args) {}
        thumbnail *t; void *args;
    };

private:
    const cbargs args;
    const int img, name, size, btn, progress_bar, deletebtn, info;
    const callback cb, delete_cb, info_cb;
    void *const hscr;
    std::string path;

    static void on_delete(void *args);
    static void on_info(void *args);
};
