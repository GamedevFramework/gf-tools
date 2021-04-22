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
#include "TilesetApp.h"

#include <imgui.h>
#include <imgui_impl_gf.h>

namespace gftools {

  TilesetApp::TilesetApp(gf::Path path, gf::Path filename)
  : gf::SceneManager("gf_tileset", gf::vec(1600, 900))
  , resources({ path })
  , datafile(filename)
  , data(TilesetData::load(datafile))
  , scene(*this)
  {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // config
    io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // load font(s)
    io.Fonts->AddFontFromFileTTF(resources.getAbsolutePath("DroidSans.ttf").string().c_str(), 16);
    ImGui_ImplGF_Init(getWindow(), getRenderer());

    pushScene(scene);
  }

  TilesetApp::~TilesetApp() {
    ImGui_ImplGF_Shutdown();
    ImGui::DestroyContext();
  }

}
