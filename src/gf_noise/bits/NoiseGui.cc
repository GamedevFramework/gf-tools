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
#include "NoiseGui.h"

#include <gf/Clock.h>
#include <gf/Noises.h>

#include <imgui.h>

#include "NoiseState.h"

namespace gftools {

  namespace {

    constexpr ImGuiWindowFlags DefaultWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    constexpr const char *NoiseList[] = { "Value", "Gradient", "Better Gradient", "Simplex", "OpenSimplex", "Worley", "Wavelet" };
    enum class NoiseFunction : int {
      Value           = 0,
      Gradient        = 1,
      BetterGradient  = 2,
      Simplex         = 3,
      OpenSimplex     = 4,
      Worley          = 5,
      Wavelet         = 6,
    };


    constexpr const char *StepList[] = { "Linear", "Cubic", "Quintic", "Cosine" };
    enum class StepFunction : int {
      Linear  = 0,
      Cubic   = 1,
      Quintic = 2,
      Cosine  = 3,
    };

    gf::Step<double> getStepFunction(StepFunction func) {
      switch (func) {
        case StepFunction::Linear:
          return gf::linearStep;
        case StepFunction::Cubic:
          return gf::cubicStep;
        case StepFunction::Quintic:
          return gf::quinticStep;
        case StepFunction::Cosine:
          return gf::cosineStep;
      }

      assert(false);
      return gf::linearStep;
    }


    constexpr const char *DistanceList[] = { "Euclidean", "Manhattan", "Chebyshev", "Natural" };
    enum class DistanceFunction : int {
      Euclidean = 0,
      Manhattan = 1,
      Chebyshev = 2,
      Natural   = 3,
    };

    gf::Distance2<double> getDistanceFunction(DistanceFunction func) {
      switch (func) {
        case DistanceFunction::Euclidean:
          return gf::squareDistance;
        case DistanceFunction::Manhattan:
          return gf::manhattanDistance;
        case DistanceFunction::Chebyshev:
          return gf::chebyshevDistance;
        case DistanceFunction::Natural:
          return gf::naturalDistance;
      }

      assert(false);
      return gf::squareDistance;
    }

    constexpr const char *CombinationList[] = { "F1", "F2", "F2F1" };
    enum class CombinationFunction : std::size_t {
      F1    = 0,
      F2    = 1,
      F2F1  = 2,
    };

    std::vector<double> getCombinationVector(CombinationFunction func) {
      switch (func) {
        case CombinationFunction::F1:
          return { 1.0 };
        case CombinationFunction::F2:
          return { 0.0, 1.0 };
        case CombinationFunction::F2F1:
          return { -1.0, 1.0 };
      }

      assert(false);
      return { 1.0 };
    }


    constexpr const char *FractalList[] = { "None", "fBm", "Multifractal", "Hetero Terrain", "Hybrid Multifractal", "Ridged Multifractal" };
    constexpr const char *RenderingList[] = { "Grayscale", "Colored" };


  }

  NoiseGui::NoiseGui(NoiseState& state)
  : m_state(state)
  , m_noiseChoice(1)
  , m_stepChoice(2)
  , m_pointCount(20)
  , m_distanceChoice(0)
  , m_combinationChoice(2)
  , m_fractalChoice(0)
  , m_renderingChoice(0)
  {
  }

  void NoiseGui::render(gf::RenderTarget& target, const gf::RenderStates& states) {

    if (ImGui::Begin("Noise parameters", nullptr, DefaultWindowFlags)) {

      ImGui::Combo("##Noise", &m_noiseChoice, NoiseList, IM_ARRAYSIZE(NoiseList));
      NoiseFunction noiseFunction = static_cast<NoiseFunction>(m_noiseChoice);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::Text("Scale:");
      ImGui::SliderFloat("##Scale", &m_state.scale, 0.1f, 20.0f);

      switch (noiseFunction) {
        case NoiseFunction::Value:
          ImGui::Text("Step function:");
          ImGui::Combo("##Step", &m_stepChoice, StepList, IM_ARRAYSIZE(StepList));
          break;

        case NoiseFunction::Gradient:
          ImGui::Text("Step function:");
          ImGui::Combo("##Step", &m_stepChoice, StepList, IM_ARRAYSIZE(StepList));
          break;

        case NoiseFunction::Worley:
          ImGui::Text("Point count:");
          ImGui::SliderInt("##PointCount", &m_pointCount, 5, 40);
          ImGui::Text("Distance function:");
          ImGui::Combo("##Distance", &m_distanceChoice, DistanceList, IM_ARRAYSIZE(DistanceList));
          ImGui::Text("Combination:");
          ImGui::Combo("##Combination", &m_combinationChoice, CombinationList, IM_ARRAYSIZE(CombinationList));
          break;

        default:
          break;
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (ImGui::TreeNode("Fractal")) {
        ImGui::Combo("##Fractal", &m_fractalChoice, FractalList, IM_ARRAYSIZE(FractalList));
        m_state.fractal.type = static_cast<FractalType>(m_fractalChoice);

        if (m_state.fractal.type != FractalType::None) {
          switch (m_state.fractal.type) {
            case FractalType::None:
              assert(false);
              break;

            case FractalType::FBm:
              ImGui::Text("Dimension:");
              ImGui::SliderFloat("##F_Dimension", &m_state.fractal.f.dimension, 0.1f, 1.9f);
              break;

            case FractalType::Multifractal:
              ImGui::Text("Dimension:");
              ImGui::SliderFloat("##M_Dimension", &m_state.fractal.m.dimension, 0.1f, 1.9f);
              break;

            case FractalType::HeteroTerrain:
              ImGui::Text("Dimension:");
              ImGui::SliderFloat("##HT_Dimension", &m_state.fractal.ht.dimension, 0.1f, 1.9f);
              ImGui::Text("Offset:");
              ImGui::SliderFloat("##HT_Offset", &m_state.fractal.ht.offset, 0.0f, 10.0f);
              break;

            case FractalType::HybridMultifractal:
              ImGui::Text("Dimension:");
              ImGui::SliderFloat("##HM_Dimension", &m_state.fractal.hm.dimension, 0.1f, 1.9f);
              ImGui::Text("Offset:");
              ImGui::SliderFloat("##HM_Offset", &m_state.fractal.hm.offset, 0.0f, 10.0f);
              break;

            case FractalType::RidgedMultifractal:
              ImGui::Text("Dimension:");
              ImGui::SliderFloat("##RM_Dimension", &m_state.fractal.rm.dimension, 0.1f, 1.9f);
              ImGui::Text("Offset:");
              ImGui::SliderFloat("##RM_Offset", &m_state.fractal.rm.offset, 0.0f, 10.0f);
              ImGui::Text("Gain:");
              ImGui::SliderFloat("##RM_Gain", &m_state.fractal.rm.gain, 1.0f, 3.0f);
              break;
          }

          ImGui::Separator();

          ImGui::Text("Octaves:");
          ImGui::SliderInt("##Octaves", &m_state.fractal.octaves, 1, 15);
          ImGui::Text("Lacunarity:");
          ImGui::SliderFloat("##Lacunarity", &m_state.fractal.lacunarity, 1.0f, 3.0f);
          ImGui::Text("Persistence:");
          ImGui::SliderFloat("##Persistence", &m_state.fractal.persistence, 0.1f, 0.9f);
        }

        ImGui::TreePop();
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (ImGui::TreeNode("Rendering")) {
        ImGui::Combo("##Rendering", &m_renderingChoice, RenderingList, IM_ARRAYSIZE(RenderingList));
        m_state.rendering.type = static_cast<RenderingType>(m_renderingChoice);

        switch (m_state.rendering.type) {
          case RenderingType::Grayscale:
            break;

          case RenderingType::Colored:
            ImGui::Text("Water level:");
            ImGui::SliderFloat("##WaterLevel", &m_state.rendering.colored.waterLevel, 0.0f, 1.0f);
            ImGui::Checkbox("Shaded", &m_state.rendering.colored.shaded);
            break;
        }

        ImGui::TreePop();
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (ImGui::Button("Generate")) {
        gf::Clock clock;

        switch (noiseFunction) {
          case NoiseFunction::Value: {
            StepFunction stepFunction = static_cast<StepFunction>(m_stepChoice);
            gf::Step<double> step = getStepFunction(stepFunction);

            gf::ValueNoise2D noise(m_state.random, step);
            m_state.generateImage(noise);
            break;
          }

          case NoiseFunction::Gradient: {
            StepFunction stepFunction = static_cast<StepFunction>(m_stepChoice);
            gf::Step<double> step = getStepFunction(stepFunction);

            gf::GradientNoise2D noise(m_state.random, step);
            m_state.generateImage(noise);
            break;
          }

          case NoiseFunction::BetterGradient: {
            gf::BetterGradientNoise2D noise(m_state.random);
            m_state.generateImage(noise);
            break;
          }

          case NoiseFunction::Simplex: {
            gf::SimplexNoise2D noise(m_state.random);
            m_state.generateImage(noise);
            break;
          }

          case NoiseFunction::OpenSimplex: {
            gf::OpenSimplexNoise2D noise(m_state.random);
            m_state.generateImage(noise);
            break;
          }

          case NoiseFunction::Worley: {
            DistanceFunction distanceFunction = static_cast<DistanceFunction>(m_distanceChoice);
            gf::Distance2<double> distance = getDistanceFunction(distanceFunction);

            CombinationFunction combinationFunction = static_cast<CombinationFunction>(m_combinationChoice);
            std::vector<double> combination = getCombinationVector(combinationFunction);

            gf::WorleyNoise2D noise(m_state.random, m_pointCount, distance, combination);
            m_state.generateImage(noise);
            break;
          }

          case NoiseFunction::Wavelet: {
            gf::WaveletNoise3D noise(m_state.random);
            gf::Noise3DTo2DAdapter adapter(noise);
            m_state.generateImage(adapter);
            break;
          }
        }

        gf::Time duration = clock.getElapsedTime();
        m_feedback = "Generation time: " + std::to_string(duration.asMilliseconds()) + " ms";
      }

      ImGui::SameLine();

      if (ImGui::Button("Save to 'noise.png'")) {
        m_state.image.saveToFile("noise.png");
      }

      if (!m_feedback.empty()) {
        ImGui::Text("%s", m_feedback.c_str());
      }

    }

    ImGui::End();
  }


}
