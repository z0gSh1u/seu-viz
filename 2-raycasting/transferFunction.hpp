#pragma once
#ifndef TRANSFERFUNCTION_HPP_
#define TRANSFERFUNCTION_HPP_

#include <glm/vec4.hpp>
#include "../framework/ReNow.hpp"

using glm::vec4;

namespace zx {

// TF for SheppLogan demo (shep3d_64.uint16.raw)
void TF_SheppLogan(uint16 *volumnData, int voxelCount,
                   vec4 *coloredVolumnData) {
  for (int i = 0; i < voxelCount; i++) {
    Byte v = volumnData[i]; // 0 ~ 255,   0  51   77   255
    if (v < 51) {
      // white
      coloredVolumnData[i] = vec4(1.0, 1.0, 1.0, 0.05);
    } else if (v < 77) {
      // a little red
      coloredVolumnData[i] = vec4(0.9, 0.1, 0.1, 0.1);
    } else if (v < 255) {
      // a little yellow
      coloredVolumnData[i] = vec4(0.1, 0.9, 0.9, 0.1);
    } else {
      // gray
      coloredVolumnData[i] = vec4(0.3, 0.3, 0.3, 0.5);
    }
  }
}

// More for CT...

} // namespace zx

#endif