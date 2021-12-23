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
#include "TilesetGeneration.h"

#include <algorithm>
#include <queue>

#include <gf/Color.h>
#include <gf/Geometry.h>
#include <gf/Log.h>
#include <gf/Span.h>
#include <gf/VectorOps.h>

namespace gftools {

  /*
   * Pixels
   */

  Pixels::Pixels(gf::Vector2i size, gf::Id biome)
  : data(size, biome)
  {
  }

  void Pixels::fillFrom(gf::Vector2i start, gf::Id biome) {
    data(start) = biome;

    std::queue<gf::Vector2i> q;
    q.push(start);

    while (!q.empty()) {
      auto curr = q.front();

      assert(data(curr) == biome);

      for (auto next : data.get4NeighborsRange(curr)) {
        if (data(next) == gf::InvalidId) {
          data(next) = biome;
          q.push(next);
        }
      }

      q.pop();
    }
  }

  void Pixels::checkHoles() {
    for (auto pos : data.getPositionRange()) {
      gf::Id id = data(pos);

      if (id == gf::InvalidId) {
        auto& invalid = data(pos);

        for (auto next : data.get8NeighborsRange(pos)) {
          gf::Id nextBiome = data(next);

          if (nextBiome != gf::InvalidId) {
            invalid = nextBiome;
          }
        }
      }

      assert(data(pos) != gf::InvalidId);
    }
  }


  /*
   * Tile
   */

  Tile::Tile(const TileSettings& settings, gf::Id biome)
  : pixels(settings.getTileSize(), biome)
  {
  }

  /*
   * Tileset
   */

  Tileset::Tileset(gf::Vector2i size)
  : tiles(size)
  , position(-1, -1)
  {
  }

  /*
   * Tiles generators
   */

  namespace {

    constexpr gf::Vector2i top(const TileSettings& settings, int i) {
      gf::unused(settings);
      return { i, 0 };
    }

    constexpr gf::Vector2i fenceTop(const TileSettings& settings, int i) {
      return top(settings, i);
    }

    constexpr gf::Vector2i bottom(const TileSettings& settings, int i) {
      return { i, settings.size - 1 };
    }

    constexpr gf::Vector2i fenceBottom(const TileSettings& settings, int i) {
      return { i, settings.size };
    }

    constexpr gf::Vector2i left(const TileSettings& settings, int i) {
      gf::unused(settings);
      return { 0, i };
    }

    constexpr gf::Vector2i fenceLeft(const TileSettings& settings, int i) {
      return left(settings, i);
    }

    constexpr gf::Vector2i right(const TileSettings& settings, int i) {
      return { settings.size - 1, i };
    }

    constexpr gf::Vector2i fenceRight(const TileSettings& settings, int i) {
      return { settings.size, i };
    }

    constexpr gf::Vector2i cornerTopLeft(const TileSettings& settings) {
      gf::unused(settings);
      return { 0, 0 };
    }

    constexpr gf::Vector2i cornerTopRight(const TileSettings& settings) {
      return { settings.size - 1, 0 };
    }

    constexpr gf::Vector2i cornerBottomLeft(const TileSettings& settings) {
      return { 0, settings.size - 1 };
    }

    constexpr gf::Vector2i cornerBottomRight(const TileSettings& settings) {
      return { settings.size - 1, settings.size - 1 };
    }

    std::vector<gf::Vector2i> makeLine(const TileSettings& settings, gf::Span<const gf::Vector2i> points, gf::Random& random, const Displacement& displacement) {
      // generate random line points

      std::vector<gf::Vector2i> tmp;

      for (std::size_t i = 0; i < points.getSize() - 1; ++i) {
        auto line = gf::midpointDisplacement1D(points[i], points[i + 1], random, displacement.iterations, displacement.initial, displacement.reduction);
        tmp.insert(tmp.end(), line.begin(), line.end());
        tmp.pop_back();
      }

      tmp.push_back(points[points.getSize() - 1]);

      // normalize

      for (auto& point : tmp) {
        point = gf::clamp(point, 0, settings.size - 1);
      }

      // compute final line

      std::vector<gf::Vector2i> out;

      for (std::size_t i = 0; i < tmp.size() - 1; ++i) {
        auto line = gf::generateLine(tmp[i], tmp[i + 1]);
        out.insert(out.end(), line.begin(), line.end());
      }

      out.push_back(points[points.getSize() - 1]);

      return out;
    }

  }

  /*
   * Two Corner Wang Tileset generators
   */

  Tile generateFull(const TileSettings& settings, gf::Id b0) {
    Tile tile(settings, b0);
    tile.origin = Origin(b0);
    tile.terrain[TerrainTopLeft] = tile.terrain[TerrainTopRight] = tile.terrain[TerrainBottomLeft] = tile.terrain[TerrainBottomRight] = b0;
    return tile;
  }


  // b0 is in the left|top, b1 is in the right|bottom
  Tile generateSplit(const TileSettings& settings, gf::Id b0, gf::Id b1, Split s, gf::Random& random, const Edge& edge) {
    Tile tile(settings);
    tile.origin = Origin(b0, b1);

    gf::Vector2i endPoints[2];
    int half = settings.size / 2;

    switch (s) {
      case Split::Horizontal:
        endPoints[0] = left(settings, half + edge.offset);
        endPoints[1] = right(settings, half + edge.offset);
        break;
      case Split::Vertical:
        endPoints[0] = top(settings, half + edge.offset);
        endPoints[1] = bottom(settings, half + edge.offset);
        break;
    }

    auto line = makeLine(settings, endPoints, random, edge.displacement);

    for (auto point : line) {
      tile.pixels(point) = b1;
    }

    tile.pixels.fillFrom(cornerTopLeft(settings), b0);
    tile.pixels.fillFrom(cornerBottomRight(settings), b1);

    switch (s) {
      case Split::Horizontal:
        tile.terrain[TerrainTopLeft] = tile.terrain[TerrainTopRight] = b0;
        tile.terrain[TerrainBottomLeft] = tile.terrain[TerrainBottomRight] = b1;
        break;

      case Split::Vertical:
        tile.terrain[TerrainTopLeft] = tile.terrain[TerrainBottomLeft] = b0;
        tile.terrain[TerrainTopRight] = tile.terrain[TerrainBottomRight] = b1;
        break;
    }

    if (edge.limit) {
      tile.fences.count = 1;

      switch (s) {
      case Split::Horizontal:
        tile.fences.segments[0].p0 = fenceLeft(settings, half + edge.offset);
        tile.fences.segments[0].p1 = fenceRight(settings, half + edge.offset);
        break;
      case Split::Vertical:
        tile.fences.segments[0].p0 = fenceTop(settings, half + edge.offset);
        tile.fences.segments[0].p1 = fenceBottom(settings, half + edge.offset);
        break;
      }
    }

    tile.checkHoles();
    return tile;
  }

  // b0 is in the corner, b1 is in the rest
  Tile generateCorner(const TileSettings& settings, gf::Id b0, gf::Id b1, Corner c, gf::Random& random, const Edge& edge) {
    Tile tile(settings);
    tile.origin = Origin(b0, b1);

    gf::Vector2i endPoints[2];
    int half = settings.size / 2;

    switch (c) {
      case Corner::TopLeft:
        endPoints[0] = top(settings, half - 1 + edge.offset);
        endPoints[1] = left(settings, half - 1 + edge.offset);
        break;
      case Corner::TopRight:
        endPoints[0] = top(settings, half - edge.offset);
        endPoints[1] = right(settings, half - 1 + edge.offset);
        break;
      case Corner::BottomLeft:
        endPoints[0] = bottom(settings, half - 1 + edge.offset);
        endPoints[1] = left(settings, half - edge.offset);
        break;
      case Corner::BottomRight:
        endPoints[0] = bottom(settings, half - edge.offset);
        endPoints[1] = right(settings, half - edge.offset);
        break;
    }

    auto line = makeLine(settings, endPoints, random, edge.displacement);

    for (auto point : line) {
      tile.pixels(point) = b0;
    }

    switch (c) {
      case Corner::TopLeft:
        tile.pixels.fillFrom(cornerTopLeft(settings), b0);
        tile.pixels.fillFrom(cornerBottomRight(settings), b1);
        break;
      case Corner::TopRight:
        tile.pixels.fillFrom(cornerTopRight(settings), b0);
        tile.pixels.fillFrom(cornerBottomLeft(settings), b1);
        break;
      case Corner::BottomLeft:
        tile.pixels.fillFrom(cornerBottomLeft(settings), b0);
        tile.pixels.fillFrom(cornerTopRight(settings), b1);
        break;
      case Corner::BottomRight:
        tile.pixels.fillFrom(cornerBottomRight(settings), b0);
        tile.pixels.fillFrom(cornerTopLeft(settings), b1);
        break;
    }

    tile.terrain[TerrainTopLeft] = tile.terrain[TerrainTopRight] = tile.terrain[TerrainBottomLeft] = tile.terrain[TerrainBottomRight] = b1;

    switch (c) {
      case Corner::TopLeft:
        tile.terrain[TerrainTopLeft] = b0;
        break;
      case Corner::TopRight:
        tile.terrain[TerrainTopRight] = b0;
        break;
      case Corner::BottomLeft:
        tile.terrain[TerrainBottomLeft] = b0;
        break;
      case Corner::BottomRight:
        tile.terrain[TerrainBottomRight] = b0;
        break;
    }

    if (edge.limit) {
      tile.fences.count = 1;

      switch (c) {
        case Corner::TopLeft:
          tile.fences.segments[0].p0 = fenceTop(settings, half + edge.offset);
          tile.fences.segments[0].p1 = fenceLeft(settings, half + edge.offset);
          break;
        case Corner::TopRight:
          tile.fences.segments[0].p0 = fenceTop(settings, half - edge.offset);
          tile.fences.segments[0].p1 = fenceRight(settings, half + edge.offset);
          break;
        case Corner::BottomLeft:
          tile.fences.segments[0].p0 = fenceBottom(settings, half + edge.offset);
          tile.fences.segments[0].p1 = fenceLeft(settings, half - edge.offset);
          break;
        case Corner::BottomRight:
          tile.fences.segments[0].p0 = fenceBottom(settings, half - edge.offset);
          tile.fences.segments[0].p1 = fenceRight(settings, half - edge.offset);
          break;
      }
    }

    tile.checkHoles();
    return tile;
  }

  // b0 is in top-left and bottom-right, b1 is in top-right and bottom-left
  Tile generateCross(const TileSettings& settings, gf::Id b0, gf::Id b1, gf::Random& random, const Edge& edge) {
    Tile tile(settings);
    tile.origin = Origin(b0, b1);

    int half = settings.size / 2;

    gf::Vector2i limitTopRight[] = { top(settings, half + edge.offset), { half, half - 1 }, right(settings, half - 1 - edge.offset) };

    auto lineTopRight = makeLine(settings, limitTopRight, random, edge.displacement);

    for (auto point : lineTopRight) {
      tile.pixels(point) = b1;
    }

    gf::Vector2i limitBottomLeft[] = { bottom(settings, half - 1 - edge.offset), { half - 1, half }, left(settings, half + edge.offset) };

    auto lineBottomLeft = makeLine(settings, limitBottomLeft, random, edge.displacement);

    for (auto point : lineBottomLeft) {
      tile.pixels(point) = b1;
    }

    tile.pixels.fillFrom(cornerTopLeft(settings), b0);
    tile.pixels.fillFrom(cornerBottomRight(settings), b0);
    tile.pixels.fillFrom(cornerTopRight(settings), b1);
    tile.pixels.fillFrom(cornerBottomLeft(settings), b1);

    tile.terrain[TerrainTopLeft] = tile.terrain[TerrainBottomRight] = b0;
    tile.terrain[TerrainTopRight] = tile.terrain[TerrainBottomLeft] = b1;

    if (edge.limit) {
      tile.fences.count = 2;
      tile.fences.segments[0].p0 = fenceTop(settings, half + edge.offset);
      tile.fences.segments[0].p1 = fenceRight(settings, half - edge.offset);
      tile.fences.segments[1].p0 = fenceBottom(settings, half - edge.offset);
      tile.fences.segments[1].p1 = fenceLeft(settings, half + edge.offset);
    }

    tile.checkHoles();
    return tile;
  }

  /*
   * Three Corner Wang Tileset generators
   */

  bool checkEdges(const Edge& e01, const Edge& e12, const Edge& e20) {
    int count = 0;
    count += e01.limit ? 1 : 0;
    count += e12.limit ? 1 : 0;
    count += e20.limit ? 1 : 0;
    return count == 0 || count == 2;
  }

  // b0 is given by split, b1 is at the left, b2 is at the right
  Tile generateHorizontalSplit(const TileSettings& settings, gf::Id b0, gf::Id b1, gf::Id b2, HSplit split, gf::Random& random, const Edge& e01, const Edge& e12, const Edge& e20) {
    Tile tile(settings);
    tile.origin = Origin(b0, b1, b2);

    int half = settings.size / 2;

    gf::Vector2i p0, p1, p2, p3;

    if (split == HSplit::Top) {
      p0 = left(settings, half - 1 + e01.offset);
      p1 = right(settings, half - 1 - e20.offset);
      p2 = gf::vec(half, half -1 + (e01.offset - e20.offset) / 2);
      p3 = bottom(settings, half + e12.offset);
    } else {
      p0 = left(settings, half - e01.offset);
      p1 = right(settings, half + e20.offset);
      p2 = gf::vec(half, half + (e20.offset - e01.offset) / 2);
      p3 = top(settings, half + e12.offset);
    }

    gf::Vector2i segmentMiddle[] = { p2, p3 };
    auto lineMiddle = makeLine(settings, segmentMiddle, random, e12.displacement);

    for (auto point : lineMiddle) {
      tile.pixels(point) = b2;
    }

    gf::Vector2i segmentLeft[] = { p0, p2 };
    auto lineLeft = makeLine(settings, segmentLeft, random, e01.displacement);

    for (auto point : lineLeft) {
      tile.pixels(point) = b0;
    }

    gf::Vector2i segmentRight[] = { p1, p2 };
    auto lineRight = makeLine(settings, segmentRight, random, e20.displacement);

    for (auto point : lineRight) {
      tile.pixels(point) = b0;
    }

    if (split == HSplit::Top) {
      tile.pixels.fillFrom(top(settings, half), b0);
      tile.pixels.fillFrom(cornerBottomLeft(settings), b1);
      tile.pixels.fillFrom(cornerBottomRight(settings), b2);
    } else {
      tile.pixels.fillFrom(bottom(settings, half), b0);
      tile.pixels.fillFrom(cornerTopLeft(settings), b1);
      tile.pixels.fillFrom(cornerTopRight(settings), b2);
    }

    if (split == HSplit::Top) {
      tile.terrain[TerrainTopLeft] = tile.terrain[TerrainTopRight] = b0;
      tile.terrain[TerrainBottomLeft] = b1;
      tile.terrain[TerrainBottomRight] = b2;
    } else {
      tile.terrain[TerrainBottomLeft] = tile.terrain[TerrainBottomRight] = b0;
      tile.terrain[TerrainTopLeft] = b1;
      tile.terrain[TerrainTopRight] = b2;
    }

    if (checkEdges(e01, e12, e20)) {

      if (e01.limit && e12.limit) {
        if (split == HSplit::Top) {
          tile.fences.segments[tile.fences.count].p0 = fenceLeft(settings, half + e01.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceBottom(settings, half + e12.offset);
        } else {
          tile.fences.segments[tile.fences.count].p0 = fenceLeft(settings, half - e01.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceTop(settings, half + e12.offset);
        }

        ++tile.fences.count;
      }

      if (e12.limit && e20.limit) {
        if (split == HSplit::Top) {
          tile.fences.segments[tile.fences.count].p0 = fenceRight(settings, half - e20.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceBottom(settings, half + e12.offset);
        } else {
          tile.fences.segments[tile.fences.count].p0 = fenceRight(settings, half + e20.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceTop(settings, half + e12.offset);
        }

        ++tile.fences.count;
      }

      if (e20.limit && e01.limit) {
        if (split == HSplit::Top) {
          tile.fences.segments[tile.fences.count].p0 = fenceLeft(settings, half + e01.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceRight(settings, half - e20.offset);
        } else {
          tile.fences.segments[tile.fences.count].p0 = fenceLeft(settings, half - e01.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceRight(settings, half + e20.offset);
        }

        ++tile.fences.count;
      }

    }

    tile.checkHoles();
    return tile;
  }

  // b0 is given by split, b1 is at the top, b2 is at the bottom
  Tile generateVerticalSplit(const TileSettings& settings, gf::Id b0, gf::Id b1, gf::Id b2, VSplit split, gf::Random& random, const Edge& e01, const Edge& e12, const Edge& e20) {
    Tile tile(settings);
    tile.origin = Origin(b0, b1, b2);

    int half = settings.size / 2;

    gf::Vector2i p0, p1, p2, p3;

    if (split == VSplit::Left) {
      p0 = top(settings, half - 1 + e01.offset);
      p1 = bottom(settings, half - 1 - e20.offset);
      p2 = gf::vec(half - 1 + (e01.offset - e20.offset) / 2, half);
      p3 = right(settings, half + e12.offset);
    } else {
      p0 = top(settings, half - e01.offset);
      p1 = bottom(settings, half + e20.offset);
      p2 = gf::vec(half + (e20.offset - e01.offset) / 2, half);
      p3 = left(settings, half + e12.offset);
    }

    gf::Vector2i segmentMiddle[] = { p2, p3 };
    auto lineMiddle = makeLine(settings, segmentMiddle, random, e12.displacement);

    for (auto point : lineMiddle) {
      tile.pixels(point) = b2;
    }

    gf::Vector2i segmentTop[] = { p0, p2 };
    auto lineTop = makeLine(settings, segmentTop, random, e01.displacement);

    for (auto point : lineTop) {
      tile.pixels(point) = b0;
    }

    gf::Vector2i segmentBottom[] = { p1, p2 };
    auto lineBottom = makeLine(settings, segmentBottom, random, e20.displacement);

    for (auto point : lineBottom) {
      tile.pixels(point) = b0;
    }

    if (split == VSplit::Left) {
      tile.pixels.fillFrom(left(settings, half), b0);
      tile.pixels.fillFrom(cornerTopRight(settings), b1);
      tile.pixels.fillFrom(cornerBottomRight(settings), b2);
    } else {
      tile.pixels.fillFrom(right(settings, half), b0);
      tile.pixels.fillFrom(cornerTopLeft(settings), b1);
      tile.pixels.fillFrom(cornerBottomLeft(settings), b2);
    }

    if (split == VSplit::Left) {
      tile.terrain[TerrainTopLeft] = tile.terrain[TerrainBottomLeft] = b0;
      tile.terrain[TerrainTopRight] = b1;
      tile.terrain[TerrainBottomRight] = b2;
    } else {
      tile.terrain[TerrainTopRight] = tile.terrain[TerrainBottomRight] = b0;
      tile.terrain[TerrainTopLeft] = b1;
      tile.terrain[TerrainBottomLeft] = b2;
    }

    if (checkEdges(e01, e12, e20)) {

      if (e01.limit && e12.limit) {
        if (split == VSplit::Left) {
          tile.fences.segments[tile.fences.count].p0 = fenceTop(settings, half + e01.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceRight(settings, half + e12.offset);
        } else {
          tile.fences.segments[tile.fences.count].p0 = fenceTop(settings, half - e01.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceLeft(settings, half + e12.offset);
        }

        ++tile.fences.count;
      }

      if (e12.limit && e20.limit) {
        if (split == VSplit::Left) {
          tile.fences.segments[tile.fences.count].p0 = fenceBottom(settings, half - e20.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceRight(settings, half + e12.offset);
        } else {
          tile.fences.segments[tile.fences.count].p0 = fenceBottom(settings, half + e20.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceLeft(settings, half + e12.offset);
        }

        ++tile.fences.count;
      }

      if (e20.limit && e01.limit) {
        if (split == VSplit::Left) {
          tile.fences.segments[tile.fences.count].p0 = fenceTop(settings, half + e01.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceBottom(settings, half - e20.offset);
        } else {
          tile.fences.segments[tile.fences.count].p0 = fenceTop(settings, half - e01.offset);
          tile.fences.segments[tile.fences.count].p1 = fenceBottom(settings, half + e20.offset);
        }

        ++tile.fences.count;
      }

    }

    tile.checkHoles();
    return tile;
  }

  // b0 is given by oblique, b1 is at the left and b2 is at the right
  Tile generateOblique(const TileSettings& settings, gf::Id b0, gf::Id b1, gf::Id b2, Oblique oblique, gf::Random& random, const Edge& e01, const Edge& e20) {
    Tile tile(settings);
    tile.origin = Origin(b0, b1, b2);

    int half = settings.size / 2;

    gf::Vector2i p0, p1, p2, p3;

    if (oblique == Oblique::Up) {
      p0 = left(settings, half - e01.offset);
      p1 = top(settings, half - e01.offset);
      p2 = right(settings, half - 1 - e20.offset);
      p3 = bottom(settings, half - 1 - e20.offset);
    } else {
      p0 = left(settings, half - 1 + e01.offset);
      p1 = bottom(settings, half - 1 - e01.offset);
      p2 = right(settings, half + e20.offset);
      p3 = top(settings, half - e20.offset);
    }

    gf::Vector2i segmentLeft[] = { p0, p1 };
    auto lineLeft = makeLine(settings, segmentLeft, random, e01.displacement);

    for (auto point : lineLeft) {
      tile.pixels(point) = b0;
    }

    gf::Vector2i segmentRight[] = { p2, p3 };
    auto lineRight = makeLine(settings, segmentRight, random, e20.displacement);

    for (auto point : lineRight) {
      tile.pixels(point) = b0;
    }

    if (oblique == Oblique::Up) {
      tile.pixels.fillFrom(cornerBottomLeft(settings), b0);
      tile.pixels.fillFrom(cornerTopLeft(settings), b1);
      tile.pixels.fillFrom(cornerBottomRight(settings), b2);
    } else {
      tile.pixels.fillFrom(cornerTopLeft(settings), b0);
      tile.pixels.fillFrom(cornerBottomLeft(settings), b1);
      tile.pixels.fillFrom(cornerTopRight(settings), b2);
    }

    if (oblique == Oblique::Up) {
      tile.terrain[TerrainBottomLeft] = tile.terrain[TerrainTopRight] = b0;
      tile.terrain[TerrainTopLeft] = b1;
      tile.terrain[TerrainBottomRight] = b2;
    } else {
      tile.terrain[TerrainTopLeft] = tile.terrain[TerrainBottomRight] = b0;
      tile.terrain[TerrainBottomLeft] = b1;
      tile.terrain[TerrainTopRight] = b2;
    }

    if (e01.limit) {
      if (oblique == Oblique::Up) {
        tile.fences.segments[tile.fences.count].p0 = fenceLeft(settings, half - e01.offset);
        tile.fences.segments[tile.fences.count].p1 = fenceTop(settings, half - e01.offset);
      } else {
        tile.fences.segments[tile.fences.count].p0 = fenceLeft(settings, half + e01.offset);
        tile.fences.segments[tile.fences.count].p1 = fenceBottom(settings, half - e01.offset);
      }

      ++tile.fences.count;
    }

    if (e20.limit) {
      if (oblique == Oblique::Up) {
        tile.fences.segments[tile.fences.count].p0 = fenceRight(settings, half - e20.offset);
        tile.fences.segments[tile.fences.count].p1 = fenceBottom(settings, half - e20.offset);
      } else {
        tile.fences.segments[tile.fences.count].p0 = fenceRight(settings, half + e20.offset);
        tile.fences.segments[tile.fences.count].p1 = fenceTop(settings, half - e20.offset);
      }

      ++tile.fences.count;
    }

    tile.checkHoles();
    return tile;
  }

  /*
   * Plain
   */

  Tileset generatePlainTileset(gf::Id b0, const TilesetData& db) {
    Tileset tileset({ AtomsTilesetSize, AtomsTilesetSize });

    for (int i = 0; i < AtomsTilesetSize; ++i) {
      for (int j = 0; j < AtomsTilesetSize; ++j) {
        tileset({ i, j }) = generateFull(db.settings.tile, b0);
      }
    }

    return tileset;
  }

  /*
   *    0    1    2    3
   *  0 +----+----+----+----+
   *    |    |  ##|##  |    |
   *    |##  |  ##|####|####|
   *  1 +----+----+----+----+
   *    |##  |  ##|####|####|
   *    |  ##|####|####|##  |
   *  2 +----+----+----+----+
   *    |  ##|####|####|##  |
   *    |    |    |  ##|##  |
   *  3 +----+----+----+----+
   *    |    |    |  ##|##  |
   *    |    |  ##|##  |    |
   *    +----+----+----+----+
   *
   *    b0 = ' '
   *    b1 = '#'
   */

  Tileset generateTwoCornersWangTileset(const Wang2& wang, gf::Random& random, const TilesetData& db) {
    Tileset tileset({ Wang2TilesetSize, Wang2TilesetSize });

    auto b0 = wang.borders[0].id.hash;
    auto b1 = wang.borders[1].id.hash;
    auto edge = wang.edge;
    auto& settings = db.settings.tile;

    tileset({ 0, 0 }) = generateCorner(settings, b1, b0, Corner::BottomLeft, random, edge.invert());
    tileset({ 0, 1 }) = generateCross(settings, b1, b0, random, edge.invert());
    tileset({ 0, 2 }) = generateCorner(settings, b1, b0, Corner::TopRight, random, edge.invert());
    tileset({ 0, 3 }) = generateFull(settings, b0);

    tileset({ 1, 0 }) = generateSplit(settings, b0, b1, Split::Vertical, random, edge);
    tileset({ 1, 1 }) = generateCorner(settings, b0, b1, Corner::TopLeft, random, edge);
    tileset({ 1, 2 }) = generateSplit(settings, b1, b0, Split::Horizontal, random, edge.invert());
    tileset({ 1, 3 }) = generateCorner(settings, b1, b0, Corner::BottomRight, random, edge.invert());

    tileset({ 2, 0 }) = generateCorner(settings, b0, b1, Corner::TopRight, random, edge);
    tileset({ 2, 1 }) = generateFull(settings, b1);
    tileset({ 2, 2 }) = generateCorner(settings, b0, b1, Corner::BottomLeft, random, edge);
    tileset({ 2, 3 }) = generateCross(settings, b0, b1, random, edge);

    tileset({ 3, 0 }) = generateSplit(settings, b0, b1, Split::Horizontal, random, edge);
    tileset({ 3, 1 }) = generateCorner(settings, b0, b1, Corner::BottomRight, random, edge);
    tileset({ 3, 2 }) = generateSplit(settings, b1, b0, Split::Vertical, random, edge.invert());
    tileset({ 3, 3 }) = generateCorner(settings, b1, b0, Corner::TopLeft, random, edge.invert());

    return tileset;
  }

  /*
   *   0    1    2    3    4    5
   * 0 +----+----+----+----+----+----+
   *   |####|####|##::|::::|::  |  ##|
   *   |::  |  ::|::  |  ##|##  |  ::|
   * 1 +----+----+----+----+----+----+
   *   |::  |  ::|::  |  ##|##  |  ::|
   *   |::##|##  |  ##|##::|::##|##::|
   * 2 +----+----+----+----+----+----+
   *   |::##|##  |  ##|##::|::##|##::|
   *   |  ::|::::|::##|##  |    |    |
   * 3 +----+----+----+----+----+----+
   *   |  ::|::::|::##|##  |    |    |
   *   |::##|##  |  ##|##::|::##|##::|
   * 4 +----+----+----+----+----+----+
   *   |::##|##  |  ##|##::|::##|##::|
   *   |::  |  ::|::  |  ##|##  |  ::|
   * 5 +----+----+----+----+----+----+
   *   |::  |  ::|::  |  ##|##  |  ::|
   *   |####|####|##::|::::|::  |  ##|
   *   +----+----+----+----+----+----+
   *
   *   This is the layout that is valid and minimizes the number of areas.
   *   b0 = ' '
   *   b1 = ':'
   *   b2 = '#'
   */

  Tileset generateThreeCornersWangTileset(const Wang3& wang, gf::Random& random, const TilesetData& db) {
    Tileset tileset({ Wang3TilesetSize, Wang3TilesetSize });

    gf::Id b0 = wang.ids[0].hash;
    gf::Id b1 = wang.ids[1].hash;
    gf::Id b2 = wang.ids[2].hash;

    Edge edge01 = db.getEdge(b0, b1);
    Edge edge12 = db.getEdge(b1, b2);
    Edge edge20 = db.getEdge(b2, b0);

    tileset({ 0, 0 }) = generateHorizontalSplit(db.settings.tile, b2, b1, b0, HSplit::Top, random, edge12.invert(), edge01.invert(), edge20.invert());
    tileset({ 0, 1 }) = generateVerticalSplit(db.settings.tile, b1, b0, b2, VSplit::Left, random, edge01.invert(), edge20.invert(), edge12.invert());
    tileset({ 0, 2 }) = generateOblique(db.settings.tile, b1, b0, b2, Oblique::Down, random, edge01.invert(), edge12.invert());
    tileset({ 0, 3 }) = generateOblique(db.settings.tile, b1, b0, b2, Oblique::Up, random, edge01.invert(), edge12.invert());
    tileset({ 0, 4 }) = generateVerticalSplit(db.settings.tile, b1, b2, b0, VSplit::Left, random, edge12, edge20, edge01);
    tileset({ 0, 5 }) = generateHorizontalSplit(db.settings.tile, b2, b1, b0, HSplit::Bottom, random, edge12.invert(), edge01.invert(), edge20.invert());

    tileset({ 1, 0 }) = generateHorizontalSplit(db.settings.tile, b2, b0, b1, HSplit::Top, random, edge20, edge01, edge12);
    tileset({ 1, 1 }) = generateOblique(db.settings.tile, b0, b2, b1, Oblique::Down, random, edge20.invert(), edge01.invert());
    tileset({ 1, 2 }) = generateHorizontalSplit(db.settings.tile, b1, b2, b0, HSplit::Bottom, random, edge12, edge20, edge01);
    tileset({ 1, 3 }) = generateHorizontalSplit(db.settings.tile, b1, b2, b0, HSplit::Top, random, edge12, edge20, edge01);
    tileset({ 1, 4 }) = generateOblique(db.settings.tile, b0, b2, b1, Oblique::Up, random, edge20.invert(), edge01.invert());
    tileset({ 1, 5 }) = generateHorizontalSplit(db.settings.tile, b2, b0, b1, HSplit::Bottom, random, edge20, edge01, edge12);

    tileset({ 2, 0 }) = generateOblique(db.settings.tile, b1, b2, b0, Oblique::Up, random, edge12, edge01);
    tileset({ 2, 1 }) = generateOblique(db.settings.tile, b0, b1, b2, Oblique::Up, random, edge01, edge20);
    tileset({ 2, 2 }) = generateVerticalSplit(db.settings.tile, b2, b0, b1, VSplit::Right, random, edge20, edge01, edge12);
    tileset({ 2, 3 }) = generateVerticalSplit(db.settings.tile, b2, b1, b0, VSplit::Right, random, edge12.invert(), edge01.invert(), edge20.invert());
    tileset({ 2, 4 }) = generateOblique(db.settings.tile, b0, b1, b2, Oblique::Down, random, edge01, edge20);
    tileset({ 2, 5 }) = generateOblique(db.settings.tile, b1, b2, b0, Oblique::Down, random, edge12, edge01);

    tileset({ 3, 0 }) = generateHorizontalSplit(db.settings.tile, b1, b0, b2, HSplit::Top, random, edge01.invert(), edge20.invert(), edge12.invert());
    tileset({ 3, 1 }) = generateOblique(db.settings.tile, b2, b0, b1, Oblique::Up, random, edge20, edge12);
    tileset({ 3, 2 }) = generateVerticalSplit(db.settings.tile, b2, b1, b0, VSplit::Left, random, edge12.invert(), edge01.invert(), edge20.invert());
    tileset({ 3, 3 }) = generateVerticalSplit(db.settings.tile, b2, b0, b1, VSplit::Left, random, edge20, edge01, edge12);
    tileset({ 3, 4 }) = generateOblique(db.settings.tile, b2, b0, b1, Oblique::Down, random, edge20, edge12);
    tileset({ 3, 5 }) = generateHorizontalSplit(db.settings.tile, b1, b0, b2, HSplit::Bottom, random, edge01.invert(), edge20.invert(), edge12.invert());

    tileset({ 4, 0 }) = generateVerticalSplit(db.settings.tile, b0, b1, b2, VSplit::Right, random, edge01, edge12, edge20);
    tileset({ 4, 1 }) = generateOblique(db.settings.tile, b2, b1, b0, Oblique::Down, random, edge12.invert(), edge20.invert());
    tileset({ 4, 2 }) = generateHorizontalSplit(db.settings.tile, b0, b1, b2, HSplit::Bottom, random, edge01, edge12, edge20);
    tileset({ 4, 3 }) = generateHorizontalSplit(db.settings.tile, b0, b1, b2, HSplit::Top, random, edge01, edge12, edge20);
    tileset({ 4, 4 }) = generateOblique(db.settings.tile, b2, b1, b0, Oblique::Up, random, edge12.invert(), edge20.invert());
    tileset({ 4, 5 }) = generateVerticalSplit(db.settings.tile, b0, b2, b1, VSplit::Right, random, edge20.invert(), edge12.invert(), edge01.invert());

    tileset({ 5, 0 }) = generateVerticalSplit(db.settings.tile, b0, b2, b1, VSplit::Left, random, edge20.invert(), edge12.invert(), edge01.invert());
    tileset({ 5, 1 }) = generateVerticalSplit(db.settings.tile, b1, b0, b2, VSplit::Right, random, edge01.invert(), edge20.invert(), edge12.invert());
    tileset({ 5, 2 }) = generateHorizontalSplit(db.settings.tile, b0, b2, b1, HSplit::Bottom, random, edge20.invert(), edge12.invert(), edge01.invert());
    tileset({ 5, 3 }) = generateHorizontalSplit(db.settings.tile, b0, b2, b1, HSplit::Top, random, edge20.invert(), edge12.invert(), edge01.invert());
    tileset({ 5, 4 }) = generateVerticalSplit(db.settings.tile, b1, b2, b0, VSplit::Right, random, edge12, edge20, edge01);
    tileset({ 5, 5 }) = generateVerticalSplit(db.settings.tile, b0, b1, b2, VSplit::Left, random, edge01, edge12, edge20);

    return tileset;
  }

}
