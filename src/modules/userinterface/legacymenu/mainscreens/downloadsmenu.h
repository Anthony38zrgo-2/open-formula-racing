/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef DOWNLOADSMENU_H
#define DOWNLOADSMENU_H

#include <portability.h>

#include "assets.h"
#include "entry.h"
#include "sink.h"
#include "writefile.h"
#include "thumbnail.h"
#include <curl/curl.h>
#include <list>
#include <string>
#include <utility>
#include <vector>

class DownloadsMenu
{
public:
    struct pressedargs
    {
        pressedargs(DownloadsMenu *m, thumbnail *t, entry *e)
            : m(m), t(t), e(e) {}
        bool operator==(const pressedargs &other) const
        {
            return m == other.m
                && t == other.t
                && e == other.e;
        }

        DownloadsMenu *const m;
        thumbnail *const t;
        entry *const e;
    };

    explicit DownloadsMenu(void *prevMenu);
    ~DownloadsMenu();
    void recompute(unsigned ms);
    void config();
    void config_exit(const std::vector<std::string> &repos);
    void restart(const std::vector<std::string> &repos);
    void on_filter();
    void on_category();
    void pressed(thumbnail *t);
    void on_delete(thumbnail *t);
    void on_info(thumbnail *t);
    int progress(const pressedargs *p, float pt) const;
    void prev_page();
    void next_page();
    void on_download_all();
    bool pending() const;
    void confirm_exit();

private:
    struct transfer
    {
        typedef int (DownloadsMenu::*done)(CURLcode, CURL *,
            const sink *, std::string &);
        transfer(CURL *h, done f, sink *s);

        CURL *h;
        done f;
        sink *s;
    };

    int add(const char *url, transfer::done f, sink *s, long max);
    int fetch_assets();
    int fetch_thumbnail(entry *e);
    int check(CURLcode result, CURL *h, std::string &e) const;
    int assets_fetched(CURLcode result, CURL *h, const sink *s, std::string &e);
    int asset_fetched(CURLcode result, CURL *h, const sink *s, std::string &e);
    int thumbnail_fetched(CURLcode result, CURL *h, const sink *s, std::string &e);
    int dispatch(const struct CURLMsg *m);
    void update_ui();
    bool visible(const Asset &a) const;
    void append(thumbnail *t, entry *e);
    void process(thumbnail *t, entry *e);
    int check_hash(const entry *e, const std::string &path, std::string &err) const;
    int extract(const entry *e, const std::string &src, std::string &err) const;
    int save(entry *e, const std::string &path, std::string &err) const;
    unsigned visible_entries() const;

    typedef std::pair<thumbnail *, entry *> barg;
    void *const hscr, *const prev;
    CURLM *const multi;
    std::list<transfer> transfers;
    std::vector<Assets *> assets;
    std::vector<entry *> entries;
    std::vector<thumbnail *> thumbnails;
    std::vector<barg> bargs;
    std::list<pressedargs> pargs;
    int error_label, prev_arrow, next_arrow, filter, category, cur_page, npages,
        download_all;
    unsigned offset;
};

#endif
