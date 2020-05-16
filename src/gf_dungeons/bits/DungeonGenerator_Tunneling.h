#ifndef DUNGEON_TUNNELING_H
#define DUNGEON_TUNNELING_H

#include "DungeonGenerator.h"

namespace gftools {

  class Tunneling : public DungeonGenerator {
  public:
    // public parameters

    int maxRooms        = 30;
    int roomSizeMinimum = 6;
    int roomSizeMaximum = 10;

    Dungeon generate(gf::Vector2i size, gf::Random& random) override;

  private:
    void generateRooms(gf::Vector2i size);
    void createRoom(const gf::RectI& room);
    void createHorizontalTunnel(int x1, int x2, int y);
    void createVerticalTunnel(int x, int y1, int y2);

  private:
    gf::Random m_savedRandom;
    gf::Random m_random;
    std::vector<gf::RectI> m_rooms;
    Dungeon m_dungeon;
  };

}

#endif // DUNGEON_TUNNELING_H
