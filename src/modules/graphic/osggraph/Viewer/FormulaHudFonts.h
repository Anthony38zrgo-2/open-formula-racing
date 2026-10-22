/***************************************************************************

    file        : FormulaHudFonts.h
    created     : 2026-10-22
    copyright   : (C) 2026 Open Formula

    Libron typography of the Formula HUD: every label, number, unit and symbol of the
    migrated components is drawn with Libron Regular or Libron Bold. The loader also
    verifies that the font files actually provide the glyphs the HUD needs, so a missing
    character cannot degrade silently into a fallback face.

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _FORMULA_HUD_FONTS_H_
#define _FORMULA_HUD_FONTS_H_

#include <map>
#include <string>
#include <vector>

#include <osg/ref_ptr>

namespace osgText
{
    class Font;
    class Text;
}

namespace osggraph
{
namespace formulahud
{

class Fonts
{
public:
    Fonts();
    ~Fonts();

    //! Loads both faces and audits their glyph coverage. Returns false when a face is
    //! missing or a required glyph is absent, after logging the concrete reason.
    bool load();

    bool isLoaded() const { return _regular.valid() && _bold.valid(); }

    osgText::Font* regular() const { return _regular.get(); }
    osgText::Font* bold() const { return _bold.get(); }

    //! Width of `text` in pixels at `sizePx`, measured with the Libron face itself.
    float measure(bool boldFace, const std::string& text, float sizePx) const;

    //! Every character the migrated components can draw. Audited at load time.
    static const std::string& requiredCharacters();

private:
    osg::ref_ptr<osgText::Font> _regular;
    osg::ref_ptr<osgText::Font> _bold;
    mutable osg::ref_ptr<osgText::Text> _measureText;
    mutable std::map<std::string, float> _cache;
};

} // namespace formulahud
} // namespace osggraph

#endif // _FORMULA_HUD_FONTS_H_
