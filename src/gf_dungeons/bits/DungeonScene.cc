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
#include "DungeonScene.h"

#include <gf/Color.h>

#include <imgui.h>
#include <imgui_impl_gf.h>

#include "DungeonApp.h"
#include "DungeonState.h"

namespace gftools {

  DungeonScene::DungeonScene(DungeonApp& app)
  : gf::Scene(app.getRenderer().getSize())
  , m_app(app)
  , m_adaptor(app.getRenderer(), getWorldView())
  , m_display(app.state)
  , m_gui(app.state)
  {
    setClearColor(gf::Color::Gray());

    float size = DungeonGenerator::CellSize * app.state.dungeonSize;
    setWorldViewSize(gf::vec(size, size));
    setWorldViewCenter(gf::vec(size, size) * 0.5f);

    addWorldEntity(m_display);

    addHudEntity(m_gui);
  }

  bool DungeonScene::doEarlyProcessEvent(gf::Event& event) {
    return ImGui_ImplGF_ProcessEvent(event);
  }

  void DungeonScene::doProcessEvent(gf::Event& event) {
    m_adaptor.processEvent(event);
  }

  void DungeonScene::doUpdate(gf::Time time) {
    ImGui_ImplGF_Update(time);
  }

  void DungeonScene::doRender(gf::RenderTarget& target, const gf::RenderStates &states) {
    ImGui::NewFrame();

    renderWorldEntities(target, states);
    renderHudEntities(target, states);

    ImGui::Render();
    ImGui_ImplGF_RenderDrawData(ImGui::GetDrawData());
  }

}
