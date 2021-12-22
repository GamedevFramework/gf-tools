#include "TilesetData.h"

#include <cinttypes>
#include <cstdint>
#include <fstream>

#include <nlohmann/json.hpp>

#include <gf/Color.h>
#include <gf/Log.h>

using JSON = nlohmann::basic_json<nlohmann::ordered_map, std::vector, std::string, bool, int32_t, uint32_t, float>;

namespace nlohmann {
  template<>
  struct adl_serializer<gf::Color4f> {
    static void to_json(JSON& j, const gf::Color4f& color) {
      gf::Color4u rgba32 = gf::Color::toRgba32(color);
      j = JSON{ rgba32.r, rgba32.g, rgba32.b, rgba32.a };
    }

    static void from_json(const JSON& j, gf::Color4f& color) {
      gf::Color4u rgba32;
      j.at(0).get_to(rgba32.r);
      j.at(1).get_to(rgba32.g);
      j.at(2).get_to(rgba32.b);
      j.at(3).get_to(rgba32.a);
      color = gf::Color::fromRgba32(rgba32);
    }
  };
}

namespace gftools {

  ImageFeatures Settings::getImageFeatures() const {
    auto size = tile.getExtendedSize();
    int step = size * 12;

    for (int width = step; width <= 8192; width += step) {
      int height = 0;

      int atomsPerLine = width / (AtomsTilesetSize * size);

      if (atomsPerLine == 0) {
        continue;
      }

      int atomsLineCount = maxAtomCount / atomsPerLine + ((maxAtomCount % atomsPerLine == 0) ? 0 : 1);
      height += atomsLineCount * (AtomsTilesetSize * size);

      if (height > width) {
        continue;
      }

      int wang2PerLine = width / (Wang2TilesetSize * size);

      if (wang2PerLine == 0) {
        continue;
      }

      int wang2LineCount = maxWang2Count / wang2PerLine + ((maxWang2Count % wang2PerLine == 0) ? 0 : 1);
      height += wang2LineCount * (Wang2TilesetSize * size);

      if (height > width) {
        continue;
      }

      int wang3PerLine = width / (Wang3TilesetSize * size);

      if (wang3PerLine == 0) {
        continue;
      }

      int wang3LineCount = maxWang3Count / wang3PerLine + ((maxWang3Count % wang3PerLine == 0) ? 0 : 1);
      height += wang3LineCount * (Wang3TilesetSize * size);

      if (height > width) {
        continue;
      }

      gf::Log::debug("atoms: %i x %i (%i)\n", atomsPerLine, atomsLineCount, atomsPerLine * atomsLineCount);
      gf::Log::debug("wang2: %i x %i (%i)\n", wang2PerLine, wang2LineCount, wang2PerLine * wang2LineCount);
      gf::Log::debug("wang3: %i x %i (%i)\n", wang3PerLine, wang3LineCount, wang3PerLine * wang3LineCount);

      ImageFeatures features;
      features.size = gf::vec(width, height);
      features.atomsPerLine = atomsPerLine;
      features.atomsLineCount = atomsLineCount;
      features.wang2PerLine = wang2PerLine;
      features.wang2LineCount = wang2LineCount;
      features.wang3PerLine = wang3PerLine;
      features.wang3LineCount = wang3LineCount;

      return features;
    }

    return ImageFeatures{};
  }

  gf::Vector2i Settings::getImageSize() const {
    return getImageFeatures().size;
  }

  Atom TilesetData::getAtom(gf::Id hash, Search search) const {
    if (search == Search::IncludeTemporary) {
      if (temporary.atom.id.hash == hash) {
        return temporary.atom;
      }
    }

    for (auto& atom : atoms) {
      if (atom.id.hash == hash) {
        return atom;
      }
    }

    Atom voidAtom;
    voidAtom.id.hash = Void;
    voidAtom.id.name = "-";
    voidAtom.color = gf::Color::Transparent;
    voidAtom.pigment.style = PigmentStyle::Plain;

    if (hash != Void) {
      gf::Log::warning("Unknown atom hash: %" PRIX64 "\n", hash);
    }

    return voidAtom;
  }

  Wang2 TilesetData::getWang2(gf::Id id0, gf::Id id1, Search search) const {
    if (search == Search::IncludeTemporary) {
      if (temporary.wang2.borders[0].id.hash == id0 && temporary.wang2.borders[1].id.hash == id1) {
        return temporary.wang2;
      }

      if (temporary.wang2.borders[0].id.hash == id1 && temporary.wang2.borders[1].id.hash == id0) {
        return temporary.wang2;
      }
    }

    for (auto& wang : wang2) {
      if (wang.borders[0].id.hash == id0 && wang.borders[1].id.hash == id1) {
        return wang;
      }

      if (wang.borders[0].id.hash == id1 && wang.borders[1].id.hash == id0) {
        return wang;
      }
    }

    Atom a0 = getAtom(id0);
    Atom a1 = getAtom(id1);

    gf::Log::warning("No wang2 for this pair of atoms: (%s, %s)\n", a0.id.name.c_str(), a1.id.name.c_str());

    Wang2 wang;
    wang.borders[0].id = a0.id;
    wang.borders[0].effect = BorderEffect::None;
    wang.borders[1].id = a1.id;
    wang.borders[1].effect = BorderEffect::None;

    if (id0 == Void) {
      std::swap(wang.borders[0].id, wang.borders[1].id);
    }

    return wang;
  }

  Edge TilesetData::getEdge(gf::Id id0, gf::Id id1, Search search) const {
    if (search == Search::IncludeTemporary) {
      if (temporary.wang2.borders[0].id.hash == id0 && temporary.wang2.borders[1].id.hash == id1) {
        return temporary.wang2.edge;
      }

      if (temporary.wang2.borders[0].id.hash == id1 && temporary.wang2.borders[1].id.hash == id0) {
        return temporary.wang2.edge.invert();
      }
    }

    for (auto& wang : wang2) {
      if (wang.borders[0].id.hash == id0 && wang.borders[1].id.hash == id1) {
        return wang.edge;
      }

      if (wang.borders[0].id.hash == id1 && wang.borders[1].id.hash == id0) {
        return wang.edge.invert();
      }
    }

    Edge edge;
    return edge;
  }

  void TilesetData::updateAtom(Atom oldAtom, Atom newAtom) {
    for (auto& atom : atoms) {
      if (atom.id.hash == oldAtom.id.hash) {
        atom = newAtom;
      }
    }

    for (auto& wang : wang2) {
      for (auto& border : wang.borders) {
        if (border.id.hash == oldAtom.id.hash) {
          border.id = newAtom.id;
        }
      }
    }

    for (auto& wang : wang3) {
      for (auto& id : wang.ids) {
        if (id.hash == oldAtom.id.hash) {
          id = newAtom.id;
        }
      }
    }
  }

  void TilesetData::deleteAtom(gf::Id id) {
    atoms.erase(std::remove_if(atoms.begin(), atoms.end(), [id](auto& atom) {
      return atom.id.hash == id;
    }), atoms.end());

    wang2.erase(std::remove_if(wang2.begin(), wang2.end(), [id](auto& wang) {
      return wang.borders[0].id.hash == id || wang.borders[1].id.hash == id;
    }), wang2.end());

    wang3.erase(std::remove_if(wang3.begin(), wang3.end(), [id](auto& wang) {
      return wang.ids[0].hash == id || wang.ids[1].hash == id || wang.ids[2].hash == id;
    }), wang3.end());
  }

  void TilesetData::generateAllWang3() {
    wang3.clear();
    std::size_t count = wang2.size();

    for (std::size_t i = 0; i < count; ++i) {
      auto & w0 = wang2[i];

      for (std::size_t j = i + 1; j < count; ++j) {
        auto & w1 = wang2[j];

        for (std::size_t k = j + 1; k < count; ++k) {
          auto & w2 = wang2[k];

          std::array<AtomId, 6> ids = {
            w0.borders[0].id, w0.borders[1].id,
            w1.borders[0].id, w1.borders[1].id,
            w2.borders[0].id, w2.borders[1].id,
          };

          std::sort(ids.begin(), ids.end(), [](const AtomId & lhs, const AtomId & rhs) { return lhs.hash < rhs.hash; });

          if (ids[0].hash == ids[1].hash && ids[2].hash == ids[3].hash && ids[4].hash == ids[5].hash) {
            Wang3 wang;
            wang.ids[0] = ids[0];
            wang.ids[1] = ids[2];
            wang.ids[2] = ids[4];

            if (wang.ids[0].hash == Void) {
              std::swap(wang.ids[0], wang.ids[2]);
            }

            if (wang.ids[1].hash == Void) {
              std::swap(wang.ids[1], wang.ids[2]);
            }

            wang3.push_back(wang);
          }
        }
      }
    }
  }

  /*
   * parsing and saving in JSON
   */

  NLOHMANN_JSON_SERIALIZE_ENUM( PigmentStyle, {
    { PigmentStyle::Plain, "plain" },
    { PigmentStyle::Randomize, "randomize"},
    { PigmentStyle::Striped, "striped"},
    { PigmentStyle::Paved, "paved"},
  })

  NLOHMANN_JSON_SERIALIZE_ENUM( BorderEffect, {
    { BorderEffect::None, "none" },
    { BorderEffect::Fade, "fade" },
    { BorderEffect::Outline, "outline" },
    { BorderEffect::Sharpen, "sharpen" },
    { BorderEffect::Lighten, "lighten" },
    { BorderEffect::Blur, "blur" },
    { BorderEffect::Blend, "blend" },
  })

  void to_json(JSON& j, const Settings& settings) {
    JSON tile = JSON{
      { "size", settings.tile.size },
      { "spacing", settings.tile.spacing }
    };

    j = JSON{
      { "locked", settings.locked },
      { "max_atom_count", settings.maxAtomCount },
      { "max_wang2_count", settings.maxWang2Count },
      { "max_wang3_count", settings.maxWang3Count },
      { "tile", tile }
    };
  }

  void from_json(const JSON& j, Settings& settings) {
    j.at("locked").get_to(settings.locked);
    j.at("max_atom_count").get_to(settings.maxAtomCount);
    j.at("max_wang2_count").get_to(settings.maxWang2Count);
    j.at("max_wang3_count").get_to(settings.maxWang3Count);
    j.at("tile").at("size").get_to(settings.tile.size);
    j.at("tile").at("spacing").get_to(settings.tile.spacing);
  }

  void to_json(JSON& j, const Pigment& pigment) {
    switch (pigment.style) {
      case PigmentStyle::Plain:
        j = JSON{
          { "style", pigment.style }
        };
        break;
      case PigmentStyle::Randomize:
        j = JSON{
          { "style", pigment.style },
          { "ratio", pigment.randomize.ratio },
          { "deviation", pigment.randomize.deviation },
          { "size", pigment.randomize.size },
        };
        break;
      case PigmentStyle::Striped:
        j = JSON{
          { "style", pigment.style },
          { "width", pigment.striped.width },
          { "stride", pigment.striped.stride }
        };
        break;
      case PigmentStyle::Paved:
        j = JSON{
          { "style", pigment.style },
          { "width", pigment.paved.width },
          { "length", pigment.paved.length },
          { "modulation", pigment.paved.modulation }
        };
    }
  }

  void from_json(const JSON& j, Pigment& pigment) {
    j.at("style").get_to(pigment.style);

    switch (pigment.style) {
      case PigmentStyle::Plain:
        break;
      case PigmentStyle::Randomize:
        j.at("ratio").get_to(pigment.randomize.ratio);
        j.at("deviation").get_to(pigment.randomize.deviation);
        j.at("size").get_to(pigment.randomize.size);
        break;
      case PigmentStyle::Striped:
        j.at("width").get_to(pigment.striped.width);
        j.at("stride").get_to(pigment.striped.stride);
        break;
      case PigmentStyle::Paved:
        j.at("width").get_to(pigment.paved.width);
        j.at("length").get_to(pigment.paved.length);
        j.at("modulation").get_to(pigment.paved.modulation);
        break;
    }
  }

  void to_json(JSON& j, const AtomId& id) {
    j = JSON(id.name);
  }

  void from_json(const JSON& j, AtomId& id) {
    j.get_to(id.name);
    id.hash = gf::hash(id.name);
  }

  void to_json(JSON& j, const Atom& atom) {
    j = JSON{
      { "id", atom.id },
      { "color", atom.color },
      { "pigment", atom.pigment }
    };
  }

  void from_json(const JSON& j, Atom& atom) {
    j.at("id").get_to(atom.id);
    j.at("color").get_to(atom.color);
    j.at("pigment").get_to(atom.pigment);
  }

  void to_json(JSON& j, const Border& border) {
    switch (border.effect) {
      case BorderEffect::None:
        j = JSON{
          { "id", border.id },
          { "effect", border.effect }
        };
        break;
      case BorderEffect::Fade:
        j = JSON{
          { "id", border.id },
          { "effect", border.effect },
          { "distance", border.fade.distance }
        };
        break;
      case BorderEffect::Outline:
        j = JSON{
          { "id", border.id },
          { "effect", border.effect },
          { "distance", border.outline.distance },
          { "factor", border.outline.factor }
        };
        break;
      case BorderEffect::Sharpen:
        j = JSON{
          { "id", border.id },
          { "effect", border.effect },
          { "distance", border.sharpen.distance }
        };
        break;
      case BorderEffect::Lighten:
        j = JSON{
          { "id", border.id },
          { "effect", border.effect },
          { "distance", border.lighten.distance }
        };
        break;
      case BorderEffect::Blur:
        j = JSON{
          { "id", border.id },
          { "effect", border.effect }
        };
        break;
      case BorderEffect::Blend:
        j = JSON{
          { "id", border.id },
          { "effect", border.effect },
          { "distance", border.blend.distance }
        };
        break;
    }
  }

  void from_json(const JSON& j, Border& border) {
    j.at("id").get_to(border.id);
    j.at("effect").get_to(border.effect);

    switch (border.effect) {
      case BorderEffect::None:
        break;
      case BorderEffect::Fade:
        j.at("distance").get_to(border.fade.distance);
        break;
      case BorderEffect::Outline:
        j.at("distance").get_to(border.outline.distance);
        j.at("factor").get_to(border.outline.factor);
        break;
      case BorderEffect::Sharpen:
        j.at("distance").get_to(border.sharpen.distance);
        break;
      case BorderEffect::Lighten:
        j.at("distance").get_to(border.lighten.distance);
        break;
      case BorderEffect::Blur:
        break;
      case BorderEffect::Blend:
        j.at("distance").get_to(border.blend.distance);
        break;
    }
  }

  void to_json(JSON& j, const Displacement& displacement) {
    j = JSON{
      { "iterations", displacement.iterations },
      { "initialFactor", displacement.initial },
      { "reductionFactor", displacement.reduction }
    };
  }

  void from_json(const JSON& j, Displacement& displacement) {
    j.at("iterations").get_to(displacement.iterations);
    j.at("initialFactor").get_to(displacement.initial);
    j.at("reductionFactor").get_to(displacement.reduction);
  }

  void to_json(JSON& j, const Wang2& wang) {
    j = JSON{
      { "borders", wang.borders },
      { "offset", wang.edge.offset },
      { "displacement", wang.edge.displacement },
      { "limit", wang.edge.limit }
    };
  }

  void from_json(const JSON& j, Wang2& wang) {
    j.at("borders").get_to(wang.borders);
    j.at("offset").get_to(wang.edge.offset);
    j.at("displacement").get_to(wang.edge.displacement);
    j.at("limit").get_to(wang.edge.limit);
  }

  void to_json(JSON& j, const Wang3& wang) {
    j = JSON(wang.ids);
  }

  void from_json(const JSON& j, Wang3& wang) {
    j.get_to(wang.ids);
  }

  void to_json(JSON& j, const TilesetData& data) {
    j = JSON{
      { "settings", data.settings },
      { "atoms", data.atoms },
      { "wang2", data.wang2 },
      { "wang3", data.wang3 }
    };
  }

  void from_json(const JSON& j, TilesetData& data) {
    j.at("settings").get_to(data.settings);
    j.at("atoms").get_to(data.atoms);
    j.at("wang2").get_to(data.wang2);
    j.at("wang3").get_to(data.wang3);
  }

  TilesetData TilesetData::load(const gf::Path& filename) {
    TilesetData data;

    try {
      std::ifstream ifs(filename.string());
      auto j = JSON::parse(ifs, nullptr, true, true);
      data = j.get<TilesetData>();
    } catch (std::exception& ex) {
      gf::Log::error("An error occurred while parsing file '%s': %s\n", filename.string().c_str(), ex.what());
    }

    return data;
  }

  void TilesetData::save(const gf::Path& filename, const TilesetData& data) {
    try {
      JSON j = data;
      std::ofstream ofs(filename.string());
      ofs << j.dump(1, '\t') << '\n';
      gf::Log::info("Project successfully saved in '%s'\n", filename.string().c_str());
    } catch (std::exception& ex) {
      gf::Log::error("An error occurred while saving file '%s': %s\n", filename.string().c_str(), ex.what());
    }
  }

}
