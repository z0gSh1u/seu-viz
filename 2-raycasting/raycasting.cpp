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
#include "../framework/Utils.hpp"

using namespace zx;

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 320

#define VOL_N 256

const int VolumnWidth = VOL_N;
const int VolumnHeight = VOL_N;
const int VolumnZCount = VOL_N;
const int ElementSize = sizeof(Byte);
const int SliceSize = VolumnWidth * VolumnHeight;
const int VoxelCount = VolumnZCount * VolumnWidth * VolumnHeight;

// string rawPath = "./model/shep3d_" + std::to_string(VOL_N) + ".uchar.raw";
string rawPath = "./model/shep3d_256.uchar.raw";

using namespace std;

Byte volData[VoxelCount];

int getIndex(int z, int r, int c) {
  return SliceSize * z + VolumnWidth * r + c;
}

// design TF (Classify)
vec4 Colors[VoxelCount]; // RGBA

float slice[VolumnWidth][VolumnHeight][4];

void transferFunction1() {
  for (int i = 0; i < VoxelCount; i++) {
    Byte v = volData[i]; // 0 ~ 255,   0  51   77   255
    if (v < 51) {
      // white
      Colors[i].r = 1.0;
      Colors[i].g = 1.0;
      Colors[i].b = 1.0;
      Colors[i].a = 0.05;
    } else if (v < 77) {
      // a little red
      Colors[i].r = 0.9;
      Colors[i].g = 0.1;
      Colors[i].b = 0.1;
      Colors[i].a = 0.1;
    } else if (v < 255) {
      // a little yellow
      Colors[i].r = 0.1;
      Colors[i].g = 0.9;
      Colors[i].b = 0.9;
      Colors[i].a = 0.1;
    } else {
      // gray
      Colors[i].r = 0.3;
      Colors[i].g = 0.3;
      Colors[i].b = 0.3;
      Colors[i].a = 0.5;
    }
  }
}



int main() {

  // initialize
  GLFWwindow *window =
      initGLWindow("2-raycasting", WINDOW_WIDTH, WINDOW_HEIGHT);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glClearColor(0.1, 0.1, 0.1, 1); // white

  readFileBinary(rawPath, VoxelCount, volData);

  cout << volData[getIndex(128, 128, 128)] << endl;

  transferFunction1();
  for (int i = 0; i < VolumnWidth; i++) {
    for (int j = 0; j < VolumnHeight; j++) {
      for (int k = 0; k < 4; k++) {
        slice[i][j][k] = Colors[getIndex(128, i, j)][k];
      }
    }
  }

  std::cout << ">>> Start rendering." << std::endl;
  // main loop
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawPixels(VolumnWidth, VolumnHeight, GL_RGBA, GL_FLOAT, slice);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}