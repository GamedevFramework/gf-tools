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
#ifndef TILESET_APP_H
#define TILESET_APP_H

#include <gf/Random.h>
#include <gf/ResourceManager.h>
#include <gf/SceneManager.h>

#include "TilesetData.h"
#include "TilesetScene.h"

namespace gftools {

  struct TilesetApp : public gf::SceneManager {
    TilesetApp(gf::Path path, gf::Path filename);
    ~TilesetApp();

    gf::ResourceManager resources;
    gf::Random random;

    gf::Path datafile;
    TilesetData data;

    TilesetScene scene;
  };

}

#endif // TILES_APP_H
