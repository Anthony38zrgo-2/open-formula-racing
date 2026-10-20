/*
 * Speed Dreams, a free and open source motorsport simulator.
 * Copyright (C) 2024 Xavier Del Campo Romero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "unzip.h"
#include <tgf.h>
#include <minizip/unzip.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

int unzip::filename(std::string &out) const
{
    char *filename = NULL;
    unz_file_info info;
    int ret = -1,
        error = unzGetCurrentFileInfo(f, &info, NULL, 0, NULL, 0, NULL, 0);
    size_t n;

    if (error != UNZ_OK)
    {
        GfLogError("unzGetCurrentFileInfo %s failed with %d\n", src.c_str(),
            error);
        goto end;
    }
    else if (!info.size_filename)
    {
        GfLogError("%s: unexpected empty filename size\n", src.c_str());
        goto end;
    }

    n = info.size_filename + 1;

    if (!(filename = static_cast<char *>(malloc(n))))
    {
        GfLogError("malloc(3): %s\n", strerror(errno));
        goto end;
    }
    else if ((error = unzGetCurrentFileInfo(f, &info, filename,
        info.size_filename, NULL, 0, NULL, 0))
        != UNZ_OK)
    {
        GfLogError("unzGetCurrentFileInfo %s failed with %d\n", src.c_str(),
            error);
        goto end;
    }

    filename[n - 1] = '\0';
    out = filename;
    ret = 0;

end:
    free(filename);
    return ret;
}

int unzip::extract(const std::string &path) const
{
    std::ofstream out(path, std::ios::binary);
    int ret = -1, error = unzOpenCurrentFile(f);

    if (error)
    {
        GfLogError("%s: unzOpenCurrentFile %s failed with %d\n",
            src.c_str(), path.c_str(), error);
        return -1;
    }
    else if (!out.is_open())
    {
        GfLogError("Failed to open %s for writing\n", path.c_str());
        return -1;
    }

    for (;;)
    {
        char buf[BUFSIZ];
        int n = unzReadCurrentFile(f, buf, sizeof buf);

        if (!n)
            break;
        else if (n < 0)
        {
            GfLogError("%s: unzReadCurrentFile %s failed with %d\n",
                src.c_str(), path.c_str(), n);
            goto end;
        }

        try
        {
            out.write(buf, n);
        }
        catch (const std::ios_base::failure &failure)
        {
            GfLogError("Failed to write %d bytes\n", n);
            return -1;
        }
    }

    ret = 0;

end:
    if ((error = unzCloseCurrentFile(f)))
    {
        GfLogError("%s: unzOpenCurrentFile %s failed with %d\n",
            src.c_str(), path.c_str(), error);
        ret = -1;
    }

    return ret;
}

int unzip::next(bool &end) const
{
    int res = unzGoToNextFile(f);

    switch (res)
    {
        case UNZ_END_OF_LIST_OF_FILE:
            end = true;
            break;

        case UNZ_OK:
            end = false;
            break;

        default:
            GfLogError("unzGoToNextFile %s failed with %d\n", src.c_str(),
                res);
            return -1;
    }

    return 0;
}

int unzip::once() const
{
    std::string name, exp = dir + "/";

    if (filename(name))
    {
        GfLogError("%s: failed to extract current filename\n", src.c_str());
        return -1;
    }

    std::string::size_type n = name.find(exp);

    if (n == std::string::npos || n != 0)
    {
        GfLogInfo("%s: ignoring entry %s\n", src.c_str(), name.c_str());
        return 0;
    }

    std::string abspath = dst + name;
    std::string::size_type slash = abspath.rfind('/');
    std::string dir = abspath.substr(0, slash);
    auto last = name.rbegin();

    if (slash == std::string::npos)
    {
        GfLogInfo("%s: ignoring entry %s\n", src.c_str(), name.c_str());
        return 0;
    }
    else if (GfDirCreate(dir.c_str()) != GF_DIR_CREATED)
    {
        GfLogError("%s: failed to create directory %s\n", dir.c_str(),
            name.c_str());
        return -1;
    }
    else if (*last != '/' && extract(abspath))
    {
            GfLogError("%s: failed to extract %s\n", src.c_str(),
                name.c_str());
            return -1;
    }

    return 0;
}

int unzip::run() const
{
    int res = unzGoToFirstFile(f);

    if (res != UNZ_OK)
    {
        GfLogError("unzLocateFile %s failed with %d\n", src.c_str(), res);
        return -1;
    }

    for (;;)
    {
        bool end;

        if (once() || next(end))
            return -1;
        else if (end)
            break;
    }

    return 0;
}

unzip::unzip(const std::string &src, const std::string &dst,
    const std::string &dir) :
    src(src),
    dst(dst),
    dir(dir),
    f(unzOpen(src.c_str()))
{
    if (!f)
    {
        std::string s = "unzOpen ";

        s += src;
        s += " failed";
        throw std::runtime_error(s);
    }
}

unzip::~unzip()
{
    if (f)
    {
        int res = unzClose(f);

        if (res != UNZ_OK)
            GfLogError("unzClose %s failed with %d\n", src.c_str(), res);
    }
}
