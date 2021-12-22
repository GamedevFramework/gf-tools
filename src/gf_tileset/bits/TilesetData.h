#ifndef TILESET_DATA_H
#define TILESET_DATA_H

#include <vector>
#include <string>

#include <gf/Id.h>
#include <gf/Path.h>
#include <gf/Random.h>
#include <gf/Vector.h>

using namespace gf::literals;

namespace gftools {

  constexpr gf::Id Void = "Void"_id;

  struct TileSettings {
    int size = 32;
    int spacing = 1;

    gf::Vector2i getTileSize() const {
      return { size, size };
    }

    int getExtendedSize() const {
      return size + 2 * spacing;
    }

    gf::Vector2i getExtendedTileSize() const {
      return { getExtendedSize(), getExtendedSize() };
    }
  };

  struct ImageFeatures {
    gf::Vector2i size;
    int atomsPerLine;
    int atomsLineCount;
    int wang2PerLine;
    int wang2LineCount;
    int wang3PerLine;
    int wang3LineCount;
  };

  struct Settings {
    bool locked = false;
    int maxAtomCount = 64;
    int maxWang2Count = 48;
    int maxWang3Count = 32;
    TileSettings tile;

    gf::Vector2i getImageSize() const;
    ImageFeatures getImageFeatures() const;
  };

  enum class PigmentStyle {
    Plain,
    Randomize,
    Striped,
    Paved,
  };

  struct Pigment {
    PigmentStyle style = PigmentStyle::Plain;

    union {
      struct {
      } plain;
      struct {
        float ratio;
        float deviation;
        int size;
      } randomize;
      struct {
        int width;
        int stride;
      } striped;
      struct {
        int width;
        int length;
        float modulation;
      } paved;
    };
  };

  struct AtomId {
    gf::Id hash = gf::InvalidId;
    std::string name;
  };

  struct Atom {
    AtomId id;
    gf::Color4f color = gf::Color4f(0.0f, 0.0f, 0.0f, 0.0f);
    Pigment pigment;
  };


  enum class BorderEffect {
    None,
    Fade,
    Outline,
    Sharpen,
    Lighten,
    Blur,
    Blend,
  };

  struct Border {
    AtomId id;
    BorderEffect effect = BorderEffect::None;

    union {
      struct {
      } none;
      struct {
        int distance;
      } fade;
      struct {
        int distance;
        float factor;
      } outline;
      struct {
        int distance;
        float max;
      } sharpen;
      struct {
        int distance;
        float max;
      } lighten;
      struct {
      } blur;
      struct {
        int distance;
      } blend;
    };
  };

  struct Displacement {
    int iterations = 2;
    float initial = 0.5f;
    float reduction = 0.5f;
  };

  struct Edge {
    int offset = 0;
    Displacement displacement;
    bool limit = false;

    Edge invert() const {
      return { -offset, displacement, limit };
    }
  };

  struct Wang2 {
    Border borders[2];
    Edge edge;

    bool isOverlay() const {
      return borders[1].id.hash == Void;
    }
  };

  struct Wang3 {
    AtomId ids[3];

    bool isOverlay() const {
      return ids[2].hash == Void;
    }
  };

  constexpr int AtomsTilesetSize = 4;
  constexpr int Wang2TilesetSize = 4;
  constexpr int Wang3TilesetSize = 6;

  enum class Search {
    UseDatabaseOnly,
    IncludeTemporary
  };


  struct TilesetData {
    Settings settings;
    std::vector<Atom> atoms;
    std::vector<Wang2> wang2;
    std::vector<Wang3> wang3;

    struct {
      Atom atom;
      Wang2 wang2;
    } temporary;

    Atom getAtom(gf::Id hash, Search search = Search::UseDatabaseOnly) const;
    Wang2 getWang2(gf::Id id0, gf::Id id1, Search search = Search::UseDatabaseOnly) const;
    Edge getEdge(gf::Id id0, gf::Id id1, Search search = Search::UseDatabaseOnly) const;

    void updateAtom(Atom oldAtom, Atom newAtom);
    void deleteAtom(gf::Id id);

    void generateAllWang3();

    static TilesetData load(const gf::Path& filename);
    static void save(const gf::Path& filename, const TilesetData& data);
  };

}

#endif // TILESET_DATA_H
