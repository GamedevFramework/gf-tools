#include "DungeonGenerator_DrunkardMarch.h"

namespace gftools {

  Dungeon DrunkardMarch::generate(gf::Vector2i size, gf::Random& random) {
    switch (getPhase()) {
      case Phase::Start:
        // fallthrough
      case Phase::Iterate:
        generateDungeon(size, random);
        // fallthrough
      case Phase::Finish:
        break;
    }

    setPhase(Phase::Finish);
    return m_dungeon;
  }

  void DrunkardMarch::generateDungeon(gf::Vector2i size, gf::Random& random) {
    m_dungeon = Dungeon(size, CellState::Wall);

    m_filled = 0;
    m_currentDirection = gf::Direction::Center;

    m_currentPosition.x = random.computeUniformInteger(2, size.width - 2);
    m_currentPosition.y = random.computeUniformInteger(2, size.height - 2);

    int filledGoal = size.width * size.height * percentGoal;
    int maxIterations = size.width * size.height * 10;

    for (int i = 0; i < maxIterations; ++i) {
      walk(size, random);

      if (m_filled >= filledGoal) {
        break;
      }
    }
  }

  void DrunkardMarch::walk(gf::Vector2i size, gf::Random& random) {
    static constexpr gf::Direction Directions[4] = {
      gf::Direction::Up,
      gf::Direction::Right,
      gf::Direction::Down,
      gf::Direction::Left
    };

    static constexpr float EdgePercent = 0.25f;

    double upWeigth = 1.0;
    double rightWeight = 1.0;
    double downWeight = 1.0;
    double leftWeight = 1.0;

    if (m_currentPosition.x <= size.width * EdgePercent) {
      rightWeight += weightForCenter;
    }

    if (m_currentPosition.x >= size.width * (1 - EdgePercent)) {
      leftWeight += weightForCenter;
    }

    if (m_currentPosition.y <= size.height * EdgePercent) {
      downWeight += weightForCenter;
    }

    if (m_currentPosition.y >= size.height * (1 - EdgePercent)) {
      upWeigth += weightForCenter;
    }

    switch (m_currentDirection) {
      case gf::Direction::Up:
        upWeigth += weightForPreviousDirection;
        break;
      case gf::Direction::Right:
        rightWeight += weightForPreviousDirection;
        break;
      case gf::Direction::Down:
        downWeight += weightForPreviousDirection;
        break;
      case gf::Direction::Left:
        leftWeight += weightForPreviousDirection;
        break;
      default:
        break;
    }

    std::discrete_distribution<int> distribution({ upWeigth, rightWeight, downWeight, leftWeight });
    int chosenDirection = distribution(random.getEngine());
    gf::Direction newDirection = Directions[chosenDirection];
    gf::Vector2i newPosition = m_currentPosition;

    switch (newDirection) {
      case gf::Direction::Up:
        if (newPosition.y > 2) {
          --newPosition.y;
        }
        break;
      case gf::Direction::Down:
        if (newPosition.y < size.height - 2) {
          ++newPosition.y;
        }
        break;
      case gf::Direction::Left:
        if (newPosition.x > 2) {
          --newPosition.x;
        }
        break;
      case gf::Direction::Right:
        if (newPosition.x < size.width - 2) {
          ++newPosition.x;
        }
        break;
      default:
        break;
    }

    if (m_currentPosition != newPosition) {
      if (m_dungeon(newPosition) == CellState::Wall) {
        m_dungeon(newPosition) = CellState::Path;
        ++m_filled;
      }

      m_currentPosition = newPosition;
      m_currentDirection = newDirection;
    }
  }

}


