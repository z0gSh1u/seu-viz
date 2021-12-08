#pragma once

// Transfer functions for Ray Casting
// by z0gSh1u (Zhuo Xu) @ https://github.com/z0gSh1u/seu-viz
// Contains TFs for CT scan now.

#ifndef TRANSFERFUNCTION_HPP_
#define TRANSFERFUNCTION_HPP_

#include <glm/vec4.hpp>
#include "../framework/ReNow.hpp"
#include "../framework/Utils.hpp"

using glm::vec4;

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
    if (v < 1150) {
      color = Transparent; // [~, 1150], we dont care
    } else if (v < 2200) { // [1150, 2200], bone (mostly)
      color =
          RGBAColor(colorInterpLinear(v, 1150, 2200, RGBColor(180, 180, 180),
                                      vec3(60, 60, 60)),
                    0.06); // gray
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
          RGBAColor(colorInterpLinear(v, 1040, 1155, RGBColor(255, 208, 175),
                                      vec3(0, 30, 30)),
                    0.05); // like skin color
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

// Shows mainly muscle and some bone.
void TF_CT_What(const uint16 *volumeData, int voxelCount,
                         RGBAColor *coloredVolumeData) {
  RGBAColor color;
  uint16 v;
  for (int i = 0; i < voxelCount; i++) {
    v = volumeData[i];
    if (v < 880) { // [~, 1040], we dont care
      color = Transparent;
    } else if (v < 920) { // [1040, 1155], muscle
      color =
          RGBAColor(colorInterpLinear(v, 880, 920, RGBColor(255, 208, 175),
                                      vec3(0, 30, 30)),
                    0.5); // like skin color
      // color =RGBAColor(RGBColor(255, 208, 175), 1.0);
    } else if (v < 2200) { // [1155, 2200], bone (mostly)
      // color =
      //     RGBAColor(colorInterpLinear(v, 1155, 2200, RGBColor(180, 180, 180),
      //                                 vec3(60, 60, 60)),
      //               0.07); // gray
      color = Transparent;
    } else {
      color = Transparent;
    }
    coloredVolumeData[i] = normalizeRGBAColor(color);
  }
}

} // namespace zx

#endif