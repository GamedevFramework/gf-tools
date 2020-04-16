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
#include "NoiseState.h"

#include <gf/Noises.h>

namespace gftools {

  namespace {

    void generateArrayFromNoise(gf::Heightmap& heightmap, gf::Noise2D& noise, double scale = 1.0) {
      heightmap.reset();
      heightmap.addNoise(noise, scale);
      heightmap.normalize();
    }

    gf::Image generateImageFromArray(const RenderingParams& renderingParams, const gf::Heightmap& heightmap) {
      switch (renderingParams.type) {
        case RenderingType::Grayscale:
          return heightmap.copyToGrayscaleImage();

        case RenderingType::Colored: {
          // see: http://www.blitzbasic.com/codearcs/codearcs.php?code=2415
          gf::ColorRampD ramp;
          ramp.addColorStop(0.000, gf::ColorD::fromRgba32(  2,  43,  68)); // very dark blue: deep water
          ramp.addColorStop(0.250, gf::ColorD::fromRgba32(  9,  62,  92)); // dark blue: water
          ramp.addColorStop(0.490, gf::ColorD::fromRgba32( 17,  82, 112)); // blue: shallow water
          ramp.addColorStop(0.500, gf::ColorD::fromRgba32( 69, 108, 118)); // light blue: shore
          ramp.addColorStop(0.501, gf::ColorD::fromRgba32( 42, 102,  41)); // green: grass
          ramp.addColorStop(0.750, gf::ColorD::fromRgba32(115, 128,  77)); // light green: veld
          ramp.addColorStop(0.850, gf::ColorD::fromRgba32(153, 143,  92)); // brown: tundra
          ramp.addColorStop(0.950, gf::ColorD::fromRgba32(179, 179, 179)); // grey: rocks
          ramp.addColorStop(1.000, gf::ColorD::fromRgba32(255, 255, 255)); // white: snow

          gf::Heightmap::Render renderMode = renderingParams.colored.shaded ? gf::Heightmap::Render::Shaded : gf::Heightmap::Render::Colored;
          return heightmap.copyToColoredImage(ramp, renderingParams.colored.waterLevel, renderMode);
        }
      }

      assert(false);
      return gf::Image();
    }
  }

  void NoiseState::generateImage(gf::Noise2D& noise) {
    switch (fractal.type) {
      case FractalType::None:
        generateArrayFromNoise(heightmap, noise, scale);
        break;

      case FractalType::FBm: {
        gf::FractalNoise2D fractalNoise(noise, 1, fractal.octaves, fractal.lacunarity, fractal.persistence, fractal.f.dimension);
        generateArrayFromNoise(heightmap, fractalNoise, scale);
        break;
      }

      case FractalType::Multifractal:  {
        gf::Multifractal2D fractalNoise(noise, 1, fractal.octaves, fractal.lacunarity, fractal.persistence, fractal.m.dimension);
        generateArrayFromNoise(heightmap, fractalNoise, scale);
        break;
      }

      case FractalType::HeteroTerrain: {
        gf::HeteroTerrain2D fractalNoise(noise, 1, fractal.ht.offset, fractal.octaves, fractal.lacunarity, fractal.persistence, fractal.ht.dimension);
        generateArrayFromNoise(heightmap, fractalNoise, scale);
        break;
      }

      case FractalType::HybridMultifractal: {
        gf::HybridMultifractal2D fractalNoise(noise, 1, fractal.hm.offset, fractal.octaves, fractal.lacunarity, fractal.persistence, fractal.hm.dimension);
        generateArrayFromNoise(heightmap, fractalNoise, scale);
        break;
      }

      case FractalType::RidgedMultifractal: {
        gf::RidgedMultifractal2D fractalNoise(noise, 1, fractal.rm.offset, fractal.rm.gain, fractal.octaves, fractal.lacunarity, fractal.persistence, fractal.rm.dimension);
        generateArrayFromNoise(heightmap, fractalNoise, scale);
        break;
      }
    }

    image = generateImageFromArray(rendering, heightmap);
    texture.update(image);
  }

}
