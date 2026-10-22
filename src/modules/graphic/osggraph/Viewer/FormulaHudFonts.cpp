/***************************************************************************

    file        : FormulaHudFonts.cpp
    created     : 2026-10-22
    copyright   : (C) 2026 Open Formula

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osgText/Text>

#include "FormulaHudFonts.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>

#include <osgText/Font>

#include <tgfclient.h>

namespace osggraph
{
namespace formulahud
{

namespace
{
    //! Every character the migrated components can draw. The degree sign, the colon and
    //! the slash are part of the design, so they are audited like the digits.
    const char* const kRequiredCharacters =
        " 0123456789:.+-/%()"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "\u00b0\u00ba\u00e1\u00e9\u00ed\u00f3\u00fa\u00c1\u00c9\u00cd\u00d3\u00da\u00f1\u00d1";

    unsigned int readU16(const std::vector<unsigned char>& data, size_t offset)
    {
        if (offset + 2 > data.size())
            return 0;
        return ((unsigned int)data[offset] << 8) | (unsigned int)data[offset + 1];
    }

    unsigned int readU32(const std::vector<unsigned char>& data, size_t offset)
    {
        if (offset + 4 > data.size())
            return 0;
        return ((unsigned int)data[offset] << 24) | ((unsigned int)data[offset + 1] << 16)
            | ((unsigned int)data[offset + 2] << 8) | (unsigned int)data[offset + 3];
    }

    //! Reads the Unicode coverage of a TrueType file straight from its `cmap` table.
    //! Format 4 (BMP segment mapping) is the one every desktop TTF carries; format 12
    //! is parsed as well so the audit stays correct if the asset is ever replaced.
    std::vector<unsigned int> readCmapFile(const std::string& path)
    {
        std::vector<unsigned int> codepoints;
        std::ifstream stream(path.c_str(), std::ios::binary);
        if (!stream.good())
            return codepoints;
        std::vector<unsigned char> data((std::istreambuf_iterator<char>(stream)),
                                        std::istreambuf_iterator<char>());
        if (data.size() < 12)
            return codepoints;
        const unsigned int tableCount = readU16(data, 4);
        size_t cmapOffset = 0;
        for (unsigned int table = 0; table < tableCount; ++table)
        {
            const size_t entry = 12 + (size_t)table * 16;
            if (entry + 16 > data.size())
                break;
            if (std::memcmp(&data[entry], "cmap", 4) == 0)
            {
                cmapOffset = readU32(data, entry + 8);
                break;
            }
        }
        if (cmapOffset == 0 || cmapOffset + 4 > data.size())
            return codepoints;
        const unsigned int subtableCount = readU16(data, cmapOffset + 2);
        size_t format4 = 0;
        size_t format12 = 0;
        for (unsigned int subtable = 0; subtable < subtableCount; ++subtable)
        {
            const size_t record = cmapOffset + 4 + (size_t)subtable * 8;
            if (record + 8 > data.size())
                break;
            const unsigned int platform = readU16(data, record);
            const size_t offset = cmapOffset + readU32(data, record + 4);
            const unsigned int format = readU16(data, offset);
            if (format == 4 && (platform == 0 || platform == 3) && format4 == 0)
                format4 = offset;
            if (format == 12 && (platform == 0 || platform == 3) && format12 == 0)
                format12 = offset;
        }
        if (format4 != 0)
        {
            const unsigned int segCount = readU16(data, format4 + 6) / 2;
            const size_t endOffsets = format4 + 14;
            const size_t startOffsets = endOffsets + segCount * 2 + 2;
            const size_t idDeltas = startOffsets + segCount * 2;
            const size_t idRangeOffsets = idDeltas + segCount * 2;
            const unsigned int tableLength = readU16(data, format4 + 2);
            for (unsigned int segment = 0; segment < segCount; ++segment)
            {
                const unsigned int end = readU16(data, endOffsets + segment * 2);
                const unsigned int start = readU16(data, startOffsets + segment * 2);
                const int delta = (int)(short)readU16(data, idDeltas + segment * 2);
                const unsigned int rangeOffset = readU16(data, idRangeOffsets + segment * 2);
                if (start == 0xFFFFu || start > end)
                    continue;
                for (unsigned int codepoint = start; codepoint <= end && codepoint < 0xFFFFu;
                     ++codepoint)
                {
                    unsigned int glyph = 0;
                    if (rangeOffset == 0)
                    {
                        glyph = (codepoint + (unsigned int)delta) & 0xFFFFu;
                    }
                    else
                    {
                        const size_t position = idRangeOffsets + segment * 2 + rangeOffset
                            + (codepoint - start) * 2;
                        if (position + 2 > format4 + tableLength || position + 2 > data.size())
                            continue;
                        glyph = readU16(data, position);
                        if (glyph != 0)
                            glyph = (glyph + (unsigned int)delta) & 0xFFFFu;
                    }
                    if (glyph != 0)
                        codepoints.push_back(codepoint);
                }
            }
        }
        else if (format12 != 0)
        {
            const unsigned int groupCount = readU32(data, format12 + 12);
            for (unsigned int group = 0; group < groupCount; ++group)
            {
                const size_t entry = format12 + 16 + (size_t)group * 12;
                const unsigned int start = readU32(data, entry);
                const unsigned int end = readU32(data, entry + 4);
                for (unsigned int codepoint = start; codepoint <= end && codepoint < 0x10000u;
                     ++codepoint)
                    codepoints.push_back(codepoint);
            }
        }
        std::sort(codepoints.begin(), codepoints.end());
        codepoints.erase(std::unique(codepoints.begin(), codepoints.end()), codepoints.end());
        return codepoints;
    }

    std::string describeCharacter(unsigned int codepoint)
    {
        char buffer[32];
        if (codepoint >= 0x20 && codepoint < 0x7F)
            std::snprintf(buffer, sizeof(buffer), "'%c' (U+%04X)", (char)codepoint, codepoint);
        else
            std::snprintf(buffer, sizeof(buffer), "U+%04X", codepoint);
        return std::string(buffer);
    }
}

Fonts::Fonts()
{
}

Fonts::~Fonts()
{
}

const std::string& Fonts::requiredCharacters()
{
    static const std::string characters(kRequiredCharacters);
    return characters;
}

bool Fonts::load()
{
    const std::string directory = std::string(GetDataDir()) + "data/fonts/Libron/";
    const std::string regularPath = directory + "Libron-Regular.ttf";
    const std::string boldPath = directory + "Libron-Bold.ttf";

    _regular = osgText::readFontFile(regularPath);
    _bold = osgText::readFontFile(boldPath);
    if (!_regular.valid() || !_bold.valid())
    {
        GfLogError("Formula HUD: cannot load Libron from %s (regular=%d bold=%d)\n",
                   directory.c_str(), _regular.valid() ? 1 : 0, _bold.valid() ? 1 : 0);
        return false;
    }

    // Glyph audit: a partial font would silently substitute glyphs from another face, so
    // every character the components can draw is checked against both files.
    bool complete = true;
    const char* faces[2] = { regularPath.c_str(), boldPath.c_str() };
    for (int face = 0; face < 2; ++face)
    {
        const std::vector<unsigned int> coverage = readCmapFile(faces[face]);
        std::string missing;
        const std::string& required = requiredCharacters();
        // UTF-8 is walked as whole code points, so a multi-byte character is audited as
        // one glyph instead of as its continuation bytes.
        const size_t length = required.size();
        for (size_t index = 0; index < length;)
        {
            const unsigned char lead = (unsigned char)required[index];
            unsigned int codepoint = lead;
            size_t step = 1;
            if (lead >= 0xF0)
            {
                codepoint = lead & 0x07u;
                step = 4;
            }
            else if (lead >= 0xE0)
            {
                codepoint = lead & 0x0Fu;
                step = 3;
            }
            else if (lead >= 0xC0)
            {
                codepoint = lead & 0x1Fu;
                step = 2;
            }
            if (index + step <= length)
            {
                for (size_t extra = 1; extra < step; ++extra)
                    codepoint = (codepoint << 6) | ((unsigned char)required[index + extra] & 0x3Fu);
                if (codepoint >= 0x20
                    && !std::binary_search(coverage.begin(), coverage.end(), codepoint))
                {
                    if (!missing.empty())
                        missing += ", ";
                    missing += describeCharacter(codepoint);
                }
            }
            index += step;
        }
        if (!missing.empty())
        {
            GfLogError("Formula HUD: Libron face %s lacks required glyphs: %s\n",
                       faces[face], missing.c_str());
            complete = false;
        }
        else
        {
            GfLogInfo("Formula HUD: Libron face %s provides every required glyph (%u code points)\n",
                      faces[face], (unsigned int)coverage.size());
        }
    }
    return complete;
}

float Fonts::measure(bool boldFace, const std::string& text, float sizePx) const
{
    if (!isLoaded() || sizePx <= 0.0f)
        return 0.0f;
    char key[128];
    std::snprintf(key, sizeof(key), "%d|%.2f|%s", boldFace ? 1 : 0, sizePx, text.c_str());
    std::map<std::string, float>::const_iterator cached = _cache.find(key);
    if (cached != _cache.end())
        return cached->second;

    if (!_measureText.valid())
    {
        _measureText = new osgText::Text;
        _measureText->setAlignment(osgText::Text::LEFT_BOTTOM_BASE_LINE);
        _measureText->setFontResolution(200, 200);
    }
    _measureText->setFont(boldFace ? bold() : regular());
    _measureText->setCharacterSize(sizePx);
    _measureText->setText(text, osgText::String::ENCODING_UTF8);
    const osg::BoundingBox box = _measureText->getBoundingBox();
    const float width = box.valid() ? (box.xMax() - box.xMin()) : 0.0f;
    _cache[key] = width;
    return width;
}

} // namespace formulahud
} // namespace osggraph
