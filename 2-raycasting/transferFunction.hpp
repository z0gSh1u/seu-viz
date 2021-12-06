#pragma once
#ifndef TRANSFERFUNCTION_HPP_
#define TRANSFERFUNCTION_HPP_

#include <glm/vec4.hpp>
#include "../framework/ReNow.hpp"

using glm::vec4;

namespace zx {

// TF for SheppLogan demo (shep3d_64.uint16.raw)
void TF_SheppLogan(const uint16 *volumeData, int voxelCount,
                   vec4 *coloredVolumeData) {
  for (int i = 0; i < voxelCount; i++) {
    uint16 v = volumeData[i]; // 0 ~ 255,   0  51   77   255
    if (v < 51) {
      // white
      coloredVolumeData[i] = vec4(1.0, 1.0, 1.0, 0.05);
    } else if (v < 77) {
      // a little red
      coloredVolumeData[i] = vec4(0.9, 0.1, 0.1, 0.1);
    } else if (v < 255) {
      // a little yellow
      coloredVolumeData[i] = vec4(0.1, 0.9, 0.9, 0.1);
    } else {
      // gray
      coloredVolumeData[i] = vec4(0.3, 0.3, 0.3, 0.5);
    }
  }
}

// TF for CT...
// Note that numbers in volumeData is UINT16, which means
// before Rescale Slope operation to CT HU value.
// So the actual HU value is `v-1024` if rescaling function
// is y=1*x-1024.

void TF_CT_BoneOnly(const uint16 *volumeData, int voxelCount,
                    vec4 *coloredVolumeData) {
  for (int i = 0; i < voxelCount; i++) {
    uint16 v = volumeData[i];
    RGBAColor color;
    if (v < 200) {                           // [~, 200] // air
      color = RGBAColor(255, 255, 255, 0.0); // transparent
    } else if (v < 950) {                    // [200, 990] // soft tissue
      color = RGBAColor(250, 235, 215, 0.0); // skin color
    } else if (v < 1150) {                   // [990, 1150] // muscle, heart
      color = RGBAColor(222, 129, 46, 0.0);  // light red
    } else if (v < 2200) {                   // [1150, 2200] // bone
      int tmp = v - 1150;
      float yz = tmp / (2200 - 1150);
      vec3 xx = yz * vec3(30, 30, 30);
      xx.x = int(xx.x);
      xx.y = int(xx.y);
      xx.z = int(xx.z);
      vec3 rgb = vec3(200, 200, 200) + xx;
      color = RGBAColor(rgb, 0.06);          // light blue
    } else {                                 // [2200, ~] // invalid value
      color = RGBAColor(255, 255, 255, 0.0); // transparent
    }
    coloredVolumeData[i] = normalizeRGBAColor(color);
  }
}

void TF_CT_Bone(const uint16 *volumeData, int voxelCount,
                vec4 *coloredVolumeData) {
  for (int i = 0; i < voxelCount; i++) {
    uint16 v = volumeData[i];
    RGBAColor color;
    if (v < 200) {                            // [~, 200] // air
      color = RGBAColor(255, 255, 255, 0.0);  // transparent
    } else if (v < 950) {                     // [200, 990] // soft tissue
      color = RGBAColor(250, 235, 215, 0.05); // skin color
    } else if (v < 1150) {                    // [990, 1150] // muscle, heart
      color = RGBAColor(230, 0, 26, 0.0);     // light red
    } else if (v < 2200) {                    // [1150, 2200] // bone
      color = RGBAColor(119, 136, 153, 0.5);  // light blue
    } else {                                  // [2200, ~] // invalid value
      color = RGBAColor(255, 255, 255, 0.0);  // transparent
    }
    coloredVolumeData[i] = normalizeRGBAColor(color);
  }
}

void TF_CT_Lung(const uint16 *volumeData, int voxelCount,
                vec4 *coloredVolumeData) {
  // TODO
  return;
}

} // namespace zx

#endif