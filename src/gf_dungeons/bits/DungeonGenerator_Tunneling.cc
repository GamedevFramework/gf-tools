#include "DungeonGenerator_Tunneling.h"

namespace gftools {

  Dungeon Tunneling::generate(gf::Vector2i size, gf::Random& random) {
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

  void Tunneling::generateRooms(gf::Vector2i size) {
    m_rooms.clear();
    m_dungeon = Dungeon(size, CellState::Wall);

    for (int i = 0; i < maxRooms; ++i) {
      gf::Vector2i roomPos, roomSize;
      roomSize.width = m_random.computeUniformInteger(roomSizeMinimum, roomSizeMaximum);
      roomSize.height = m_random.computeUniformInteger(roomSizeMinimum, roomSizeMaximum);
      roomPos.x = m_random.computeUniformInteger(0, size.width - roomSize.width - 1);
      roomPos.y = m_random.computeUniformInteger(0, size.height - roomSize.height - 1);

      gf::RectI room = gf::RectI::fromPositionSize(roomPos, roomSize);

      if (m_rooms.empty()) {
        createRoom(room);
        m_rooms.push_back(room);
      } else {
        if (std::any_of(m_rooms.begin(), m_rooms.end(), [&room](const gf::RectI& other) { return room.intersects(other); })) {
          continue;
        }

        createRoom(room);

        auto center = room.getCenter();
        auto previousCenter = m_rooms.back().getCenter();

        if (m_random.computeBernoulli(0.5)) {
          createHorizontalTunnel(previousCenter.x, center.x, previousCenter.y);
          createVerticalTunnel(center.x, center.y, previousCenter.y);
        } else {
          createVerticalTunnel(previousCenter.x, center.y, previousCenter.y);
          createHorizontalTunnel(previousCenter.x, center.x, center.y);
        }

        m_rooms.push_back(room);
      }
    }
  }

  void Tunneling::createRoom(const gf::RectI& room) {
    for (int x = room.min.x + 1; x < room.max.x; ++x) {
      for (int y = room.min.y + 1; y < room.max.y; ++y) {
        m_dungeon({ x, y }) = CellState::Path;
      }
    }
  }

  void Tunneling::createHorizontalTunnel(int x1, int x2, int y) {
    if (x2 < x1) {
      std::swap(x1, x2);
    }

    for (int x = x1; x <= x2; ++x) {
      m_dungeon({ x, y }) = CellState::Path;
    }
  }

  void Tunneling::createVerticalTunnel(int x, int y1, int y2) {
    if (y2 < y1) {
      std::swap(y1, y2);
    }

    for (int y = y1; y <= y2; ++y) {
      m_dungeon({ x, y }) = CellState::Path;
    }
  }

}

