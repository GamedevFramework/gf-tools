/*
 * gf-tools
 * Copyright (C) 2020 Julien Bernard
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#include "DungeonGui.h"

#include <gf/Clock.h>

#include <imgui.h>

#include "DungeonState.h"

namespace gftools {

  namespace {

    constexpr ImGuiWindowFlags DefaultWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    constexpr const char *GeneratorList[] = { "Cellular Automaton", "Drunkard March", "Tunneling", "Binary Space Paritioning" }; // see GeneratorType

    constexpr const char *ModeList[] = { "Diamond-4", "Square-8", "Diamond-12", "Square-24" }; // see CellularAutomaton::Mode


    int computeModeMax(int mode) {
      switch (mode) {
        case 0:
          return 4;
        case 1:
          return 8;
        case 2:
          return 12;
        case 3:
          return 24;
        default:
          assert(false);
          break;
      }

      assert(false);
      return 0;
    }

    void ensureGreater(int& param, int value) {
      if (param < value) {
        param = value;
      }
    }

    void ensureLess(int& param, int value) {
      if (param > value) {
        param = value;
      }
    }

  }

  DungeonGui::DungeonGui(DungeonState& state)
  : m_state(state)
  {
  }

  void DungeonGui::render([[maybe_unused]] gf::RenderTarget& target, [[maybe_unused]] const gf::RenderStates& states) {

    ImGui::SetNextWindowBgAlpha(0.7f);
    if (ImGui::Begin("Dungeon parameters", nullptr, DefaultWindowFlags)) {

      ImGui::Text("Size: %i", m_state.dungeonSize);
      if (ImGui::SliderInt("##Size", &m_state.log2DungeonSize, 5, 9, "")) {
        m_state.dungeonSize = 1 << m_state.log2DungeonSize;
        m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Start);
      }

      int half = m_state.dungeonSize / 2;

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::Combo("##Algorithm", &m_generatorChoice, GeneratorList, IM_ARRAYSIZE(GeneratorList));
      GeneratorType generator = static_cast<GeneratorType>(m_generatorChoice);

      switch (generator) {
        case GeneratorType::CellularAutomaton:
          m_state.currentGenerator = &m_state.cellular;

          ImGui::Text("Initial Ratio");
          if (ImGui::SliderFloat("##Ratio", &m_state.cellular.threshold, 0.0f, 1.0f, "%.2f")) {
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }

          ImGui::Text("Neighborhood");
          if (ImGui::Combo("##Neighborhood", &m_modeChoice, ModeList, IM_ARRAYSIZE(ModeList))) {
            m_state.cellular.mode = static_cast<CellularAutomaton::Mode>(m_modeChoice);
            m_state.cellular.survivalThreshold = std::min(m_state.cellular.survivalThreshold, computeModeMax(m_modeChoice));
            m_state.cellular.birthThreshold = std::min(m_state.cellular.birthThreshold, computeModeMax(m_modeChoice));
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }

          ImGui::Text("Survival Threshold");
          if (ImGui::SliderInt("##Survival", &m_state.cellular.survivalThreshold, 0, computeModeMax(m_modeChoice))) {
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }

          ImGui::Text("Birth Threshold");
          if (ImGui::SliderInt("##Birth", &m_state.cellular.birthThreshold, 0, computeModeMax(m_modeChoice))) {
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }

          ImGui::Text("Number of Iterations");
          if (ImGui::SliderInt("##Iterations", &m_state.cellular.iterations, 0, 20)) {
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }

          break;

        case GeneratorType::DrunkardMarch:
          m_state.currentGenerator = &m_state.march;

          ImGui::Text("Fill Percentage Goal");
          if (ImGui::SliderFloat("##FillPercentageGoal", &m_state.march.percentGoal, 0.0f, 1.0f, "%.2f")) {
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }

          ImGui::Text("Weight for Center");
          if (ImGui::SliderFloat("##WeightForCenter", &m_state.march.weightForCenter, 0.0f, 1.0f, "%.2f")) {
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }

          ImGui::Text("Weight for Previous Direction");
          if (ImGui::SliderFloat("##WeightForPreviousDirection", &m_state.march.weightForPreviousDirection, 0.0f, 1.0f, "%.2f")) {
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }

          break;

        case GeneratorType::Tunneling:
          m_state.currentGenerator = &m_state.tunneling;

          ImGui::Text("Maximum Number of Rooms");
          if (ImGui::SliderInt("##MaximumNumberOfRooms", &m_state.tunneling.maxRooms, 2, 100)) {
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }

          ImGui::Text("Minimum Size of Rooms");
          if (ImGui::SliderInt("##TunMinimumSizeOfRooms", &m_state.tunneling.roomSizeMinimum, 2, half)) {
            ensureGreater(m_state.tunneling.roomSizeMaximum, m_state.tunneling.roomSizeMinimum);
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }
          m_state.tunneling.roomSizeMinimum = gf::clamp(m_state.tunneling.roomSizeMinimum, 2, half);

          ImGui::Text("Maximum Size of Rooms");
          if (ImGui::SliderInt("##TunMaximumSizeOfRooms", &m_state.tunneling.roomSizeMaximum, 2, half)) {
            ensureLess(m_state.tunneling.roomSizeMinimum, m_state.tunneling.roomSizeMaximum);
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }
          m_state.tunneling.roomSizeMaximum = gf::clamp(m_state.tunneling.roomSizeMaximum, 2, half);

          break;

        case GeneratorType::BinarySpacePartitioning:
          m_state.currentGenerator = &m_state.bsp;

          ImGui::Text("Minimum Size of Leafs");
          if (ImGui::SliderInt("##MinimumSizeOfLeafs", &m_state.bsp.leafSizeMinimum, 3, half - 1)) {
            ensureLess(m_state.bsp.roomSizeMinimum, m_state.bsp.leafSizeMinimum - 1);
            ensureGreater(m_state.bsp.roomSizeMaximum, m_state.bsp.leafSizeMinimum);
            ensureGreater(m_state.bsp.leafSizeMaximum, m_state.bsp.roomSizeMaximum + 1);
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }
          m_state.bsp.leafSizeMinimum = gf::clamp(m_state.bsp.leafSizeMinimum, 3, half - 1);

          ImGui::Text("Maximum Size of Leafs");
          if (ImGui::SliderInt("##MaximumSizeOfLeafs", &m_state.bsp.leafSizeMaximum, 4, half)) {
            ensureLess(m_state.bsp.roomSizeMaximum, m_state.bsp.leafSizeMaximum - 1);
            ensureLess(m_state.bsp.leafSizeMinimum, m_state.bsp.roomSizeMaximum);
            ensureLess(m_state.bsp.roomSizeMinimum, m_state.bsp.leafSizeMinimum - 1);
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }
          m_state.bsp.leafSizeMaximum = gf::clamp(m_state.bsp.leafSizeMaximum, 4, half);

          ImGui::Text("Minimum Size of Rooms");
          if (ImGui::SliderInt("##BspMinimumSizeOfRooms", &m_state.bsp.roomSizeMinimum, 2, half - 2)) {
            ensureGreater(m_state.bsp.leafSizeMinimum, m_state.bsp.roomSizeMinimum + 1);
            ensureGreater(m_state.bsp.roomSizeMaximum, m_state.bsp.leafSizeMinimum);
            ensureGreater(m_state.bsp.leafSizeMaximum, m_state.bsp.roomSizeMaximum + 1);
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }
          m_state.bsp.roomSizeMinimum = gf::clamp(m_state.bsp.roomSizeMinimum, 2, half - 2);

          ImGui::Text("Maximum Size of Rooms");
          if (ImGui::SliderInt("##BspMaximumSizeOfRooms", &m_state.bsp.roomSizeMaximum, 3, half - 1)) {
            ensureGreater(m_state.bsp.leafSizeMaximum, m_state.bsp.roomSizeMaximum + 1);
            ensureLess(m_state.bsp.leafSizeMinimum, m_state.bsp.roomSizeMaximum);
            ensureLess(m_state.bsp.roomSizeMinimum, m_state.bsp.leafSizeMinimum - 1);
            m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Iterate);
          }
          m_state.bsp.roomSizeMaximum = gf::clamp(m_state.bsp.roomSizeMaximum, 3, half - 1);

          break;

      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (ImGui::Button("Generate")) {
        m_state.currentGenerator->setPhase(DungeonGenerator::Phase::Start);
      }

      m_state.updateDisplayWith(generator);
    }

    ImGui::End();

  }

}
