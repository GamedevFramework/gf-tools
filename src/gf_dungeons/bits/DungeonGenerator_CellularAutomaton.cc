#include "DungeonGenerator_CellularAutomaton.h"

namespace gftools {

  namespace {

    static int getAliveCount(CellState state) {
      switch (state) {
        case CellState::Wall:
          return 0;
        case CellState::Path:
          return 1;
      }

      assert(false);
      return 0;
    }

    gf::Array2D<float> generateBase(gf::Vector2i size, gf::Random& random) {
      gf::Array2D<float> ret(size);

      for (auto& value : ret) {
        value = random.computeUniformFloat(0.0f, 1.0f);
      }

      return ret;
    }

    Dungeon computeFirst(const gf::Array2D<float>& array, float threshold) {
      Dungeon ret(array.getSize());

      for (auto pos : array.getPositionRange()) {
        if (array(pos) > threshold) {
          ret(pos) = CellState::Path;
        } else {
          ret(pos) = CellState::Wall;
        }
      }

      return ret;
    }

  }


  Dungeon CellularAutomaton::generate(gf::Vector2i size, gf::Random& random) {
    switch (getPhase()) {
      case Phase::Start:
        m_base = generateBase(size, random);
        // fallthrough
      case Phase::Iterate:
        m_dungeon = computeFirst(m_base, threshold);
        computeIterations();
        // fallthrough
      case Phase::Finish:
        break;
    }

    setPhase(Phase::Finish);
    return m_dungeon;
  }

  void CellularAutomaton::computeIterations() {
    Dungeon result(m_dungeon.getSize());

    for (int i = 0; i < iterations; ++i) {
      for (auto row : m_dungeon.getRowRange()) {
        for (auto col : m_dungeon.getColRange()) {
          gf::Vector2i pos(col, row);
          int count = 0;

          switch (mode) {
            case Mode::Diamond4: {
              for (auto neighbor : m_dungeon.get4NeighborsRange(pos)) {
                count += getAliveCount(m_dungeon(neighbor));
              }
              break;
            }
            case Mode::Square8:
              for (auto neighbor : m_dungeon.get8NeighborsRange(pos)) {
                count += getAliveCount(m_dungeon(neighbor));
              }
              break;
            case Mode::Diamond12:
              for (auto neighbor : m_dungeon.get12NeighborsRange(pos)) {
                count += getAliveCount(m_dungeon(neighbor));
              }
              break;
            case Mode::Square24:
              for (auto neighbor : m_dungeon.get24NeighborsRange(pos)) {
                count += getAliveCount(m_dungeon(neighbor));
              }
              break;
          }

          if (m_dungeon(pos) == CellState::Path) {
            if (count >= survivalThreshold) {
              result(pos) = CellState::Path;
            } else {
              result(pos) = CellState::Wall;
            }
          } else {
            if (count >= birthThreshold) {
              result(pos) = CellState::Path;
            } else {
              result(pos) = CellState::Wall;
            }
          }
        }
      }

      m_dungeon.swap(result);
    }
  }


}
