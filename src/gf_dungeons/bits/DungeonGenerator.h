#ifndef DUNGEON_GENERATOR_H
#define DUNGEON_GENERATOR_H

#include <gf/Array2D.h>
#include <gf/Random.h>
#include <gf/Vector.h>

namespace gftools {

  enum class CellState : int {
    Wall,
    Path,
  };

  using Dungeon = gf::Array2D<CellState, int>;

  class DungeonGenerator {
  public:
    static constexpr float CellSize = 16.0f;

    enum class Phase {
      Start,
      Iterate,
      Finish,
    };

    DungeonGenerator();
    virtual ~DungeonGenerator();

    Phase getPhase() const {
      return m_phase;
    }

    void setPhase(Phase phase) {
      m_phase = phase;
    }

    virtual Dungeon generate(gf::Vector2i size, gf::Random& random) = 0;

  private:
    Phase m_phase;
  };

}


#endif // DUNGEON_GENERATOR_H
