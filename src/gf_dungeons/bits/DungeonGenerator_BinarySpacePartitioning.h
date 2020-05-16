#ifndef DUNGEON_BINARY_SPACE_PARTITIONING_H
#define DUNGEON_BINARY_SPACE_PARTITIONING_H

#include <memory>

#include "DungeonGenerator.h"

namespace gftools {

  struct BinarySpacePartitioningTree {
    std::unique_ptr<BinarySpacePartitioningTree> left;
    std::unique_ptr<BinarySpacePartitioningTree> right;

    gf::RectI space;
    gf::RectI room;

    BinarySpacePartitioningTree(gf::RectI initialSpace);

    bool split(gf::Random& random, int leafSizeMinimum);
    void recursiveSplit(gf::Random& random, int leafSizeMinimum, int leafSizeMaximum);
    void createRooms(gf::Random& random, int roomSizeMinimum, int roomSizeMaximum);

  };

  class BinarySpacePartitioning : public DungeonGenerator {
  public:
    BinarySpacePartitioning();

    // public parameters

    int leafSizeMinimum = 10;
    int leafSizeMaximum = 24;
    int roomSizeMinimum = 6;
    int roomSizeMaximum = 15;

    Dungeon generate(gf::Vector2i size, gf::Random& random) override;

  private:
    void generateRooms(gf::Vector2i size);
    void walkTree(const BinarySpacePartitioningTree& tree);
    void createRoom(const gf::RectI& room);
    void createHorizontalTunnel(int x1, int x2, int y);
    void createVerticalTunnel(int x, int y1, int y2);

  private:
    gf::Random m_savedRandom;
    gf::Random m_random;
    BinarySpacePartitioningTree m_root;
    Dungeon m_dungeon;
  };

}

#endif // DUNGEON_BINARY_SPACE_PARTITIONING_H
