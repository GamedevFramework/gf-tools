/*
 * gf-tools
 * Copyright (C) 2020 Julien Bernard
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#ifndef TILESET_GUI_H
#define TILESET_GUI_H

#include <gf/Entity.h>
#include <gf/Random.h>
#include <gf/Texture.h>

#include "TilesetData.h"

namespace gftools {

  class TilesetGui : public gf::Entity {
  public:
    TilesetGui(gf::Path datafile, TilesetData& data, gf::Random& random);

    void render(gf::RenderTarget& target, const gf::RenderStates& states) override;

  private:
    gf::Path m_datafile;
    TilesetData& m_data;
    gf::Random& m_random;

    bool m_modified = false;

    // for settings
    gf::Vector2i m_size;

    // edit for atoms
    Atom m_editedAtom;
    static constexpr std::size_t NameBufferSize = 256;
    char m_nameBuffer[NameBufferSize];
    int m_pigmentChoice = 0;
    gf::Texture m_pigmentPreview;
    bool m_newAtom = false;

    // edit for wang2
    Wang2 m_editedWang2;
    int m_borderEffectChoices[2] = { 0, 0 };
    gf::Texture m_wang2Preview;
    bool m_newWang2 = false;

    // edit for wang3
    Wang3 m_editedWang3;
    gf::Id m_idsChoice[3];
    gf::Texture m_wang3Preview;
    bool m_newWang3 = false;
  };

}

#endif // TILESET_GUI_H
