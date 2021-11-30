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
#include "TilesetProcess.h"

#include <cinttypes>
#include <algorithm>
#include <sstream>

#include <gf/Color.h>
#include <gf/Geometry.h>
#include <gf/Ref.h>
#include <gf/VectorOps.h>

#include <gf/Log.h>

namespace gftools {

  /*
   * Colors
   */

  Colors::Colors(gf::Vector2i size)
  : data(size, gf::Color::Transparent)
  {
  }

  Colors Colors::extend(int space) const {
    auto size = data.getSize();
    Colors result(size + 2 * space);

    for (auto pos : result.data.getPositionRange()) {
      auto sourcePos = gf::clamp(pos - space, gf::vec(0, 0), size - 1);
      result.data(pos) = data(sourcePos);
    }

    return result;
  }

  void Colors::blit(const Colors source, gf::Vector2i offset) {
    auto srcSize = source.data.getSize();
    auto dstSize = data.getSize();

    assert(offset.x >= 0);
    assert(offset.y >= 0);
    assert(offset.x + srcSize.width <= dstSize.width);
    assert(offset.y + srcSize.height <= dstSize.height);

    for (auto pos : source.data.getPositionRange()) {
      data(offset + pos) = source.data(pos);
    }
  }

  gf::Image Colors::createImage() const {
    gf::Image image(data.getSize(), gf::Color::toRgba32(gf::Color::Transparent));

    for (auto pos : data.getPositionRange()) {
      gf::Color4f raw = data(pos);
      gf::Color4u color = gf::Color::toRgba32(raw);
      image.setPixel(pos, color);
    }

    return image;
  }

  namespace {

    void colorizeAtom(Colors& colors, const Atom& atom, const Tile& tile, gf::Random& random) {
      if (atom.id.hash == Void) {
        return;
      }

      switch (atom.pigment.style) {
        case PigmentStyle::Plain:
          for (auto pos : tile.pixels.data.getPositionRange()) {
            if (tile.pixels(pos) != atom.id.hash) {
              continue;
            }

            colors(pos) = atom.color;
          }
          break;

        case PigmentStyle::Randomize: {
          for (auto pos : tile.pixels.data.getPositionRange()) {
            if (tile.pixels(pos) != atom.id.hash) {
              continue;
            }

            colors(pos) = atom.color;
          }

          auto size = tile.pixels.data.getSize();
          int anomalies = atom.pigment.randomize.ratio * size.width * size.height / gf::square(atom.pigment.randomize.size) + 1;

          for (int i = 0; i < anomalies; ++i) {
            gf::Vector2i pos = random.computePosition(gf::RectI::fromSize(size - atom.pigment.randomize.size));

            gf::Id id = tile.pixels(pos);

            if (id != atom.id.hash) {
              continue;
            }

            float change = gf::clamp(random.computeNormalFloat(0.0f, atom.pigment.randomize.deviation), -0.5f, 0.5f);
            auto modified = (change > 0) ? gf::Color::darker(atom.color, change) : gf::Color::lighter(atom.color, -change);

            gf::Vector2i offset;

            for (offset.y = 0; offset.y < atom.pigment.randomize.size; ++offset.y) {
              for (offset.x = 0; offset.x < atom.pigment.randomize.size; ++offset.x) {
                auto neighbor = pos + offset;
                assert(tile.pixels.data.isValid(neighbor));

                if (tile.pixels(neighbor) == id) {
                  colors(neighbor) = modified;
                }
              }
            }
          }

          break;
        }

        case PigmentStyle::Striped:
          for (auto pos : tile.pixels.data.getPositionRange()) {
            if (tile.pixels(pos) != atom.id.hash) {
              continue;
            }

            if ((pos.x + pos.y ) % atom.pigment.striped.stride < atom.pigment.striped.width) {
              colors(pos) = atom.color;
            } else {
              colors(pos) = atom.color * gf::Color::Opaque(0.0f);
            }
          }
          break;

        case PigmentStyle::Paved: {
          auto modulateColor = [&atom]() {
            if (atom.pigment.paved.modulation < 0.0f) {
              return gf::Color::lighter(atom.color, -atom.pigment.paved.modulation);
            }

            return gf::Color::darker(atom.color, atom.pigment.paved.modulation);
          };

          for (auto pos : tile.pixels.data.getPositionRange()) {
            if (tile.pixels(pos) != atom.id.hash) {
              continue;
            }

            colors(pos) = atom.color;

            int y = pos.y + atom.pigment.paved.width / 2;

            if (y % atom.pigment.paved.width == 0) {
              colors(pos) = modulateColor();
            } else {
              int x = pos.x + atom.pigment.paved.length / 4;

              if (y / atom.pigment.paved.width % 2 == 0) {
                if (x % atom.pigment.paved.length == 0) {
                  colors(pos) = modulateColor();
                }
              } else {
                if (x % atom.pigment.paved.length == atom.pigment.paved.length / 2) {
                  colors(pos) = modulateColor();
                }
              }
            }
          }
          break;
        }
      }
    }


    void colorizeBorder(Colors& colors, const Colors& originalColors, const Wang2& wang, const Tile& tile, gf::Random& random, const TilesetData& db) {
      for (int i = 0; i < 2; ++i) {
        auto& border = wang.borders[i];

        gf::Id id = border.id.hash;

        if (id == Void) {
          continue;
        }

        if (border.effect == BorderEffect::None) {
          continue;
        }

        Atom atom = db.getAtom(id);

        gf::Id other = wang.borders[1 - i].id.hash;

        for (auto pos : tile.pixels.data.getPositionRange()) {
          if (tile.pixels(pos) != id) {
            continue;
          }

          int minDistance = 1'000'000;
          gf::Vector2i minNeighbor(-1, -1);

          for (auto neighbor : tile.pixels.data.getPositionRange()) {
            if (tile.pixels(neighbor) != other) {
              continue;
            }

            int distance = gf::manhattanDistance(pos, neighbor);

            if (distance < minDistance) {
              minDistance = distance;
              minNeighbor = neighbor;
            }
          }

          auto color = originalColors(pos);
          bool changed = false;

          switch (border.effect) {
            case BorderEffect::Fade:
              if (minDistance <= border.fade.distance) {
                changed = true;
                color.a = gf::lerp(color.a, 0.0f, (border.fade.distance - minDistance) / static_cast<float>(border.fade.distance));
              }
              break;

            case BorderEffect::Outline:
              if (minDistance <= border.outline.distance) {
                changed = true;
                color = gf::Color::darker(atom.color, border.outline.factor);
              }
              break;

            case BorderEffect::Sharpen:
              if (minDistance <= border.sharpen.distance) {
                changed = true;
                color = gf::Color::darker(color, (border.sharpen.distance - minDistance) * 0.5f / border.sharpen.distance);
              }
              break;

            case BorderEffect::Lighten:
              if (minDistance <= border.sharpen.distance) {
                changed = true;
                color = gf::Color::lighter(color, (border.lighten.distance - minDistance) * 0.5f / border.lighten.distance);
              }
              break;

            case BorderEffect::Blur:
              if (minDistance < 5) {
                changed = true;

                // see https://en.wikipedia.org/wiki/Kernel_(image_processing)

                float finalCoeff = 36.0f;
                gf::Color4f finalColor = 36.0f * originalColors(pos);

                for (auto next : originalColors.data.get24NeighborsRange(pos)) {
                  gf::Color4f nextColor = originalColors(next);
                  gf::Vector2i diff = gf::abs(pos - next);

                  if (diff == gf::Vector2i(1, 0) || diff == gf::Vector2i(0, 1)) {
                    finalColor += 24.0f * nextColor;
                    finalCoeff += 24.0f;
                  } else if (diff == gf::Vector2i(1, 1)) {
                    finalColor += 16.0f * nextColor;
                    finalCoeff += 16.0f;
                  } else if (diff == gf::Vector2i(2, 0) || diff == gf::Vector2i(0, 2)) {
                    finalColor += 6.0f * nextColor;
                    finalCoeff += 6.0f;
                  } else if (diff == gf::Vector2i(2, 1) || diff == gf::Vector2i(1, 2)) {
                    finalColor += 4.0f * nextColor;
                    finalCoeff += 4.0f;
                  } else if (diff == gf::Vector2i(2, 2)) {
                    finalColor += 1.0f * nextColor;
                    finalCoeff += 1.0f;
                  } else {
                    assert(false);
                  }
                }

                color = finalColor / finalCoeff;
              }
              break;

            case BorderEffect::Blend:
              if (minDistance <= border.blend.distance) {
                float stop = 1.0f;

                if (wang.borders[1 - i].effect == BorderEffect::Blend) {
                  stop = 0.5f;
                }

                changed = true;
                color = gf::lerp(color, originalColors(minNeighbor), stop * (border.blend.distance - minDistance) / static_cast<float>(border.blend.distance) + random.computeUniformFloat(0.0f, 0.05f));
              }
              break;

            default:
              assert(false);
              break;
          }

          if (changed) {
            colors(pos) = color;
          }
        }

      }
    }


    Colors colorizeRawTile(const Tile& tile, gf::Random& random, const TilesetData& db, Search search) {
      Colors colors(tile.pixels.data.getSize());
      auto& origin = tile.origin;

      // first pass: base biome color

      for (auto biome : origin.ids) {
        if (biome == Void || biome == gf::InvalidId) {
          continue;
        }

        auto atom = db.getAtom(biome, search);
        colorizeAtom(colors, atom, tile, random);
      }

      // second pass: borders

      Colors original(colors);

      if (origin.count == 2) {
        colorizeBorder(colors, original, db.getWang2(origin.ids[0], origin.ids[1], search), tile, random, db);
      } else if (origin.count == 3) {
        colorizeBorder(colors, original, db.getWang2(origin.ids[0], origin.ids[1], search), tile, random, db);
        colorizeBorder(colors, original, db.getWang2(origin.ids[1], origin.ids[2], search), tile, random, db);
        colorizeBorder(colors, original, db.getWang2(origin.ids[2], origin.ids[0], search), tile, random, db);
      } else {
        assert(origin.count == 1);
      }

      return colors;
    }

  }

  Colors colorizeTile(const Tile& tile, gf::Random& random, const TilesetData& db) {
    return colorizeRawTile(tile, random, db, Search::UseDatabaseOnly).extend(db.settings.tile.spacing);
  }

  gf::Image generateAtomPreview(const Atom& atom, gf::Random& random, const TileSettings& settings) {
    Tile tile = generateFull(settings, atom.id.hash);
    Colors colors(tile.pixels.data.getSize());
    colorizeAtom(colors, atom, tile, random);
    return colors.createImage();
  }

  gf::Image generateWang2Preview(const Wang2& wang, gf::Random& random, const TilesetData& db) {
    Tileset tileset = generateTwoCornersWangTileset(wang, random, db);
    Colors colors(tileset.tiles.getSize() * (db.settings.tile.getTileSize() + 1) - 1);

    for (auto pos : tileset.tiles.getPositionRange()) {
      Colors tileColors = colorizeRawTile(tileset(pos), random, db, Search::IncludeTemporary);
      gf::Vector2i offset = pos * (db.settings.tile.getTileSize() + 1);
      colors.blit(tileColors, offset);
    }

    return colors.createImage();
  }

  gf::Image generateWang3Preview(const Wang3& wang, gf::Random& random, const TilesetData& db) {
    Tileset tileset = generateThreeCornersWangTileset(wang, random, db);
    Colors colors(tileset.tiles.getSize() * (db.settings.tile.getTileSize() + 1) - 1);

    for (auto pos : tileset.tiles.getPositionRange()) {
      Colors tileColors = colorizeRawTile(tileset(pos), random, db, Search::UseDatabaseOnly);
      gf::Vector2i offset = pos * (db.settings.tile.getTileSize() + 1);
      colors.blit(tileColors, offset);
    }

    return colors.createImage();
  }

  /*
   * DecoratedTileset
   */

  gf::Vector2i DecoratedTileset::findTerrainPosition(gf::Id id) const {
    for (auto& tileset : atoms) {
      for (auto tilePosition : tileset.tiles.getPositionRange()) {
        auto& tile = tileset(tilePosition);

        if (tile.origin.count == 1 && tile.origin.ids[0] == id) {
          return tileset.position + tilePosition;
        }
      }
    }

    gf::Log::error("Could not find a terrain for %" PRIx64 "\n", id);
    return gf::vec(-1, -1);
  }


  DecoratedTileset generateTilesets(gf::Random& random, const TilesetData& db) {
    DecoratedTileset tilesets;

    auto features = db.settings.getImageFeatures();

    gf::Vector2i globalPosition(0, 0);
    gf::Vector2i tilesetPosition(0, 0);

    for (auto& atom : db.atoms) {
      auto tileset = generatePlainTileset(atom.id.hash, db);
      tileset.position = globalPosition + tilesetPosition * AtomsTilesetSize;
      tilesets.atoms.push_back(std::move(tileset));

      ++tilesetPosition.x;

      if (tilesetPosition.x == features.atomsPerLine) {
        tilesetPosition.x = 0;
        ++tilesetPosition.y;
      }
    }

    globalPosition.y += features.atomsLineCount * AtomsTilesetSize;

    // wang2

    tilesetPosition = gf::vec(0, 0);

    for (auto& wang : db.wang2) {
      auto tileset = generateTwoCornersWangTileset(wang, random, db);
      tileset.position = globalPosition + tilesetPosition * Wang2TilesetSize;
      tilesets.wang2.push_back(std::move(tileset));

      ++tilesetPosition.x;

      if (tilesetPosition.x == features.wang2PerLine) {
        tilesetPosition.x = 0;
        ++tilesetPosition.y;
      }
    }

    globalPosition.y += features.wang2LineCount * Wang2TilesetSize;

    // wang3

    tilesetPosition = gf::vec(0, 0);

    for (auto& wang : db.wang3) {
      auto tileset = generateThreeCornersWangTileset(wang, random, db);
      tileset.position = globalPosition + tilesetPosition * Wang3TilesetSize;
      tilesets.wang3.push_back(std::move(tileset));

      ++tilesetPosition.x;

      if (tilesetPosition.x == features.wang3PerLine) {
        tilesetPosition.x = 0;
        ++tilesetPosition.y;
      }
    }

    return tilesets;
  }


  gf::Image generateTilesetImage(gf::Random& random, const TilesetData& db, const DecoratedTileset& tilesets) {
    auto features = db.settings.getImageFeatures();
    Colors mainColors(features.size);

    for (auto container : { gf::ref(tilesets.atoms), gf::ref(tilesets.wang2), gf::ref(tilesets.wang3) }) {
      for (auto& tileset : container.get()) {
        for (auto tilePosition : tileset.tiles.getPositionRange()) {
          Colors tileColors = colorizeTile(tileset(tilePosition), random, db);
          mainColors.blit(tileColors, (tileset.position + tilePosition) * db.settings.tile.getExtendedTileSize());
        }
      }
    }

    return mainColors.createImage();
  }


  namespace {

    template<typename T>
    struct KV {
      const char *key;
      T value;
    };

    template<typename T>
    KV<T> kv(const char * key, T value) {
      return { key, value };
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const KV<T>& kv) {
      return os << kv.key << '=' << '"' << kv.value << '"';
    }

  }

  std::string generateTilesetXml(const gf::Path& image, const TilesetData& db, const DecoratedTileset& tilesets) {
    std::map<gf::Id, std::size_t> mapping;

    for (std::size_t i = 0; i < db.atoms.size(); ++i) {
      mapping.emplace(db.atoms[i].id.hash, i);
    }

    auto getTerrainIndex = [&mapping](gf::Id id) {
      auto it = mapping.find(id);

      if (it != mapping.end()) {
        return std::to_string(it->second);
      }

      return std::string();
    };

    auto features = db.settings.getImageFeatures();

    gf::Vector2i tileCount = features.size / db.settings.tile.getExtendedTileSize();

    auto positionToIndex = [tileCount](gf::Vector2i position) {
      return position.y * tileCount.width + position.x;
    };

    std::ostringstream os;

    os << "<?xml " << kv("version", "1.0") << ' ' << kv("encoding", "UTF-8") << "?>\n";
    os << "<tileset " << kv("name", image.stem().string()) << ' '
        << kv("tilewidth", db.settings.tile.size) << ' ' << kv("tileheight", db.settings.tile.size) << ' '
        << kv("tilecount", tileCount.width * tileCount.height) << ' ' << kv("columns", tileCount.width) << ' '
        << kv("spacing", db.settings.tile.spacing * 2) << ' ' << kv("margin", db.settings.tile.spacing)
        << ">\n";

    os << "<image " << kv("source", image.string()) << ' '
        << kv("width", features.size.width) << ' ' << kv("height", features.size.height)
        << "/>\n";

    os << "<terraintypes>\n";

    int index = 0;

    for (auto& atom : db.atoms) {
      os << "\t<terrain " << kv("name", atom.id.name) << ' ' << kv("tile", positionToIndex(tilesets.findTerrainPosition(atom.id.hash))) << "/>\n";
      ++index;
    }

    os << "</terraintypes>\n";

    for (auto container : { gf::ref(tilesets.atoms), gf::ref(tilesets.wang2), gf::ref(tilesets.wang3) }) {
      for (auto& tileset : container.get()) {
        for (auto tilePosition : tileset.tiles.getPositionRange()) {
          auto& tile = tileset(tilePosition);

          os << "<tile id=\"" << positionToIndex(tileset.position + tilePosition) << "\" terrain=\""
            << getTerrainIndex(tile.terrain[0]) << ','
            << getTerrainIndex(tile.terrain[1]) << ','
            << getTerrainIndex(tile.terrain[2]) << ','
            << getTerrainIndex(tile.terrain[3]) << "\"";

          if (tile.fences.count > 0) {
            os << ">\n";
            os << "\t<properties>\n";
            os << "\t\t<property " << kv("name", "fence_count") << ' ' << kv("value", tile.fences.count) << ' ' << kv("type", "int") << "/>\n";

            char name[] = "fence#";

            for (int i = 0; i < tile.fences.count; ++i) {
              static constexpr char Sep = ',';

              name[5] = '0' + i;
              os << "\t\t<property name=\"" << name << "\" value=\""
                << tile.fences.segments[i].p0.x << Sep
                << tile.fences.segments[i].p0.y << Sep
                << tile.fences.segments[i].p1.x << Sep
                << tile.fences.segments[i].p1.y << "\" />\n";
            }

            os << "\t</properties>\n";
            os << "</tile>\n";
          } else {
            os << "/>\n";
          }

        }
      }
    }

    os << "</tileset>\n";

    return os.str();
  }

}
