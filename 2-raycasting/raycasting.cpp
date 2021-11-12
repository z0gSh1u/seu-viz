// ################################################
// Project 2 - Ray Casting 体绘制
// 实现体积绘制的光线跟踪（光线投射）算法
// by z0gSh1u @ https://github.com/z0gSh1u/seu-viz
// ################################################

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../framework/ReNow.hpp"
#include "../framework/Camera.hpp"
#include "../framework/Utils.hpp"

using namespace std;
using namespace zx;

using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 320

// volumn data metainfo
#define VOL_N 256
const int VolumnWidth = VOL_N;
const int VolumnHeight = VOL_N;
const int VolumnZCount = VOL_N;
const int BytesPerVoxel = 1; // TODO support 16bit data
const int SizePerSlice = VolumnWidth * VolumnHeight;
const int VoxelCount = VolumnZCount * VolumnWidth * VolumnHeight;
const string VolumnPath = "./model/shep3d_256.uchar.raw";

Byte volumnData[VoxelCount];

int getVoxel(int x, int y, int z) {
  int voxel = -1;
  if (BytesPerVoxel == 1) {
    voxel = volumnData[getIndex(z, y, x)];
  } else if (BytesPerVoxel == 2) {
    voxel = volumnData[getIndex(z, y, x) + 1]
            << 8 + volumnData[getIndex(z, y, x)];
  } else {
    throw;
  }
  return voxel;
}

int getIndex(int z, int r, int c) {
  return SizePerSlice * z + VolumnWidth * r + c;
}

// design TF (Classify)
vec4 coloredVolumnData[VoxelCount]; // RGBA

// float slice[VolumnWidth][VolumnHeight][4];

void transferFunction1() {
  for (int i = 0; i < VoxelCount; i++) {
    Byte v = volumnData[i]; // 0 ~ 255,   0  51   77   255
    if (v < 51) {
      // white
      coloredVolumnData[i].r = 1.0;
      coloredVolumnData[i].g = 1.0;
      coloredVolumnData[i].b = 1.0;
      coloredVolumnData[i].a = 0.05;
    } else if (v < 77) {
      // a little red
      coloredVolumnData[i].r = 0.9;
      coloredVolumnData[i].g = 0.1;
      coloredVolumnData[i].b = 0.1;
      coloredVolumnData[i].a = 0.1;
    } else if (v < 255) {
      // a little yellow
      coloredVolumnData[i].r = 0.1;
      coloredVolumnData[i].g = 0.9;
      coloredVolumnData[i].b = 0.9;
      coloredVolumnData[i].a = 0.1;
    } else {
      // gray
      coloredVolumnData[i].r = 0.3;
      coloredVolumnData[i].g = 0.3;
      coloredVolumnData[i].b = 0.3;
      coloredVolumnData[i].a = 0.5;
    }
  }
}

Camera camera(vec3(0.5, 0.5, 1));

vec4 triLinearInterp(const vec3 &pos, const vec3 &bboxTR) {
  int x0, y0, z0, x1, y1, z1;
  float xd, yd, zd;

  x0 = int(pos.x);
  xd = pos.x - x0;
  x1 = x0 + 1;
  x1 = x1 >= bboxTR.x ? x1 - 1 : x1;

  y0 = int(pos.y);
  yd = pos.y - y0;
  y1 = y0 + 1;
  y1 = y1 >= bboxTR.y ? y1 - 1 : y1;

  z0 = int(pos.z);
  zd = pos.z - z0;
  z1 = z0 + 1;
  z1 = z1 >= bboxTR.z ? z1 - 1 : z1;

  vec4 res;

  // TODO DRAW
  res =
      (1 - xd) * (1 - yd) * (1 - zd) * coloredVolumnData[getIndex(z0, y0, x0)] +
      xd * (1 - yd) * (1 - zd) * coloredVolumnData[getIndex(z0, y0, x1)] +
      (1 - xd) * yd * (1 - zd) * coloredVolumnData[getIndex(z0, y1, x0)] +
      (1 - xd) * (1 - yd) * zd * coloredVolumnData[getIndex(z1, y0, x0)] +
      xd * yd * (1 - zd) * coloredVolumnData[getIndex(z0, y1, x1)] +
      xd * (1 - yd) * zd * coloredVolumnData[getIndex(z1, y0, x1)] +
      (1 - xd) * yd * zd * coloredVolumnData[getIndex(z0, y1, x1)] +
      xd * yd * zd * coloredVolumnData[getIndex(z0, y0, x0)];

  // TODO clip TODO
  zx::clipRGBA(res);

  return res;
}

void fusionColorFrontToBack(vec4 &accmulated, const vec4 &sample) {
  // RGB Fusion
  for (int i = 0; i < 3; i++) {
    accmulated[i] = accmulated[i] + (1 - accmulated.a) * sample.a * sample[i];
  }
  // alpha channel
  accmulated.a = accmulated.a + (1 - accmulated.a) * sample.a;
}

void rayCast(int x0, int y0, const vec3 &bboxTR) {
  float delta = 1;
  vec4 accumulated(0, 0, 0, 0);
  vec4 sampleColor;
  vec3 eye, direction, entry, samplePos;
  // parallel lighting
  eye = vec3(x0, y0, 0); // z = 0, eye on z-axis
  auto worldMatrix = camera.getLookAt();

  while (inBBox(samplePos, bboxTR) && accumulated.a < 1.0) {
    // get sampleColor via interpolation
    sampleColor = triLinearInterp(samplePos, bboxTR);

    // Cout = Cin + (1-ain)aiCi
    fusionColorFrontToBack(accumulated, sampleColor);

    // go forward
    samplePos += delta * direction;
  }
}

bool inBBox(const vec3 &point, const vec3 &bboxTR) {
  return point.x > 0 && point.x < bboxTR.x && point.y > 0 &&
         point.y < bboxTR.y && point.z > 0 && point.z < bboxTR.z;
}

void swapFloat(float &a, float &b) {
  float tmp = a;
  a = b;
  b = tmp;
}

// get entry point
bool intersect(vec3 origin, vec3 direction, vec3 bboxTR, float &t,
               vec3 &entry) {
  float t0Final = -9999999, t1Final = 9999999;
  float t0, t1;
  float bMin, bMax;
  float EPSILON = 1e-4;

  // for YZ
  bMin = 0, bMax = bboxTR.x;
  if (fabs(direction.x) > EPSILON) { // dx != 0
    t0 = (bMin - origin.x) / direction.x;
    t1 = (bMax - origin.x) / direction.x;
    if (t0 > t1) {
      swapFloat(t0, t1);
    }
    t0Final = max(t0Final, t0);
    t1Final = min(t1Final, t1);
    if (t0Final > t1Final || t1 < 0) {
      return false;
    }
  }

  // same for XY, ZX
  bMin = 0, bMax = bboxTR.y;
  if (fabs(direction.y) > EPSILON) {
    t0 = (bMin - origin.y) / direction.y;
    t1 = (bMax - origin.y) / direction.y;
    if (t0 > t1) {
      swapFloat(t0, t1);
    }
    t0Final = max(t0Final, t0);
    t1Final = min(t1Final, t1);
    if (t0Final > t1Final || t1 < 0) {
      return false;
    }
  }

  bMin = 0, bMax = bboxTR.z;
  if (fabs(direction.z) > EPSILON) {
    t0 = (bMin - origin.z) / direction.z;
    t1 = (bMax - origin.z) / direction.z;
    if (t0 > t1) {
      swapFloat(t0, t1);
    }
    t0Final = max(t0Final, t0);
    t1Final = min(t1Final, t1);
    if (t0Final > t1Final || t1 < 0) {
      return false;
    }
  }

  t = t0Final > 0 ? t0Final : t1Final;
  entry = origin + t * direction;

  return true;
}

int main() {

  // initialize
  GLFWwindow *window =
      initGLWindow("2-raycasting", WINDOW_WIDTH, WINDOW_HEIGHT);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glClearColor(0.1, 0.1, 0.1, 1); // white

  readFileBinary(VolumnPath, VoxelCount, volumnData);

  cout << volumnData[getIndex(128, 128, 128)] << endl;

  transferFunction1();
  // for (int i = 0; i < VolumnWidth; i++) {
  //   for (int j = 0; j < VolumnHeight; j++) {
  //     for (int k = 0; k < 4; k++) {
  //       slice[i][j][k] = Colors[getIndex(128, i, j)][k];
  //     }
  //   }
  // }

  std::cout << ">>> Start rendering." << std::endl;
  // main loop
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    // glDrawPixels(VolumnWidth, VolumnHeight, GL_RGBA, GL_FLOAT, slice);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}