#ifndef DUNGEON_DRUNKARD_MARCH_H
#define DUNGEON_DRUNKARD_MARCH_H

#include <gf/Direction.h>

#include "DungeonGenerator.h"

namespace gftools {

  class DrunkardMarch : public DungeonGenerator {
  public:
    // public parameters

    float percentGoal                 = 0.4f;
    float weightForCenter             = 0.15f;
    float weightForPreviousDirection  = 0.7f;

    Dungeon generate(gf::Vector2i size, gf::Random& random) override;

  private:
    void generateDungeon(gf::Vector2i size, gf::Random& random);
    void walk(gf::Vector2i size, gf::Random& random);

  private:
    Dungeon m_dungeon;
    int m_filled;
    gf::Direction m_currentDirection;
    gf::Vector2i m_currentPosition;
  };

}

#endif // DUNGEON_DRUNKARD_MARCH_H
