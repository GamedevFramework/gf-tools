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
#ifndef TILESET_GENERATION_H
#define TILESET_GENERATION_H

#include <gf/Array2D.h>
#include <gf/GeometryTypes.h>
#include <gf/Id.h>
#include <gf/Image.h>
#include <gf/Polyline.h>
#include <gf/Random.h>
#include <gf/Unused.h>
#include <gf/Vector.h>

#include "TilesetData.h"

namespace gftools {

  struct Pixels {
    gf::Array2D<gf::Id, int> data;

    Pixels() = default;
    Pixels(gf::Vector2i size, gf::Id biome);

    gf::Id& operator()(gf::Vector2i pos) { return data(pos); }
    gf::Id operator()(gf::Vector2i pos) const { return data(pos); }

    void fillFrom(gf::Vector2i start, gf::Id biome);
    void checkHoles();
  };

  struct Origin {
    Origin() = default;

    Origin(gf::Id id0)
    : count(1)
    , ids{ id0, gf::InvalidId, gf::InvalidId }
    {
    }

    Origin(gf::Id id0, gf::Id id1)
    : count(2)
    , ids{ id0, id1, gf::InvalidId }
    {
    }

    Origin(gf::Id id0, gf::Id id1, gf::Id id2)
    : count(3)
    , ids{ id0, id1, id2 }
    {
    }

    int count = 0;
    gf::Id ids[3];
  };

  struct Fences {
    gf::SegmentI segments[2];
    int count = 0;
  };

  constexpr std::size_t TerrainTopLeft = 0;
  constexpr std::size_t TerrainTopRight = 1;
  constexpr std::size_t TerrainBottomLeft = 2;
  constexpr std::size_t TerrainBottomRight = 3;

  struct Tile {
    Tile() = default;
    Tile(const TileSettings& settings, gf::Id biome = gf::InvalidId);

    void checkHoles() {
      pixels.checkHoles();
    }

    Origin origin;
    Pixels pixels;
    Fences fences;
    std::vector<gf::Polyline> limits;
    std::array<gf::Id, 4> terrain;
  };

  struct Tileset {
    gf::Array2D<Tile, int> tiles;
    gf::Vector2i position;

    Tileset(gf::Vector2i size);

    Tile& operator()(gf::Vector2i pos) { return tiles(pos); }
    const Tile& operator()(gf::Vector2i pos) const { return tiles(pos); }
  };

  Tile generateFull(const TileSettings& settings, gf::Id b0);

  enum class Split {
    Horizontal, // left + right
    Vertical,   // top + bottom
  };

  // b0 is in the left|top, b1 is in the right|bottom
  Tile generateSplit(const TileSettings& settings, gf::Id b0, gf::Id b1, Split s, gf::Random& random, const Edge& edge);

  enum class Corner {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
  };

  // b0 is in the corner, b1 is in the rest
  Tile generateCorner(const TileSettings& settings, gf::Id b0, gf::Id b1, Corner c, gf::Random& random, const Edge& edge);

  // b0 is in top-left and bottom-right, b1 is in top-right and bottom-left
  Tile generateCross(const TileSettings& settings, gf::Id b0, gf::Id b1, gf::Random& random, const Edge& edge);

  enum class HSplit {
    Top,
    Bottom,
  };

  // b0 is given by split, b1 is at the left, b2 is at the right
  Tile generateHorizontalSplit(const TileSettings& settings, gf::Id b0, gf::Id b1, gf::Id b2, HSplit split, gf::Random& random, const Edge& e01, const Edge& e12, const Edge& e20);

  enum class VSplit {
    Left,
    Right,
  };

  // b0 is given by split, b1 is at the top, b2 is at the bottom
  Tile generateVerticalSplit(const TileSettings& settings, gf::Id b0, gf::Id b1, gf::Id b2, VSplit split, gf::Random& random, const Edge& e01, const Edge& e12, const Edge& e20);

  enum class Oblique {
    Up,
    Down,
  };

  // b0 is given by oblique, b1 is at the left and b2 is at the right
  Tile generateOblique(const TileSettings& settings, gf::Id b0, gf::Id b1, gf::Id b2, Oblique oblique, gf::Random& random, const Edge& e01, const Edge& e12, const Edge& e20);


  Tileset generatePlainTileset(gf::Id b0, const TilesetData& db);
  Tileset generateTwoCornersWangTileset(const Wang2& wang, gf::Random& random, const TilesetData& db);
  Tileset generateThreeCornersWangTileset(const Wang3& wang, gf::Random& random, const TilesetData& db);


}

#endif // TILESET_GENERATION_H
