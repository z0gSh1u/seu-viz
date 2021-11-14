#pragma once
#ifndef TRANSFERFUNCTION_HPP_
#define TRANSFERFUNCTION_HPP_

#include <glm/vec4.hpp>
#include "../framework/ReNow.hpp"

using glm::vec4;

namespace zx {

// TF for SheppLogan demo (shep3d_64.uint16.raw)
void TF_SheppLogan(const uint16 *volumnData, int voxelCount,
                   vec4 *coloredVolumnData) {
  for (int i = 0; i < voxelCount; i++) {
    uint16 v = volumnData[i]; // 0 ~ 255,   0  51   77   255
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

void TF_CTLung(const uint16 *volumnData, int voxelCount,
               vec4 *coloredVolumnData) {
  for (int i = 0; i < voxelCount; i++) {
    uint16 v = volumnData[i]; // 0 ~ 255,   0  51   77   255
    if (v < 620) {
      // sm hu
      coloredVolumnData[i] = normalize8bitColor(vec3(226, 195, 175));
      coloredVolumnData[i].a = 0.2;
    } else {
      // lg hu
      coloredVolumnData[i] = normalize8bitColor(vec3(165, 135, 118));
      coloredVolumnData[i].a = 0.7;
    }
  }
}

void TF_CTBone(const uint16 *volumnData, int voxelCount,
               vec4 *coloredVolumnData) {
  for (int i = 0; i < voxelCount; i++) {
    uint16 v = volumnData[i]; // 0 ~ 255,   0  51   77   255
    if (v < 620) {
      // sm hu
      coloredVolumnData[i] = normalize8bitColor(vec3(226, 195, 175));
      coloredVolumnData[i].a = 0.2;
    } else if (v < 1060) {
      // lg hu
      coloredVolumnData[i] = normalize8bitColor(vec3(165, 135, 118));
      coloredVolumnData[i].a = 0.4;
    } else {
      // bone... > 1060
      coloredVolumnData[i] = normalize8bitColor(vec3(242, 161, 47));
      coloredVolumnData[i].a = 0.7;
    }
  }
}

void TF_CTBone2(const uint16 *volumnData, int voxelCount,
                vec4 *coloredVolumnData) {
  for (int i = 0; i < voxelCount; i++) {
    uint16 v = volumnData[i]; // 0 ~ 255,   0  51   77   255
    if (v < 1060) {
      // sm hu
      coloredVolumnData[i] = normalize8bitColor(vec3(0, 0, 0));
      coloredVolumnData[i].a = 0.0;
    } else {
      // bone... > 1060
      coloredVolumnData[i] = normalize8bitColor(vec3(242, 161, 47));
      coloredVolumnData[i].a = 1.0;
    }
  }
}

} // namespace zx

#endif