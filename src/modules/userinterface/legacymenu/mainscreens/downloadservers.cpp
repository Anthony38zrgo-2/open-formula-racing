#include "downloadservers.h"
#include <tgf.h>
#include <string>
#include <vector>

static const char path[] = "config/downloadservers.xml",
    section[] = "Downloads";

int downloadservers_get(std::vector<std::string> &urls)
{
    int ret = -1, num;
    void *f = GfParmReadFileLocal(path, GFPARM_RMODE_REREAD);

    if (!f)
    {
        GfLogError("GfParmReadFileLocal failed\n");
        goto end;
    }
    else if ((num = GfParmGetNum(f, section, "num", NULL, -1)) < 0)
    {
        GfLogError("GfParmGetNum failed\n");
        goto end;
    }

    for (int i = 0; i < num; i++)
    {
        std::string url_s = "url";

        url_s += std::to_string(i);

        const char *url = GfParmGetStr(f, section, url_s.c_str(), NULL);

        if (!url)
        {
            GfLogError("GfParmGetStr url%u failed\n", i);
            goto end;
        }

        for (const auto &u : urls)
            if (u == url)
            {
                GfLogError("Found duplicate URL %s failed\n", url);
                goto end;
            }

        urls.push_back(url);
    }

    ret = 0;

end:
    if (f)
        GfParmReleaseHandle(f);

    return ret;
}

int downloadservers_set(const std::vector<std::string> &urls)
{
    int ret = -1;
    enum {flags = GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT};
    void *f = GfParmReadFileLocal(path, flags);

    if (!f)
    {
        GfLogError("GfParmReadFileLocal failed\n");
        goto end;
    }
    else if (GfParmSetNum(f, section, "num", NULL, urls.size()))
    {
        GfLogError("GfParmSetStr num failed\n");
        goto end;
    }

    for (size_t i = 0; i < urls.size(); i++)
    {
        std::string key = "url";
        const std::string &u = urls.at(i);

        key += std::to_string(i);

        if (GfParmSetStr(f, section, key.c_str(), u.c_str()))
        {
            GfLogError("GfParmSetStr %zu failed\n", i);
            goto end;
        }
    }

    if (GfParmWriteFileLocal(path, f, "downloadservers"))
    {
        GfLogError("GfParmWriteFileLocal failed\n");
        goto end;
    }

    ret = 0;

end:
    if (f)
        GfParmReleaseHandle(f);

    return ret;
}
