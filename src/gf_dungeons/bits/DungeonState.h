#ifndef DUNGEON_STATE_H
#define DUNGEON_STATE_H

#include <gf/Random.h>
#include <gf/VertexArray.h>

#include "DungeonGenerator.h"
#include "DungeonGenerator_BinarySpacePartitioning.h"
#include "DungeonGenerator_CellularAutomaton.h"
#include "DungeonGenerator_DrunkardMarch.h"
#include "DungeonGenerator_Tunneling.h"

namespace gftools {

  enum class GeneratorType : int {
    CellularAutomaton       = 0,
    DrunkardMarch           = 1,
    Tunneling               = 2,
    BinarySpacePartitioning = 3,
  };

  struct DungeonState {
    DungeonState();

    GeneratorType type = GeneratorType::CellularAutomaton;

    CellularAutomaton cellular;
    DrunkardMarch march;
    Tunneling tunneling;
    BinarySpacePartitioning bsp;

    Dungeon dungeon;
    int dungeonSize = 64;
    int log2DungeonSize = 6;

    gf::Random random;
    DungeonGenerator *currentGenerator = nullptr;
    gf::VertexArray vertices;


    void updateDisplayWith(GeneratorType type);
  };

}


#endif // DUNGEON_STATE_H
