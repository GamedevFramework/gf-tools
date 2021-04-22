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
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include <gf/Log.h>

#include "bits/TilesetApp.h"

#include "config.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::printf("Usage: gf_tileset <file.json>\n");
    return EXIT_FAILURE;
  }

  gf::Path path(argv[1]);

  if (!std::filesystem::exists(path)) {
    gf::Log::info("File does not exists. Creating an empty file: '%s'\n", path.string().c_str());
    gftools::TilesetData data;
    gftools::TilesetData::save(path, data);
  }

  gftools::TilesetApp app(GF_TOOLS_DATADIR, path);
  app.run();
  return EXIT_SUCCESS;
}
