// ################################################
// Project 2 - Ray Casting 体绘制
// 实现体积绘制的光线跟踪（光线投射）算法
// by z0gSh1u @ https://github.com/z0gSh1u/seu-viz
// ################################################

// [About the geometry]
// PLEASE Refer to the document for coordinate system geometry definition!
// [About the bounding box]
// The left-bottom-front point of bounding box is at (0, 0, 0)
// So we only need the right-top-back point to determine the bounding box
// We call that point `bboxRTB`
// [About the data]
// IMPORTANT: Required volume data .raw file fomat is
// 16-bit unsigned LittleEndian per voxel
#define BytesPerVoxel 2
// [About the transfer function]
// Refer to:
#include "transferFunction.hpp"
// Some preset TFs are set for demo volume data
// Refer to the document for detail

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ctime>

#include "../framework/ReNow.hpp"
#include "../framework/Camera.hpp"
#include "../framework/Utils.hpp"

using namespace std;
using namespace zx;

using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;

// ###### Here are some parameters maybe you want to modify ######

#define WINDOW_WIDTH 520
#define WINDOW_HEIGHT 520

// volume data metainfo
const int VolumeWidth = 512;  // x
const int VolumeHeight = 512; // y
const int VolumeZCount = 340; // z (thickness)
const vec3 bboxRTB(VolumeWidth, VolumeHeight, VolumeZCount);
const int SizePerSlice = VolumeWidth * VolumeHeight;
const int VoxelCount = SizePerSlice * VolumeZCount;
// const string VolumePath = "./model/shep3d_256.uint16.raw";
const string VolumePath = "./model/lungct_C052_512_512_340.raw";
// volume data
uint16 volumeData[VoxelCount];
// after coloring using transfer function (TF) (Classify Step)
vec4 coloredVolumeData[VoxelCount]; // RGBA

// image plane related
const int ImagePlaneWidth = 512;
const int ImagePlaneHeight = 512;
const int ImagePlaneSize = ImagePlaneWidth * ImagePlaneHeight;
RGBAColor imagePlane[ImagePlaneHeight][ImagePlaneWidth]; // row-col order
vec3 initialEyePos(0, 0, -128);
vec3 initialEyeDirection(0, 0, 1);

// ###### ###### ###### ###### ###### ###### ###### ###### ######

int getVoxelIndex(int x, int y, int z) {
  return SizePerSlice * z + VolumeWidth * y + x;
}

// get voxel value according to (x, y, z)
int getVoxel(int x, int y, int z) { return volumeData[getVoxelIndex(x, y, z)]; }

Camera camera(vec3(0.5*512, 1.5*512, 0.5*128));

// Get interpolated color of pos using TriLinear method.
RGBAColor colorInterpTriLinear(const vec3 &pos, const vec3 &bboxRTB) {
  int x0, y0, z0, x1, y1, z1; // integer positions
  float xd, yd, zd;           // remainders
  RGBAColor res;

  x0 = int(pos.x);
  xd = pos.x - x0;
  x1 = x0 + 1;
  x1 = x1 >= bboxRTB.x ? x1 - 1 : x1;

  y0 = int(pos.y);
  yd = pos.y - y0;
  y1 = y0 + 1;
  y1 = y1 >= bboxRTB.y ? y1 - 1 : y1;

  z0 = int(pos.z);
  zd = pos.z - z0;
  z1 = z0 + 1;
  z1 = z1 >= bboxRTB.z ? z1 - 1 : z1;

  res =
      (1 - xd) * (1 - yd) * (1 - zd) *
          coloredVolumeData[getVoxelIndex(x0, y0, z0)] +
      xd * (1 - yd) * (1 - zd) * coloredVolumeData[getVoxelIndex(x1, y0, z1)] +
      (1 - xd) * yd * (1 - zd) * coloredVolumeData[getVoxelIndex(x0, y1, z0)] +
      (1 - xd) * (1 - yd) * zd * coloredVolumeData[getVoxelIndex(x1, y0, z0)] +
      xd * yd * (1 - zd) * coloredVolumeData[getVoxelIndex(x0, y1, z1)] +
      xd * (1 - yd) * zd * coloredVolumeData[getVoxelIndex(x1, y0, z1)] +
      (1 - xd) * yd * zd * coloredVolumeData[getVoxelIndex(x0, y1, z1)] +
      xd * yd * zd * coloredVolumeData[getVoxelIndex(x0, y0, z0)];

  zx::clipRGBA(res);
  return res;
}

// Fusion `sample` color into `accumalted` using front-to-back iteration method.
void fusionColorFrontToBack(vec4 &accmulated, const vec4 &sample) {
  // RGB fusion
  for (int i = 0; i < 3; i++) {
    accmulated[i] = accmulated[i] + (1 - accmulated.a) * sample.a * sample[i];
  }
  // alpha channel
  accmulated.a = accmulated.a + (1 - accmulated.a) * sample.a;
}

// judge if point is inside bouding box
bool inBBox(const vec3 &point, const vec3 &bboxRTB) {
  return point.x >= 0 && point.x <= bboxRTB.x && point.y >= 0 &&
         point.y <= bboxRTB.y && point.z >= 0 && point.z <= bboxRTB.z;
}

// helper for function `bool intersectTest`
bool _intersectTestOnePlane(const float origin, const float direction,
                            const float bboxRTB, float &t0Final,
                            float &t1Final) {
  const float EPSILON = 1e-4;
  float bMin = 0, bMax = bboxRTB;
  float t0, t1;

  if (fabs(direction) > EPSILON) { // direction != 0
    t0 = (bMin - origin) / direction;
    t1 = (bMax - origin) / direction;
    if (t0 > t1) {
      zx::swap<float>(t0, t1);
    }
    t0Final = max(t0Final, t0);
    t1Final = min(t1Final, t1);

    if (t0Final > t1Final || t1 < 0) {
      return false; // must not intersect
    }
  }

  return true; // cannot determine for now
}

// Ray and bounding box intersection test
// Eric Haines. "Essential Ray Tracing Algorithms." An introduction to ray
// tracing. pp.33, 1989.
// @see https://zhuanlan.zhihu.com/p/138259656
// origin->direction determines the ray
// the entry point is returned using &entry with parameter &t
bool intersectTest(const vec3 &origin, const vec3 &direction,
                   const vec3 &bboxRTB, vec3 &entry, float &t) {
  float t0Final = -9999999, t1Final = 9999999;

  if (!_intersectTestOnePlane(origin.x, direction.x, bboxRTB.x, t0Final,
                              t1Final) ||
      !_intersectTestOnePlane(origin.y, direction.y, bboxRTB.y, t0Final,
                              t1Final) ||
      !_intersectTestOnePlane(origin.z, direction.z, bboxRTB.z, t0Final,
                              t1Final)) {
    return false;
  }

  t = t0Final >= 0 ? t0Final : t1Final;
  entry = origin + t * direction;

  return true;
}

int colorTime = 0;

// Cast one ray corresponding to (x0, y0) at image plane
// according to `coloredVolumeData`. Returns the fused RGBAColor.
void castOneRay(int x0, int y0, const vec3 &bboxRTB, const mat3 &rotateMat,
                const vec3 &translateVec,
                const vec4 &defaultColor = vec4(RGBBlack, 1.0)) {
  RGBAColor accumulated(0, 0, 0, 0); // acuumulated color during line integral

  // we use parallel projection
  vec3 source(x0, y0, 0);
  // the default direction of ray is +z
  vec3 direction(initialEyeDirection);
  // transform to object corrdinate
  // [xobj, yobj, zobj](t) = [x, y, tz]R+T
  source = rotateMat * source + translateVec;
  direction = rotateMat * direction;

  vec3 entry;     // entry point of ray into bounding box
  vec3 samplePos; // current voxel coordinate
  RGBAColor sampleColor(0.0, 0.0, 0.0, 0.0); // current color (at current voxel)

  const float delta = 1; // sampling stepsize
  float tEntry;

  bool intersect = intersectTest(source, direction, bboxRTB, entry, tEntry);
  if (intersect) {
    // initialize samplePos
    samplePos = entry;
    while (inBBox(samplePos, bboxRTB) && accumulated.a < 1.0) {
      // get sampleColor via interpolation
      sampleColor = colorInterpTriLinear(samplePos, bboxRTB);
      // Cout = Cin + (1-ain)aiCi
      fusionColorFrontToBack(accumulated, sampleColor);
      // go forward
      samplePos += delta * direction;
    }
    imagePlane[y0][x0] = RGBAColor(accumulated);
    colorTime++;
  } else {
    imagePlane[y0][x0] = RGBAColor(defaultColor);
  }
}

// Implement of simple progressbar
// @see https://stackoverflow.com/questions/14539867/
void updateProgressBar(float progress, int barWidth = 50) {
  cout << "[";
  int pos = barWidth * progress;
  for (int i = 0; i < barWidth; ++i) {
    if (i < pos)
      cout << "=";
    else if (i == pos)
      cout << ">";
    else
      cout << " ";
  }
  cout << "] " << int(progress * 100.0) << " %\r";
  cout.flush();
}

// The very main.
void rayCasting() {
  // R, C computed using camera
  // mat3 R = mat3(camera.getLookAt());
  mat3 R = eye3();
  vec3 C = vec3(initialEyePos);

  float progress = 0.0;
  const float progressPerRow = (float)ImagePlaneWidth / ImagePlaneSize;

  for (int r = 0; r < ImagePlaneHeight; r++) {
    for (int c = 0; c < ImagePlaneWidth; c++) {
      castOneRay(c, r, bboxRTB, R, initialEyePos);
    }
    updateProgressBar(progress = progress + progressPerRow);
  }
}

int main() {
  time_t tic = time(NULL);

  // initialize
  GLFWwindow *window =
      initGLWindow("2-raycasting", WINDOW_WIDTH, WINDOW_HEIGHT);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glClearColor(0.9, 0.9, 0.9, 1);

  readFileBinary(VolumePath, BytesPerVoxel, VoxelCount, volumeData);

  // FIXME
  camera.rotate(vec2(-89, 0));

  cout << ">>> Start applying transfer function..." << endl;
  // TF_SheppLogan(volumeData, VoxelCount, coloredVolumeData);
  // TF_CTLung(volumeData, VoxelCount, coloredVolumeData);
  TF_CTBone2(volumeData, VoxelCount, coloredVolumeData);

  cout << ">>> Start ray casting..." << endl;
  rayCasting();
  cout << endl;

  time_t toc = time(NULL);
  cout << "Time elapsed: " << toc - tic << " secs." << endl;
  cout << "# Ray cast: " << ImagePlaneSize << endl;

  cout << ">>> Start rendering..." << endl;
  // main loop
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawPixels(ImagePlaneWidth, ImagePlaneHeight, GL_RGBA, GL_FLOAT,
                 imagePlane);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}