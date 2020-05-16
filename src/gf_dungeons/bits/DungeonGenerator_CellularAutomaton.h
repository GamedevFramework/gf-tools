#ifndef DUNGEON_CELLULAR_AUTOMATON_H
#define DUNGEON_CELLULAR_AUTOMATON_H

#include "DungeonGenerator.h"

namespace gftools {

  class CellularAutomaton : public DungeonGenerator {
  public:
    enum class Mode : int {
      Diamond4    = 0,
      Square8     = 1,
      Diamond12   = 2,
      Square24    = 3,
    };

    // public parameters

    float threshold       = 0.4f;
    Mode mode             = Mode::Square8;
    int survivalThreshold = 4;
    int birthThreshold    = 6;
    int iterations        = 5;

    Dungeon generate(gf::Vector2i size, gf::Random& random) override;

  private:
    void computeIterations();

  private:
    gf::Array2D<float> m_base;
    Dungeon m_dungeon;
  };

}

#endif // DUNGEON_CELLULAR_AUTOMATON_H
