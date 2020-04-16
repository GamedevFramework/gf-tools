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
#ifndef NOISE_STATE_H
#define NOISE_STATE_H

#include <gf/Heightmap.h>
#include <gf/Noise.h>
#include <gf/Random.h>
#include <gf/Texture.h>

namespace gftools {

  constexpr int MapSize = 1024;

  enum class RenderingType {
    Grayscale = 0,
    Colored   = 1,
  };

  struct RenderingParams {
    RenderingType type = RenderingType::Grayscale;

    struct {
      bool shaded       = false;
      float waterLevel  = 0.5f;
    } colored;
  };

  enum class FractalType {
    None                = 0,
    FBm                 = 1,
    Multifractal        = 2,
    HeteroTerrain       = 3,
    HybridMultifractal  = 4,
    RidgedMultifractal  = 5,
  };

  struct FractalParams {
    FractalType type = FractalType::None;

    struct {
      float dimension = 1.0f;
    } f; // fBm

    struct {
      float dimension = 1.0f;
    } m; // Multifractal

    struct {
      float offset = 1.0f;
      float dimension = 1.0f;
    } ht; // Hetero Terrain

    struct {
      float offset    = 0.25f;
      float dimension = 0.7f;
    } hm; // Hybrid Multifractal

    struct {
      float offset      = 1.0f;
      float gain        = 2.0f;
      float dimension   = 1.0f;
    } rm; // Ridged Multifractal

    int octaves         = 8;
    float lacunarity    = 2.0f;
    float persistence   = 0.5f;
  };

  struct NoiseState {
    NoiseState()
    : heightmap(gf::vec(MapSize, MapSize))
    , image(gf::vec(MapSize, MapSize))
    , texture(image)
    {
    }

    float scale = 1.0;
    FractalParams fractal;
    RenderingParams rendering;
    gf::Heightmap heightmap;
    gf::Image image;
    gf::Texture texture;
    gf::Random random;

    void generateImage(gf::Noise2D& noise);
  };

}

#endif // NOISE_STATE_H
