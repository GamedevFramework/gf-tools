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
#ifndef DUNGEON_SCENE_H
#define DUNGEON_SCENE_H

#include <gf/Scene.h>
#include <gf/Views.h>

#include "DungeonDisplay.h"
#include "DungeonGui.h"

namespace gftools {

  struct DungeonApp;

  class DungeonScene : public gf::Scene {
  public:
    DungeonScene(DungeonApp& app);

  private:
    bool doEarlyProcessEvent(gf::Event& event) override;
    void doProcessEvent(gf::Event& event) override;
    void doUpdate(gf::Time time) override;
    void doRender(gf::RenderTarget& target, const gf::RenderStates &states) override;

  private:
    DungeonApp& m_app;

    gf::ZoomingViewAdaptor m_adaptor;

    DungeonDisplay m_display;
    DungeonGui m_gui;
  };

}


#endif // DUNGEON_SCENE_H
