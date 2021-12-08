#pragma once

// Transfer functions for Ray Casting
// by z0gSh1u (Zhuo Xu) @ https://github.com/z0gSh1u/seu-viz
// Contains TFs for CT scan now.

#ifndef TRANSFERFUNCTION_HPP_
#define TRANSFERFUNCTION_HPP_

#include <glm/vec4.hpp>
#include "../framework/ReNow.hpp"
#include "../framework/Utils.hpp"

// Note that numbers in volumeData is Uint16, which means
// before "Rescaling" to CT HU value.
// So the actual HU value is `v-1024` if rescaling function
// is y=1*x-1024.
#define HU(a) (a + 1024)

namespace zx {

// Shows bone only.
void TF_CT_Bone(const uint16 *volumeData, int voxelCount,
                RGBAColor *coloredVolumeData) {
  RGBAColor color;
  uint16 v;
  for (int i = 0; i < voxelCount; i++) {
    v = volumeData[i];
    if (v < 1155) {
      color = Transparent; // [~, 1155], we dont care
    } else if (v < 2200) { // [1155, 2200], bone (mostly)
      color =
          RGBAColor(colorInterpLinear(v, 1155, 2200, RGBColor(180, 180, 180),
                                      vec3(60, 60, 60)),
                    0.1); // gray
    } else {
      color = Transparent; // [2200, ~] // invalid value
    }
    coloredVolumeData[i] = normalizeRGBAColor(color);
  }
}

// Shows mainly muscle and some bone.
void TF_CT_MuscleAndBone(const uint16 *volumeData, int voxelCount,
                         RGBAColor *coloredVolumeData) {
  RGBAColor color;
  uint16 v;
  for (int i = 0; i < voxelCount; i++) {
    v = volumeData[i];
    if (v < 1040) { // [~, 1040], we dont care
      color = Transparent;
    } else if (v < 1155) { // [1040, 1155], muscle
      color =
          RGBAColor(colorInterpLinear(v, 1040, 1155, RGBColor(255, 188, 155),
                                      vec3(0, 50, 50)),
                    0.05); // "muscle" (skin) color
    } else if (v < 2200) { // [1155, 2200], bone (mostly)
      color =
          RGBAColor(colorInterpLinear(v, 1155, 2200, RGBColor(180, 180, 180),
                                      vec3(60, 60, 60)),
                    0.07); // gray
    } else {
      color = Transparent;
    }
    coloredVolumeData[i] = normalizeRGBAColor(color);
  }
}

// Shows skin.
void TF_CT_Skin(const uint16 *volumeData, int voxelCount,
                RGBAColor *coloredVolumeData) {
  RGBAColor color;
  uint16 v;
  for (int i = 0; i < voxelCount; i++) {
    v = volumeData[i];
    if (v < 880) { // [~, 880], we dont care
      color = Transparent;
    } else if (v < 925) { // [880, 925], muscle
      color = RGBAColor(colorInterpLinear(v, 880, 925, RGBColor(255, 198, 165),
                                          vec3(0, 15, 15)),
                        0.8); // skin color, high alpha to block anatomy inside
    } else {
      color = Transparent;
    }
    coloredVolumeData[i] = normalizeRGBAColor(color);
  }
}

} // namespace zx

#endif