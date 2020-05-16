#include "DungeonState.h"

namespace gftools {

  namespace {

    void computeDisplay(const Dungeon& dungeon, gf::VertexArray& vertices) {
      vertices.clear();

      for (auto row : dungeon.getRowRange()) {
        for (auto col : dungeon.getColRange()) {
          gf::Vector2u pos(col, row);


          gf::Vertex v[4];
          v[0].position = pos * DungeonGenerator::CellSize;
          v[1].position = pos * DungeonGenerator::CellSize + gf::Vector2f(DungeonGenerator::CellSize, 0.0f);
          v[2].position = pos * DungeonGenerator::CellSize + gf::Vector2f(0.0f, DungeonGenerator::CellSize);
          v[3].position = pos * DungeonGenerator::CellSize + gf::Vector2f(DungeonGenerator::CellSize, DungeonGenerator::CellSize);

          if (dungeon(pos) == CellState::Path) {
            v[0].color = v[1].color = v[2].color = v[3].color = gf::Color::White;
          } else {
            v[0].color = v[1].color = v[2].color = v[3].color = gf::Color::Black;
          }

          vertices.append(v[0]);
          vertices.append(v[1]);
          vertices.append(v[2]);

          vertices.append(v[2]);
          vertices.append(v[1]);
          vertices.append(v[3]);
        }
      }
    }

  }

  DungeonState::DungeonState()
  : currentGenerator(&cellular)
  , vertices(gf::PrimitiveType::Triangles)
  {
    dungeon = currentGenerator->generate({ dungeonSize, dungeonSize }, random);
    computeDisplay(dungeon, vertices);
  }

  void DungeonState::updateDisplayWith(GeneratorType newType) {
    if (type != newType) {
      type = newType;
      currentGenerator->setPhase(DungeonGenerator::Phase::Start);
    }

    if (currentGenerator->getPhase() != DungeonGenerator::Phase::Finish) {
      dungeon = currentGenerator->generate({ dungeonSize, dungeonSize }, random);
      computeDisplay(dungeon, vertices);
    }
  }

}
