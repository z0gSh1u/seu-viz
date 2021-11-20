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
#include <rapidjson/document.h>
#include <ctime>
#include <thread>
#include <algorithm>

#include "../framework/ReNow.hpp"
#include "../framework/Utils.hpp"

using namespace std;
using namespace zx;

using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;
using rapidjson::Document;

// ###### Here are some parameters that will be read from config.json ######
// ###### or calculated accordingly.                                  ######

int WINDOW_WIDTH;
int WINDOW_HEIGHT;

// volume data metainfo
int VolumeWidth;  // x
int VolumeHeight; // y
int VolumeZCount; // z (thickness)
vec3 bboxRTB;     // bounding box Right-Top-Back
int SizePerSlice;
int VoxelCount;
string VolumePath;
// volume data
uint16 *volumeData;
// after coloring using transfer function (TF) (Classify Step)
vec4 *coloredVolumeData; // RGBA

// image plane related
int ImagePlaneWidth;
int ImagePlaneHeight;
int ImagePlaneSize = ImagePlaneWidth * ImagePlaneHeight;
RGBAColor *imagePlane; // row-col order
vec3 initialEyePos;
vec3 initialEyeDirection(0, 0, 1); // refer to the geometry
mat3 rotateMatrix;                 // the R matrix

// transfer function
string TransferFunctionName;

int multiThread; // number of threads

// for progress bar printing
float progress = 0.0;
float progressPerThreadPerRow;

// ###### ###### ###### ###### ###### ###### ###### ###### ######

// preset rotate matrics
vector<pair<string, mat3>> PresetRotateMatrics = {
    {"Axial View Plane", mat3(eye4())},
    {"Coronal View Plane", mat3(eye4() * rotateX(radians(-90)))},
    {"Sagittal View Plane", mat3(eye4() * rotateY(radians(90)))}};
int rotateMatrixIndex = 2;

// load config file from config.json
void loadConfigFile() {
  Document d;
  d.Parse(readFileText("./config2.json").c_str());

  WINDOW_WIDTH = d["WindowWidth"].GetInt();
  WINDOW_HEIGHT = d["WindowHeight"].GetInt();

  VolumePath = d["VolumePath"].GetString();
  VolumeWidth = d["VolumeWidth"].GetInt();
  VolumeHeight = d["VolumeHeight"].GetInt();
  VolumeZCount = d["VolumeZCount"].GetInt();
  bboxRTB = vec3(VolumeWidth, VolumeHeight, VolumeZCount);
  SizePerSlice = VolumeWidth * VolumeHeight;
  VoxelCount = SizePerSlice * VolumeZCount;
  volumeData = new uint16[VoxelCount];
  coloredVolumeData = new vec4[VoxelCount];

  ImagePlaneWidth = d["ImagePlaneWidth"].GetInt();
  ImagePlaneHeight = d["ImagePlaneHeight"].GetInt();
  ImagePlaneSize = ImagePlaneWidth * ImagePlaneHeight;
  imagePlane = new RGBAColor[ImagePlaneSize];

  initialEyePos = vec3(0, 0, VolumeZCount);

  TransferFunctionName = d["TransferFunction"].GetString();

  multiThread = d["MultiThread"].GetInt();
  progressPerThreadPerRow =
      (float)ImagePlaneWidth * multiThread / ImagePlaneSize;
}

// ###### About image pixel data access ######

int getPixelIndex(int r, int c) {
  // Refer to the geometry definition. Row starts from "bottom"
  // rather than "top".
  return (ImagePlaneHeight - 1 - r) * ImagePlaneWidth + c;
}

// ###### ###### ###### ###### ###### ######

// ###### About volume data access ######

int getVoxelIndex(int x, int y, int z) {
  return SizePerSlice * z + VolumeWidth * y + x;
}

// get voxel value according to (x, y, z)
int getVoxel(int x, int y, int z) { return volumeData[getVoxelIndex(x, y, z)]; }

// ###### ###### ###### ###### ###### ######

// apply transfer function to fill `coloredVolumeData`
void applyTransferFunction() {
  if (TransferFunctionName == "SheppLogan") {
    TF_SheppLogan(volumeData, VoxelCount, coloredVolumeData);
  } else if (TransferFunctionName == "CT_Lung") {
    TF_CT_Lung(volumeData, VoxelCount, coloredVolumeData);
  } else if (TransferFunctionName == "CT_Bone") {
    TF_CT_Bone(volumeData, VoxelCount, coloredVolumeData);
  } else {
    cerr << "[WARN] Invalid transfer function: " << TransferFunctionName
         << endl;
  }
}

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
  return point.x >= 0 && point.x < bboxRTB.x && point.y >= 0 &&
         point.y < bboxRTB.y && point.z >= 0 && point.z < bboxRTB.z;
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

int colorTimes = 0;

// Cast one ray corresponding to (x0, y0) at image plane
// according to `coloredVolumeData`. Returns the fused RGBAColor.
void castOneRay(int x0, int y0, const vec3 &bboxRTB, const mat3 &rotateMat,
                const vec3 &translateVec,
                const vec4 &defaultColor = vec4(RGBBlack, 1.0)) {
  RGBAColor accumulated(0, 0, 0, 0); // acuumulated color during line integral

  // use parallel projection
  vec3 source(x0, y0, 0); // light source point
  // the default direction of ray is +z
  vec3 direction(initialEyeDirection);
  // transform to object coordinate
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
    imagePlane[getPixelIndex(y0, x0)] = RGBAColor(accumulated);
    colorTimes++;
  } else {
    imagePlane[getPixelIndex(y0, x0)] = RGBAColor(defaultColor);
  }
}

// Implement of simple progressbar
// @see https://stackoverflow.com/questions/14539867/
void updateProgressBar(float progress, int barWidth = 50) {
  cout << "[";
  int pos = barWidth * progress;
  for (int i = 0; i < barWidth; i++) {
    cout << (i < pos ? "=" : i == pos ? ">" : " ");
  }
  cout << "] " << int(progress * 100.0) << " %\r";
  cout.flush();
}

// Written in this form to support multi-thread processing.
void castSomeRays(int rowLow, int rowHigh) {
  mat3 R = rotateMatrix;
  vec3 T = -initialEyePos;
  for (int r = rowLow; r < rowHigh; r++) {
    for (int c = 0; c < ImagePlaneWidth; c++) {
      castOneRay(c, r, bboxRTB, R, T);
    }
    if (rowLow == 0) { // the first thread
      updateProgressBar(progress = progress + progressPerThreadPerRow);
    }
  }
}

// print name of current view plane
void printCurrentViewPlane() {
  cout << "[Current View Plane]\n"
       << PresetRotateMatrics[rotateMatrixIndex].first << "\n";
}

void consoleLogWelcome() {
  cout << "################################\n"
          "# Viz Project 2 - Ray Casting  #\n"
          "#  by 212138 - Zhuo Xu         #\n"
          "# @ github.com/z0gSh1u/seu-viz #\n"
          "################################\n"
          "### Operation Guide ###\n"
          "Use [Enter] key to switch between Preset Rotate Matrics\n"
          "to observe from different views.\n"
          "########################\n";
}

// The very main: Multi-thread Ray Casting
void rayCasting() {
  vector<thread> threadPool;
  const float sharePerThread = (float)1 / multiThread;
  for (int i = 0; i < multiThread; i++) {
    int low = int(sharePerThread * i * ImagePlaneHeight);
    int high = i == multiThread - 1
                   ? ImagePlaneHeight
                   : int(sharePerThread * (i + 1) * ImagePlaneHeight);
    threadPool.push_back(thread(castSomeRays, low, high));
  }
  for_each(threadPool.begin(), threadPool.end(), [=](thread &t) { t.join(); });
}

void keyboardCallback(GLFWwindow *window, int key, int _, int action, int __) {
  if (action == GLFW_PRESS) {
    // change between view planes
    if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
      // prepare for a new rendering
      colorTimes = 0;
      progress = 0.0;
      // switch rotate matrix
      rotateMatrixIndex = (rotateMatrixIndex + 1) % PresetRotateMatrics.size();
      rotateMatrix = PresetRotateMatrics[rotateMatrixIndex].second;
      cout << ">>> Restart ray casting using " << multiThread << " threads..."
           << endl;
      // print current view plane name
      cout << "[Current View Plane]: "
           << PresetRotateMatrics[rotateMatrixIndex].first << "\n";
      // restart ray casting
      time_t tic = time(NULL);
      rayCasting();
      time_t toc = time(NULL);
      cout << endl
           << "# Ray intersects: " << colorTimes << endl
           << "# Ray cast: " << ImagePlaneSize << endl
           << "Time elapsed: " << toc - tic << " secs." << endl
           << endl
           << ">>> Start rendering..." << endl
           << endl;
    }
  }
}

int main() {
  // initialize
  consoleLogWelcome();
  loadConfigFile();

  GLFWwindow *window =
      initGLWindow("2-raycasting", WINDOW_WIDTH, WINDOW_HEIGHT);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glfwSetKeyCallback(window, keyboardCallback);
  glClearColor(0.9, 0.9, 0.9, 1);

  readFileBinary(VolumePath, BytesPerVoxel, VoxelCount, volumeData);

  time_t tic = time(NULL);
  cout << ">>> Start applying transfer function..." << endl;
  cout << "[Transfer Function Name]: " << TransferFunctionName << endl;
  applyTransferFunction();
  cout << endl;

  // simulate keyboard press to start rendering
  keyboardCallback(NULL, GLFW_KEY_ENTER, -1, GLFW_PRESS, -1);

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