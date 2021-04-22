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
#ifndef TILESET_SCENE_H
#define TILESET_SCENE_H

#include <gf/Scene.h>

#include "TilesetGui.h"

namespace gftools {

  struct TilesetApp;

  class TilesetScene : public gf::Scene {
  public:
    TilesetScene(TilesetApp& app);

  private:
    bool doEarlyProcessEvent(gf::Event& event) override;
    void doUpdate(gf::Time time) override;
    void doRender(gf::RenderTarget& target, const gf::RenderStates &states) override;

  private:
    TilesetApp& m_app;
    TilesetGui m_gui;
  };

}


#endif // TILESET_SCENE_H
