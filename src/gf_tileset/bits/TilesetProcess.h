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
#ifndef TILESET_PROCESS_H
#define TILESET_PROCESS_H

#include <gf/Array2D.h>
#include <gf/Image.h>
#include <gf/Random.h>
#include <gf/Vector.h>

#include "TilesetData.h"
#include "TilesetGeneration.h"

namespace gftools {

  struct Colors {
    gf::Array2D<gf::Color4f, int> data;

    Colors() = default;
    Colors(gf::Vector2i size);

    gf::Color4f& operator()(gf::Vector2i pos) { return data(pos); }
    gf::Color4f operator()(gf::Vector2i pos) const { return data(pos); }

    Colors extend(int space) const;
    void blit(const Colors source, gf::Vector2i offset);
    gf::Image createImage() const;
  };


  Colors colorizeTile(const Tile& tile, gf::Random& random, const TilesetData& db);

  gf::Image generateAtomPreview(const Atom& atom, gf::Random& random, const TileSettings& settings);
  gf::Image generateWang2Preview(const Wang2& wang, gf::Random& random, const TilesetData& db);
  gf::Image generateWang3Preview(const Wang3& wang, gf::Random& random, const TilesetData& db);


  struct DecoratedTileset {
    std::vector<Tileset> atoms;
    std::vector<Tileset> wang2;
    std::vector<Tileset> wang3;

    gf::Vector2i findTerrainPosition(gf::Id id) const;
  };

  DecoratedTileset generateTilesets(gf::Random& random, const TilesetData& db);

  gf::Image generateTilesetImage(gf::Random& random, const TilesetData& db, const DecoratedTileset& tilesets);
  std::string generateTilesetXml(const gf::Path& image, const TilesetData& db, const DecoratedTileset& tilesets);

}

#endif // TILESET_PROCESS_H
