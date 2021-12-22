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
#include "TilesetGui.h"

#include <cassert>
#include <algorithm>
#include <fstream>

#include <imgui.h>

#include <gf/Color.h>
#include <gf/Log.h>
#include <gf/RenderTarget.h>

#include "TilesetData.h"
#include "TilesetProcess.h"

namespace gftools {

  namespace {

    constexpr int BottomMargin = 130; // TODO: find something better?
    constexpr float EmptySize = 22.0f;

    constexpr const char *PigmentStyleList[] = { "Plain", "Randomize", "Striped", "Paved" }; // see PigmentStyle
    constexpr const char *BorderEffectList[] = { "None", "Fade", "Outline", "Sharpen", "Lighten", "Blur", "Blend" }; // see BorderEffect


    bool AtomCombo(const TilesetData& data, const char *label, gf::Id *current, std::initializer_list<gf::Id> forbidden) {
      auto currentAtom = data.getAtom(*current);

      bool res = ImGui::BeginCombo(label, currentAtom.id.name.c_str());

      if (res) {
        res = false;

        for (auto& atom : data.atoms) {
          ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;

          if (std::find(forbidden.begin(), forbidden.end(), atom.id.hash) != forbidden.end()) {
            flags |= ImGuiSelectableFlags_Disabled;
          };

          ImGui::ColorButton("##Color", ImVec4(atom.color.r, atom.color.g, atom.color.b, atom.color.a));
          ImGui::SameLine();

          if (ImGui::Selectable(atom.id.name.c_str(), atom.id.hash == *current, flags)) {
            *current = atom.id.hash;
            res = true;
          }
        }

        ImGui::EndCombo();
      }

      ImGui::SameLine();
      ImGui::ColorButton("##Color", ImVec4(currentAtom.color.r, currentAtom.color.g, currentAtom.color.b, currentAtom.color.a));

      return res;
    }
  }

  TilesetGui::TilesetGui(gf::Path datafile, TilesetData& data, gf::Random& random)
  : m_datafile(std::move(datafile))
  , m_data(data)
  , m_random(random)
  , m_size(m_data.settings.getImageSize())
  {
  }

  void TilesetGui::render(gf::RenderTarget& target, const gf::RenderStates& states) {
    auto size = target.getSize();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(size.width, size.height));

    if (ImGui::Begin("Tileset", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {

      if (ImGui::BeginTabBar("##Tabs")) {
        if (ImGui::BeginTabItem("Settings")) {

          if (ImGui::Checkbox("Locked", &m_data.settings.locked)) {
            m_modified = true;
          }

          ImGui::Separator();

          static constexpr int InputSlowStep = 16;
          static constexpr int InputFastStep = 64;

          if (ImGui::InputInt("TileSize", &m_data.settings.tile.size, InputSlowStep, InputFastStep)) {
            m_size = m_data.settings.getImageSize();
            m_modified = true;
          }

          if (ImGui::InputInt("TileSpacing", &m_data.settings.tile.spacing, 1, 2)) {
            m_size = m_data.settings.getImageSize();
            m_modified = true;
          }

          if (!m_data.settings.locked) {
            ImGui::Separator();

            if (ImGui::InputInt("Max Atom Count", &m_data.settings.maxAtomCount, InputSlowStep, InputFastStep)) {
              m_size = m_data.settings.getImageSize();
              m_modified = true;
            }

            if (ImGui::InputInt("Max Wang2 Count", &m_data.settings.maxWang2Count, InputSlowStep, InputFastStep)) {
              m_size = m_data.settings.getImageSize();
              m_modified = true;
            }

            if (ImGui::InputInt("Max Wang3 Count", &m_data.settings.maxWang3Count, InputSlowStep, InputFastStep)) {
              m_size = m_data.settings.getImageSize();
              m_modified = true;
            }
          }

          ImGui::Text("Image size: %ix%i", m_size.width, m_size.height);

          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Atoms")) {
          ImGui::Text("Atom count: %zu/%i", m_data.atoms.size(), m_data.settings.maxAtomCount);
          ImGui::Spacing();

          if (ImGui::BeginChild("##AtomChild", ImVec2(0, size.height - BottomMargin))) {

            if (ImGui::BeginTable("##AtomTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
              ImGui::TableSetupColumn("Atom");
              ImGui::TableSetupColumn("Operations", ImGuiTableColumnFlags_WidthFixed);


              ImGui::TableHeadersRow();

              std::size_t index = 0;
              gf::Id atomToDelete = gf::InvalidId;

              for (auto& atom : m_data.atoms) {
                ImGui::TableNextColumn();

                ImGui::PushID(index);
                ImGui::ColorButton("##Color", ImVec4(atom.color.r, atom.color.g, atom.color.b, atom.color.a));
                ImGui::SameLine();
                ImGui::Text("%s", atom.id.name.c_str());

                ImGui::TableNextColumn();

                if (!m_data.settings.locked && index + 1 < m_data.atoms.size()) {
                  if (ImGui::ArrowButton("Down", ImGuiDir_Down)) {
                    std::swap(m_data.atoms[index], m_data.atoms[index + 1]);
                    m_modified = true;
                  }
                } else {
                  ImGui::Dummy(ImVec2(EmptySize, EmptySize));
                }

                ImGui::SameLine();

                if (!m_data.settings.locked && index > 0) {
                  if (ImGui::ArrowButton("Up", ImGuiDir_Up)) {
                    std::swap(m_data.atoms[index], m_data.atoms[index - 1]);
                    m_modified = true;
                  }
                } else {
                  ImGui::Dummy(ImVec2(EmptySize, EmptySize));
                }

                ImGui::SameLine();

                auto prepareEditedAtom = [&]() {
                  m_editedAtom = atom;
                  assert(m_editedAtom.id.name.size() < NameBufferSize - 1);
                  std::copy(m_editedAtom.id.name.begin(), m_editedAtom.id.name.end(), std::begin(m_nameBuffer));
                  m_nameBuffer[m_editedAtom.id.name.size()] = '\0';
                  m_pigmentChoice = static_cast<int>(m_editedAtom.pigment.style);
                };

                auto generatePreview = [this]() {
                  m_data.temporary.atom = m_editedAtom;
                  m_pigmentPreview = gf::Texture(generateAtomPreview(m_editedAtom, m_random, m_data.settings.tile));
                  m_data.temporary.atom = Atom();
                };

                if (ImGui::Button("Edit")) {
                  ImGui::OpenPopup("Edit");
                  prepareEditedAtom();
                  generatePreview();
                }

                if (m_newAtom && index + 1 == m_data.atoms.size()) {
                  ImGui::SetScrollHereY(1.0f);
                  ImGui::OpenPopup("Edit");
                  prepareEditedAtom();
                  generatePreview();
                  m_newAtom = false;
                }

                if (ImGui::BeginPopupModal("Edit", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                  bool changed = false;

                  ImGui::InputText("Name##Atom", m_nameBuffer, NameBufferSize);
                  changed = ImGui::ColorEdit4("Color##Atom", m_editedAtom.color.begin()) || changed;

                  if (ImGui::Combo("Pigment##PigmentStyle", &m_pigmentChoice, PigmentStyleList, IM_ARRAYSIZE(PigmentStyleList))) {
                    m_editedAtom.pigment.style = static_cast<PigmentStyle>(m_pigmentChoice);

                    switch (m_editedAtom.pigment.style) {
                      case PigmentStyle::Plain:
                        break;
                      case PigmentStyle::Randomize:
                        m_editedAtom.pigment.randomize.ratio = 0.1f;
                        m_editedAtom.pigment.randomize.deviation = 0.1f;
                        m_editedAtom.pigment.randomize.size = 1;
                        break;
                      case PigmentStyle::Striped:
                        m_editedAtom.pigment.striped.width = 3;
                        m_editedAtom.pigment.striped.stride = 8;
                        break;
                      case PigmentStyle::Paved:
                        m_editedAtom.pigment.paved.width = 8;
                        m_editedAtom.pigment.paved.length = 16;
                        m_editedAtom.pigment.paved.modulation = 0.5f;
                        break;
                    }

                    changed = true;
                  }

                  ImGui::Indent();

                  switch (m_editedAtom.pigment.style) {
                    case PigmentStyle::Plain:
                      break;
                    case PigmentStyle::Randomize:
                      changed = ImGui::SliderFloat("Ratio##AtomRandomize", &m_editedAtom.pigment.randomize.ratio, 0.0f, 1.0f, "%.2f") || changed;
                      changed = ImGui::SliderFloat("Deviation##AtomRandomize", &m_editedAtom.pigment.randomize.deviation, 0.0f, 0.5f, "%.2f") || changed;
                      changed = ImGui::SliderInt("Size##AtomRandomize", &m_editedAtom.pigment.randomize.size, 1, 5) || changed;
                      break;
                    case PigmentStyle::Striped:
                      changed = ImGui::SliderInt("Width##AtomStriped", &m_editedAtom.pigment.striped.width, 1, 8) || changed;
                      changed = ImGui::SliderInt("Stride##AtomStriped", &m_editedAtom.pigment.striped.stride, 1, 16) || changed;
                      break;
                    case PigmentStyle::Paved:
                      changed = ImGui::SliderInt("Width##AtomPaved", &m_editedAtom.pigment.paved.width, 4, 16) || changed;
                      changed = ImGui::SliderInt("Length##AtomPaved", &m_editedAtom.pigment.paved.length, 4, 32) || changed;
                      changed = ImGui::SliderFloat("Modulation##AtomPaved", &m_editedAtom.pigment.paved.modulation, -0.8f, 0.8f, "%.2f") || changed;
                      break;
                  }

                  ImGui::Unindent();

                  ImGui::Spacing();

                  static constexpr float PreviewSize = 128.0f;

                  if (changed) {
                    generatePreview();
                  }

                  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - PreviewSize) / 2);
                  ImGui::Image(static_cast<void*>(&m_pigmentPreview), ImVec2(PreviewSize, PreviewSize));

                  ImGui::Spacing();


                  if (ImGui::Button("Save")) {
                    m_editedAtom.id.name = m_nameBuffer;
                    m_editedAtom.id.hash = gf::hash(m_editedAtom.id.name);
                    m_data.updateAtom(atom, m_editedAtom);
                    ImGui::CloseCurrentPopup();
                    m_modified = true;
                  }

                  ImGui::SameLine();

                  if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                  }

                  ImGui::SameLine();

                  if (ImGui::Button("Preview")) {
                    generatePreview();
                  }

                  ImGui::EndPopup();
                }

                ImGui::SameLine();

                if (m_data.settings.locked) {
                  ImGui::TextDisabled("Delete");
                } else {
                  if (ImGui::Button("Delete")) {
                    ImGui::OpenPopup("Delete");
                  }
                }

                if (ImGui::BeginPopupModal("Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                  ImGui::Text("Are you sure you want to delete '%s'?", m_data.atoms[index].id.name.c_str());

                  if (ImGui::Button("No, do not delete!")) {
                    ImGui::CloseCurrentPopup();
                  }

                  ImGui::SameLine();

                  if (ImGui::Button("Yes, I want to delete")) {
                    m_data.atoms.erase(m_data.atoms.begin() + index);
                    ImGui::CloseCurrentPopup();
                    m_modified = true;
                  }

                  ImGui::EndPopup();
                }

                ImGui::PopID();
                ++index;
              }

              ImGui::EndTable();

              if (atomToDelete != gf::InvalidId) {
                m_data.deleteAtom(atomToDelete);
              }
            }

          }

          ImGui::EndChild();

          if (ImGui::Button("New")) {
            Atom atom;
            atom.id.name = "NewAtom"; atom.id.hash = gf::hash(atom.id.name);
            atom.color = gf::Color::White;
            atom.pigment.style = PigmentStyle::Plain;
            m_data.atoms.emplace_back(std::move(atom));
            m_newAtom = true;
            m_modified = true;
          }


          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Wang2")) {
          ImGui::Text("Wang2 count: %zu/%i", m_data.wang2.size(), m_data.settings.maxWang2Count);
          ImGui::Spacing();

          if (ImGui::BeginChild("##Wang2", ImVec2(0, size.height - BottomMargin))) {

            if (ImGui::BeginTable("##AtomTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
              ImGui::TableSetupColumn("Atom #1");
              ImGui::TableSetupColumn("Atom #2");
              ImGui::TableSetupColumn("Operations", ImGuiTableColumnFlags_WidthFixed);

              ImGui::TableHeadersRow();

              std::size_t index = 0;

              for (auto& wang : m_data.wang2) {
                ImGui::TableNextColumn();

                ImGui::PushID(index);

                for (auto& border : wang.borders) {
                  if (border.id.hash == Void) {
                    ImGui::ColorButton("##Color", ImVec4(0, 0, 0, 0), ImGuiColorEditFlags_AlphaPreview);
                    ImGui::SameLine();
                    ImGui::Text("-");
                    ImGui::TableNextColumn();
                  } else {
                    auto atom = m_data.getAtom(border.id.hash);
                    ImGui::ColorButton("##Color", ImVec4(atom.color.r, atom.color.g, atom.color.b, atom.color.a));
                    ImGui::SameLine();
                    ImGui::Text("%s", atom.id.name.c_str());
                    ImGui::TableNextColumn();
                  }
                }

                if (!m_data.settings.locked && index + 1 < m_data.wang2.size()) {
                  if (ImGui::ArrowButton("Down", ImGuiDir_Down)) {
                    std::swap(m_data.wang2[index], m_data.wang2[index + 1]);
                    m_modified = true;
                  }
                } else {
                  ImGui::Dummy(ImVec2(EmptySize, EmptySize));
                }

                ImGui::SameLine();

                if (!m_data.settings.locked && index > 0) {
                  if (ImGui::ArrowButton("Up", ImGuiDir_Up)) {
                    std::swap(m_data.wang2[index], m_data.wang2[index - 1]);
                    m_modified = true;
                  }
                } else {
                  ImGui::Dummy(ImVec2(EmptySize, EmptySize));
                }

                ImGui::SameLine();

                auto prepareEditedWang2 = [&]() {
                  m_editedWang2 = wang;
                  m_borderEffectChoices[0] = static_cast<int>(m_editedWang2.borders[0].effect);
                  m_borderEffectChoices[1] = static_cast<int>(m_editedWang2.borders[1].effect);
                };

                auto generatePreview = [this]() {
                  m_data.temporary.wang2 = m_editedWang2;
                  m_wang2Preview = gf::Texture(generateWang2Preview(m_editedWang2, m_random, m_data));
//                   m_wang2Preview.setSmooth();
                  m_data.temporary.wang2 = Wang2();
                };

                if (ImGui::Button("Edit")) {
                  ImGui::OpenPopup("Edit");
                  prepareEditedWang2();
                  generatePreview();
                }

                if (m_newWang2 && index + 1 == m_data.wang2.size()) {
                  ImGui::SetScrollHereY(1.0f);
                  ImGui::OpenPopup("Edit");
                  prepareEditedWang2();
                  generatePreview();
                  m_newWang2 = false;
                }

                if (ImGui::BeginPopupModal("Edit", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                  bool changed = false;
                  int j = 0;

                  for (auto& border : m_editedWang2.borders) {
                    ImGui::PushID(j);

                    ImGui::Text("Atom #%i\n", j+1);

                    if (j == 1) {

                      if (ImGui::RadioButton("Overlay##Wang2", border.id.hash == Void)) {
                        changed = true;

                        if (border.id.hash == Void) {
                          border.id = m_data.atoms.front().id;
                          border.effect = BorderEffect::None;
                        } else {
                          border.id.name = "Void";
                          border.id.hash = Void;
                          border.effect = BorderEffect::None;
                        }
                      }
                    }

                    if (border.id.hash != Void) {
                      if (AtomCombo(m_data, "##Wang2Atom", &border.id.hash, { m_editedWang2.borders[1 - j].id.hash })) {
                        changed = true;
                        auto atom = m_data.getAtom(border.id.hash);
                        border.id = atom.id;
                      }

                      if (ImGui::Combo("Border##BorderEffect", &m_borderEffectChoices[j], BorderEffectList, IM_ARRAYSIZE(BorderEffectList))) {
                        changed = true;

                        border.effect = static_cast<BorderEffect>(m_borderEffectChoices[j]);

                        switch (border.effect) {
                          case BorderEffect::None:
                            break;
                          case BorderEffect::Fade:
                            border.fade.distance = 11;
                            break;
                          case BorderEffect::Outline:
                            border.outline.distance = 6;
                            border.outline.factor = 0.2f;
                            break;
                          case BorderEffect::Sharpen:
                            border.sharpen.distance = 6;
                            break;
                          case BorderEffect::Lighten:
                            border.lighten.distance = 6;
                            break;
                          case BorderEffect::Blur:
                            break;
                          case BorderEffect::Blend:
                            border.blend.distance = 5;
                            break;
                        }
                      }

                      ImGui::Indent();

                      switch (border.effect) {
                        case BorderEffect::None:
                          break;
                        case BorderEffect::Fade:
                          changed = ImGui::SliderInt("Distance##Wang2Fade", &border.fade.distance, 1, m_data.settings.tile.size / 2) || changed;
                          break;
                        case BorderEffect::Outline:
                          changed = ImGui::SliderInt("Distance##Wang2Outline", &border.outline.distance, 1, m_data.settings.tile.size / 2) || changed;
                          changed = ImGui::SliderFloat("Factor##Wang2Outline", &border.outline.factor, 0.0f, 1.0f, "%.2f") || changed;
                          break;
                        case BorderEffect::Sharpen:
                          changed = ImGui::SliderInt("Distance##Wang2Sharpen", &border.sharpen.distance, 1, m_data.settings.tile.size / 2) || changed;
                          break;
                        case BorderEffect::Lighten:
                          changed = ImGui::SliderInt("Distance##Wang2Lighten", &border.lighten.distance, 1, m_data.settings.tile.size / 2) || changed;
                          break;
                        case BorderEffect::Blur:
                          break;
                        case BorderEffect::Blend:
                          changed = ImGui::SliderInt("Distance##Wang2Blend", &border.blend.distance, 1, m_data.settings.tile.size / 2) || changed;
                          break;
                      }

                      ImGui::Unindent();

                    }

                    ImGui::Separator();

                    ImGui::PopID();
                    ++j;
                  }

                  ImGui::Spacing();

                  changed = ImGui::SliderInt("Offset##Wang2Offset", &m_editedWang2.edge.offset, -m_data.settings.tile.size / 4, m_data.settings.tile.size / 4) || changed;

                  if (ImGui::RadioButton("Limit##Wang2Limit", m_editedWang2.edge.limit)) {
                    m_editedWang2.edge.limit = !m_editedWang2.edge.limit;
                  }

                  ImGui::Spacing();

                  changed = ImGui::SliderInt("Iterations##Wang2Iterations", &m_editedWang2.edge.displacement.iterations, 0, 5) || changed;
                  changed = ImGui::SliderFloat("Initial factor##Wang2Initial", &m_editedWang2.edge.displacement.initial, 0.1f, 1.0f, "%.2f") || changed;
                  changed = ImGui::SliderFloat("Reduction factor##Wang2Reduction", &m_editedWang2.edge.displacement.reduction, 0.1f, 1.0f, "%.2f") || changed;

                  ImGui::Spacing();

                  static constexpr float PreviewSize = (128.0f + 3.0f) * 2;

                  if (changed) {
                    generatePreview();
                  }

                  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - PreviewSize) / 2);
                  ImGui::Image(static_cast<void*>(&m_wang2Preview), ImVec2(PreviewSize, PreviewSize));

                  ImGui::Spacing();

                  if (ImGui::Button("Save")) {
                    wang = m_editedWang2;
                    ImGui::CloseCurrentPopup();
                    m_modified = true;
                  }

                  ImGui::SameLine();

                  if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                  }

                  ImGui::SameLine();

                  if (ImGui::Button("Preview")) {
                    generatePreview();
                  }

                  ImGui::EndPopup();
                }

                ImGui::SameLine();

                if (m_data.settings.locked) {
                  ImGui::TextDisabled("Delete");
                } else {
                  if (ImGui::Button("Delete")) {
                    ImGui::OpenPopup("Delete");
                  }
                }

                if (ImGui::BeginPopupModal("Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                  ImGui::Text("Are you sure you want to delete this?");

                  if (ImGui::Button("No, do not delete!")) {
                    ImGui::CloseCurrentPopup();
                  }

                  ImGui::SameLine();

                  if (ImGui::Button("Yes, I want to delete")) {
                    m_data.wang2.erase(m_data.wang2.begin() + index);
                    ImGui::CloseCurrentPopup();
                    m_modified = true;
                  }

                  ImGui::EndPopup();
                }

                ImGui::PopID();
                ++index;
              }

              ImGui::EndTable();
            }

          }

          ImGui::EndChild();

          if (ImGui::Button("New")) {
            assert(m_data.atoms.size() >= 2);
            Wang2 wang;
            wang.borders[0].id = m_data.atoms[0].id;
            wang.borders[0].effect = BorderEffect::None;
            wang.borders[1].id = m_data.atoms[1].id;
            wang.borders[1].effect = BorderEffect::None;
            m_data.wang2.emplace_back(std::move(wang));
            m_newWang2 = true;
            m_modified = true;
          }

          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Wang3")) {
          ImGui::Text("Wang3 count: %zu/%i", m_data.wang3.size(), m_data.settings.maxWang3Count);
          ImGui::Spacing();

          if (ImGui::BeginChild("##Wang3", ImVec2(0, size.height - BottomMargin))) {

            if (ImGui::BeginTable("##AtomTable", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
              ImGui::TableSetupColumn("Atom #1");
              ImGui::TableSetupColumn("Atom #2");
              ImGui::TableSetupColumn("Atom #3");
              ImGui::TableSetupColumn("Operations", ImGuiTableColumnFlags_WidthFixed);

              ImGui::TableHeadersRow();

              std::size_t index = 0;

              for (auto& wang : m_data.wang3) {
                ImGui::TableNextColumn();

                ImGui::PushID(index);

                for (auto& id : wang.ids) {
                  if (id.hash == Void) {
                    ImGui::ColorButton("##Color", ImVec4(0, 0, 0, 0), ImGuiColorEditFlags_AlphaPreview);
                    ImGui::SameLine();
                    ImGui::Text("-");
                    ImGui::TableNextColumn();
                  } else {
                    auto atom = m_data.getAtom(id.hash);
                    ImGui::ColorButton("##Color", ImVec4(atom.color.r, atom.color.g, atom.color.b, atom.color.a));
                    ImGui::SameLine();
                    ImGui::Text("%s", atom.id.name.c_str());
                    ImGui::TableNextColumn();
                  }
                }

                if (!m_data.settings.locked && index + 1 < m_data.wang3.size()) {
                  if (ImGui::ArrowButton("Down", ImGuiDir_Down)) {
                    std::swap(m_data.wang3[index], m_data.wang3[index + 1]);
                    m_modified = true;
                  }
                } else {
                  ImGui::Dummy(ImVec2(EmptySize, EmptySize));
                }

                ImGui::SameLine();

                if (!m_data.settings.locked && index > 0) {
                  if (ImGui::ArrowButton("Up", ImGuiDir_Up)) {
                    std::swap(m_data.wang3[index], m_data.wang3[index - 1]);
                    m_modified = true;
                  }
                } else {
                  ImGui::Dummy(ImVec2(EmptySize, EmptySize));
                }

                ImGui::SameLine();

                auto prepareEditedWang3 = [&]() {
                  m_editedWang3 = wang;
                };

                auto generatePreview = [this]() {
                  m_wang3Preview = gf::Texture(generateWang3Preview(m_editedWang3, m_random, m_data));
//                   m_wang3Preview.setSmooth();
                };

                if (ImGui::Button("Edit")) {
                  ImGui::OpenPopup("Edit");
                  prepareEditedWang3();
                  generatePreview();
                }

                if (m_newWang3 && index + 1 == m_data.wang3.size()) {
                  ImGui::SetScrollHereY(1.0f);
                  ImGui::OpenPopup("Edit");
                  prepareEditedWang3();
                  generatePreview();
                  m_newWang3 = false;
                }

                if (ImGui::BeginPopupModal("Edit", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                  bool changed = false;
                  int j = 0;

                  for (auto& id : m_editedWang3.ids) {
                    ImGui::PushID(j);

                    ImGui::Text("Atom #%i\n", j+1);

                    if (j == 2) {
                      if (ImGui::RadioButton("Overlay##Wang3", id.hash == Void)) {
                        changed = true;

                        if (id.hash == Void) {
                          id = m_data.atoms.front().id;
                        } else {
                          id.name = "Void";
                          id.hash = Void;
                        }
                      }
                    }

                    if (id.hash != Void) {
                      if (AtomCombo(m_data, "##Wang3Atom", &id.hash, { m_editedWang3.ids[(j + 1) % 3].hash, m_editedWang3.ids[(j + 2) % 3].hash })) {
                        changed = true;
                        auto atom = m_data.getAtom(id.hash);
                        id = atom.id;
                      }
                    }

                    ImGui::Separator();

                    ImGui::PopID();
                    ++j;
                  }

                  ImGui::Spacing();

                  static constexpr float PreviewSize = (192.0f + 5.0f) * 2;

                  if (changed) {
                    generatePreview();
                  }

                  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - PreviewSize) / 2);
                  ImGui::Image(static_cast<void*>(&m_wang3Preview), ImVec2(PreviewSize, PreviewSize));

                  ImGui::Spacing();

                  if (ImGui::Button("Save")) {
                    // TODO: add missing wang2
                    wang = m_editedWang3;
                    ImGui::CloseCurrentPopup();
                    m_modified = true;
                  }

                  ImGui::SameLine();

                  if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                  }

                  ImGui::SameLine();

                  if (ImGui::Button("Preview")) {
                    generatePreview();
                  }

                  ImGui::EndPopup();
                }

                ImGui::SameLine();

                if (m_data.settings.locked) {
                  ImGui::TextDisabled("Delete");
                } else {
                  if (ImGui::Button("Delete")) {
                    ImGui::OpenPopup("Delete");
                  }
                }

                if (ImGui::BeginPopupModal("Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                  ImGui::Text("Are you sure you want to delete this?");

                  if (ImGui::Button("No, do not delete!")) {
                    ImGui::CloseCurrentPopup();
                  }

                  ImGui::SameLine();

                  if (ImGui::Button("Yes, I want to delete")) {
                    m_data.wang3.erase(m_data.wang3.begin() + index);
                    ImGui::CloseCurrentPopup();
                    m_modified = true;
                  }

                  ImGui::EndPopup();
                }

                ImGui::PopID();
                ++index;
              }

              ImGui::EndTable();

            }
          }

          ImGui::EndChild();

          if (ImGui::Button("New")) {
            assert(m_data.atoms.size() >= 3);
            Wang3 wang;
            wang.ids[0] = m_data.atoms[0].id;
            wang.ids[1] = m_data.atoms[1].id;
            wang.ids[2] = m_data.atoms[2].id;
            m_data.wang3.emplace_back(std::move(wang));
            m_newWang3 = true;
            m_modified = true;
          }

          ImGui::SameLine();

          if (m_data.settings.locked) {
            ImGui::TextDisabled("Generate");
          } else {
            if (ImGui::Button("Generate")) {
              m_data.generateAllWang3();
              m_modified = true;
            }
          }

          ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
      }

      ImGui::Separator();

      if (m_modified) {
        if (ImGui::Button("Save the current project")) {
          TilesetData::save(m_datafile, m_data);
          m_modified = false;
        }
      } else {
        ImGui::TextDisabled("Save the current project");
      }

      ImGui::SameLine();

      if (ImGui::Button("Export the tileset to TMX")) {
        auto tilesets = generateTilesets(m_random, m_data);
        auto image = generateTilesetImage(m_random, m_data, tilesets);
        auto imagePath = m_datafile.replace_extension(".png");
        image.saveToFile(imagePath);
        auto xml = generateTilesetXml(imagePath.filename(), m_data, tilesets);
        auto xmlPath = m_datafile.replace_extension(".tsx");
        std::ofstream(xmlPath.string()) << xml;
      }

    }

    ImGui::End();
  }


}
