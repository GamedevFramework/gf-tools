#include "DungeonGenerator_BinarySpacePartitioning.h"

namespace gftools {

  BinarySpacePartitioningTree::BinarySpacePartitioningTree(gf::RectI initialSpace)
  : left(nullptr)
  , right(nullptr)
  , space(initialSpace)
  {
  }

  bool BinarySpacePartitioningTree::split(gf::Random& random, int leafSizeMinimum) {
    if (left || right) {
      return false;
    }

    bool splitHorizontally = random.computeBernoulli(0.5f);

    if (space.getWidth() >= 1.25 * space.getHeight()) {
      splitHorizontally = false;
    } else if (space.getHeight() >= 1.25 * space.getWidth()) {
      splitHorizontally = true;
    }

    int max = splitHorizontally ? space.getHeight() : space.getWidth();

    if (max <= 2 * leafSizeMinimum) {
      return false;
    }

    assert(leafSizeMinimum <= max - leafSizeMinimum);
    int split = random.computeUniformInteger(leafSizeMinimum, max - leafSizeMinimum);

    if (splitHorizontally) {
      left = std::make_unique<BinarySpacePartitioningTree>(gf::RectI::fromPositionSize(space.min, { space.getWidth(), split }));
      right = std::make_unique<BinarySpacePartitioningTree>(gf::RectI::fromPositionSize({ space.min.x, space.min.y + split }, { space.getWidth(), space.getHeight() - split }));
    } else {
      left = std::make_unique<BinarySpacePartitioningTree>(gf::RectI::fromPositionSize(space.min, { split, space.getHeight() }));
      right = std::make_unique<BinarySpacePartitioningTree>(gf::RectI::fromPositionSize({ space.min.x + split, space.min.y }, { space.getWidth() - split, space.getHeight() }));
    }

    return true;
  }

  void BinarySpacePartitioningTree::recursiveSplit(gf::Random& random, int leafSizeMinimum, int leafSizeMaximum) {
    assert(!left && !right);
    assert(leafSizeMinimum <= leafSizeMaximum);

    if (space.getWidth() > leafSizeMaximum || space.getHeight() > leafSizeMaximum || random.computeBernoulli(0.2)) {
      if (split(random, leafSizeMinimum)) {
        assert(left);
        left->recursiveSplit(random, leafSizeMinimum, leafSizeMaximum);
        assert(right);
        right->recursiveSplit(random, leafSizeMinimum, leafSizeMaximum);
      }
    }
  }

  void BinarySpacePartitioningTree::createRooms(gf::Random& random, int roomSizeMinimum, int roomSizeMaximum) {
    assert(roomSizeMinimum <= roomSizeMaximum);

    if (left || right) {
      assert(left && right);

      left->createRooms(random, roomSizeMinimum, roomSizeMaximum);
      right->createRooms(random, roomSizeMinimum, roomSizeMaximum);

      if (random.computeBernoulli(0.5)) {
        room = left->room;
      } else {
        room = right->room;
      }
    } else {
      assert(roomSizeMinimum <= std::min(roomSizeMaximum, space.getWidth() - 1));
      assert(roomSizeMinimum <= std::min(roomSizeMaximum, space.getHeight() - 1));

      gf::Vector2i position, size;
      size.width = random.computeUniformInteger(roomSizeMinimum, std::min(roomSizeMaximum, space.getWidth() - 1));
      size.height = random.computeUniformInteger(roomSizeMinimum, std::min(roomSizeMaximum, space.getHeight() - 1));
      position.x = random.computeUniformInteger(0, space.getWidth() - size.width - 1);
      position.y = random.computeUniformInteger(0, space.getHeight() - size.height - 1);
      position += space.getPosition();

      room = gf::RectI::fromPositionSize(position, size);
    }
  }


  BinarySpacePartitioning::BinarySpacePartitioning()
  : m_root(gf::RectI::fromPositionSize({ 0, 0 }, { 1, 1 }))
  {
  }

  Dungeon BinarySpacePartitioning::generate(gf::Vector2i size, gf::Random& random) {
    switch (getPhase()) {
      case Phase::Start:
        m_savedRandom = random;
        // fallthrough
      case Phase::Iterate:
        m_random = m_savedRandom;
        generateRooms(size);
        // fallthrough
      case Phase::Finish:
        random = m_random;
        break;
    }

    setPhase(Phase::Finish);
    return m_dungeon;
  }

  void BinarySpacePartitioning::generateRooms(gf::Vector2i size) {
    m_dungeon = Dungeon(size, CellState::Wall);

    m_root.space = gf::RectI::fromPositionSize({ 0, 0 }, size);
    m_root.left = nullptr;
    m_root.right = nullptr;

    m_root.recursiveSplit(m_random, leafSizeMinimum, leafSizeMaximum);
    m_root.createRooms(m_random, roomSizeMinimum, roomSizeMaximum);
    walkTree(m_root);
  }

  void BinarySpacePartitioning::walkTree(const BinarySpacePartitioningTree& tree) {
    if (tree.left || tree.right) {
      assert(tree.left && tree.right);
      walkTree(*tree.left);
      walkTree(*tree.right);

      auto leftRoom = tree.left->room.getCenter();
      auto rightRoom = tree.right->room.getCenter();

      if (m_random.computeBernoulli(0.5)) {
        createHorizontalTunnel(rightRoom.x, leftRoom.x, rightRoom.y);
        createVerticalTunnel(leftRoom.x, leftRoom.y, rightRoom.y);
      } else {
        createVerticalTunnel(rightRoom.x, leftRoom.y, rightRoom.y);
        createHorizontalTunnel(rightRoom.x, leftRoom.x, leftRoom.y);
      }
    } else {
      createRoom(tree.room);
    }
  }

  void BinarySpacePartitioning::createRoom(const gf::RectI& room) {
    for (int x = room.min.x + 1; x < room.max.x; ++x) {
      for (int y = room.min.y + 1; y < room.max.y; ++y) {
        m_dungeon({ x, y }) = CellState::Path;
      }
    }
  }

  void BinarySpacePartitioning::createHorizontalTunnel(int x1, int x2, int y) {
    if (x2 < x1) {
      std::swap(x1, x2);
    }

    for (int x = x1; x <= x2; ++x) {
      m_dungeon({ x, y }) = CellState::Path;
    }
  }

  void BinarySpacePartitioning::createVerticalTunnel(int x, int y1, int y2) {
    if (y2 < y1) {
      std::swap(y1, y2);
    }

    for (int y = y1; y <= y2; ++y) {
      m_dungeon({ x, y }) = CellState::Path;
    }
  }

}
