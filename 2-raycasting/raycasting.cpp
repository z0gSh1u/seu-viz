// ################################################
// Project 2 - Ray Casting 体绘制
// 实现体积绘制的光线跟踪（光线投射）算法
// by z0gSh1u @ https://github.com/z0gSh1u/seu-viz
// ################################################

// TODO
// 1 - Traceball  [OK!]
// 2 - Lighting   [OK!]
// 3 - TF Auto or design  [TODO]

// [compile]
//   Compile in `Release` mode, or rendering will be slow.
// [geometry]
//   Refer to the document for geometry definitions.
// [bounding box]
//   The left-bottom-front point of bounding box sits at (0, 0, 0).
//   So we only need the right-top-back point `bboxRTB` to determine the
//   bounding box.
// [data]
//   Required volume data is .raw file format with 16-bit unsigned LittleEndian
//   per voxel.
#define BytesPerVoxel 2
// [transfer function]
//   Refer to the document for detail. And refer to:
#include "transferFunction.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rapidjson/document.h>
#include <ctime>
#include <thread>
#include <map>
#include <functional>
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

// Here are some parameters that will be read from config
const string CONFIG_FILE = "./config.json";
// window
int WINDOW_WIDTH;
int WINDOW_HEIGHT;
// volume data metainfo
int VolumeWidth;  // x
int VolumeHeight; // y
int VolumeZCount; // z (thickness)
vec3 bboxRTB;     // bounding box Right-Top-Back
int PixelPerSlice;
int VoxelCount;
string VolumePath;
// volume data
uint16 *volumeData;
// after coloring using transfer function (TF) (classification step)
vec4 *coloredVolumeData; // RGBA
// image plane related
int ImagePlaneWidth;
int ImagePlaneHeight;
int ImagePlaneSize;
RGBAColor *imagePlane;
// transfer function
string TransferFunctionName;
// register your transfer function here
map<string, function<void(const uint16 *a, int b, vec4 *c)>>
    transferFunctionMap = {{"SheppLogan", TF_SheppLogan},
                           {"CT_Lung", TF_CT_Lung},
                           {"CT_Bone", TF_CT_Bone},
                           {"CT_BoneOnly", TF_CT_BoneOnly}};
// number of threads
int multiThread;
// for progress bar printing
float progress = 0.0;
float progressPerThreadPerRow;
int coloringCount = 0;
// observing
int currentTexId = -1;       // image plane is stored as texture
vec3 initEyePos(0, 0, 2048); // somewhere far Enough
vec3 normalizedEyePos(0, 0, 1);
const vec3 initEyeDirection(0, 0, -1);
mat3 currentRotateMatrix; // current rotate matrix
#define ARROW_KEY_TRACEBALL_DELTA 0.2
void keyboardCallback(GLFWwindow *window, int key, int _, int action, int __);
// lighting
const vec3 lightDirection(0, 0, -1);
const RGBColor diffuseColor(1, 1, 1);
const RGBColor ambientColor(1, 1, 1);
const float kAmbient = 0.5;
#define ENABLE_LIGHTING true // Whether to apply lighting to volume data
// ReNow helper
ReNowHelper helper;

// Truncate the translation vector (last column) in lookAt matrix,
// since we handle translation mannually x'=xR+T
mat3 UntranslatedLookAt(vec3 eye, vec3 at, vec3 up) {
  return mat3(glm::lookAt(eye, at, up));
}

// load config file from config.json
void loadConfigFile() {
  Document d;
  d.Parse(readFileText(CONFIG_FILE).c_str());

  WINDOW_WIDTH = d["WindowWidth"].GetInt();
  WINDOW_HEIGHT = d["WindowHeight"].GetInt();

  VolumePath = d["VolumePath"].GetString();
  VolumeWidth = d["VolumeWidth"].GetInt();
  VolumeHeight = d["VolumeHeight"].GetInt();
  VolumeZCount = d["VolumeZCount"].GetInt();

  bboxRTB = vec3(VolumeWidth - 1, VolumeHeight - 1, VolumeZCount - 1);
  PixelPerSlice = VolumeWidth * VolumeHeight;
  VoxelCount = PixelPerSlice * VolumeZCount;
  volumeData = new uint16[VoxelCount];
  coloredVolumeData = new vec4[VoxelCount];

  ImagePlaneWidth = d["ImagePlaneWidth"].GetInt();
  ImagePlaneHeight = d["ImagePlaneHeight"].GetInt();
  ImagePlaneSize = ImagePlaneWidth * ImagePlaneHeight;
  imagePlane = new RGBAColor[ImagePlaneSize];

  TransferFunctionName = d["TransferFunction"].GetString();

  multiThread = d["MultiThread"].GetInt();
  progressPerThreadPerRow =
      (float)ImagePlaneWidth * multiThread / ImagePlaneSize;
}

// Get pixel index at imaging plane.
int getPixelIndex(int r, int c) {
  // Refer to the geometry definition. Row starts from "bottom"
  // rather than "top".
  return (ImagePlaneHeight - 1 - r) * ImagePlaneWidth + c;
}

// Get voxel index in volume data.
int getVoxelIndex(int x, int y, int z) {
  return PixelPerSlice * z + VolumeWidth * y + x;
}

// Get voxel value according to (x, y, z).
int getVoxel(int x, int y, int z) { return volumeData[getVoxelIndex(x, y, z)]; }

// Get normal at point.
vec3 calcNormal(int x, int y, int z) {
  float defaultValue = getVoxel(x, y, z);
  float x1 = x > 0 ? getVoxel(x - 1, y, z) : defaultValue,
        x2 = x < VolumeWidth - 1 ? getVoxel(x + 1, y, z) : defaultValue,
        y1 = y > 0 ? getVoxel(x, y - 1, z) : defaultValue,
        y2 = y < VolumeHeight - 1 ? getVoxel(x, y + 1, z) : defaultValue,
        z1 = z > 0 ? getVoxel(x, y, z - 1) : defaultValue,
        z2 = z < VolumeZCount - 1 ? getVoxel(x, y, z + 1) : defaultValue;
  // normalized normal equal to normalized gradient
  vec3 gradient(x1 - x2, y1 - y2, z1 - z2);
  return glm::normalize(gradient);
}

// Apply transfer function to fill `coloredVolumeData`.
void applyTransferFunction(const string &name) {
  if (transferFunctionMap.count(name) != 0) {
    transferFunctionMap[name](volumeData, VoxelCount, coloredVolumeData);
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

  zx::clipRGBA(res, 0, 1);
  return res;
}

// Fusion `sample` color into `accumalted` using front-to-back iteration method.
void fusionColorFrontToBack(vec4 &accmulated, const vec4 &sample) {
  // RGB channels
  for (int i = 0; i < 3; i++) {
    accmulated[i] = accmulated[i] + (1 - accmulated.a) * sample.a * sample[i];
  }
  // alpha channel
  accmulated.a = accmulated.a + (1 - accmulated.a) * sample.a;
}

// Judge if point is inside bounding box.
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

// Apply lighting (simplified Phong without specular) at sample point.
void applyLighting(RGBAColor &src, const vec3 &pos) {
  vec3 normal = calcNormal(pos.x, pos.y, pos.z);
  vec3 rgb(src.r, src.g, src.b);
  float kDiffuse = max(glm::dot(normal, lightDirection), 0.0f);

  vec3 colored = (kDiffuse * diffuseColor + kAmbient * ambientColor) * rgb;
  zx::clipRGB(colored);
  for (int i = 0; i < 3; i++) {
    src[i] = colored[i];
  }
}

// Cast one ray corresponding to (u, v) at image plane
// according to `coloredVolumeData`. Returns the fused RGBAColor.
void castOneRay(int u, int v, const vec3 &bboxRTB, const mat3 &rotateMat,
                const vec3 &translateVec,
                const vec4 &defaultColor = vec4(RGBBlack, 1.0)) {
  RGBAColor accumulated(0, 0, 0, 0); // acuumulated color during line integral

  // use parallel projection
  vec3 source = vec3(u, v, 0) + translateVec; // light source point
  // the default direction of ray is +z
  vec3 direction(initEyeDirection);
  // transform to object coordinate
  // [xobj, yobj, zobj](t) = [x, y, tz]R+T
  source = source * rotateMat;
  // the direction of ray is changed too
  direction = glm::normalize(direction * rotateMat);

  vec3 entry;            // entry point of ray into bounding box
  vec3 samplePos;        // current voxel coordinate
  RGBAColor sampleColor; // current color (at current voxel)
  const float delta = 1; // sampling stepsize
  float tEntry;          // parameter t of entry point

  bool intersect = intersectTest(source, direction, bboxRTB, entry, tEntry);
  if (intersect) {
    // initialize samplePos
    samplePos = entry;
    // ray marching
    while (inBBox(samplePos, bboxRTB) && accumulated.a < 1.0) {
      // get sampleColor via interpolation
      sampleColor = colorInterpTriLinear(samplePos, bboxRTB);
      // light it
      if (ENABLE_LIGHTING) {
        applyLighting(sampleColor, samplePos);
      }
      // Cout = Cin + (1-ain)aiCi
      fusionColorFrontToBack(accumulated, sampleColor);
      // go forward
      samplePos += delta * direction;
    }
    imagePlane[getPixelIndex(v, u)] = RGBAColor(accumulated);
    coloringCount++;
  } else {
    imagePlane[getPixelIndex(v, u)] = RGBAColor(defaultColor);
  }
}

// Written in this form to support multi-thread processing.
void castSomeRays(int rowLow, int rowHigh) {
  for (int r = rowLow; r < rowHigh; r++) {
    for (int c = 0; c < ImagePlaneWidth; c++) {
      castOneRay(c, r, bboxRTB, currentRotateMatrix, initEyePos);
    }
    if (rowLow == 0) { // the first thread
      updateProgressBar(progress = progress + progressPerThreadPerRow);
    }
  }
}

// Multi-thread Ray Casting
void castAllRays() {
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

// The very main
void rayCasting() {
  // prepare for a new rendering
  coloringCount = 0;
  progress = 0.0;

  cout << ">>> Restart ray casting using " << multiThread << " threads..."
       << endl;
  time_t tic = time(NULL);
  castAllRays();
  time_t toc = time(NULL);

  // store image plane
  if (currentTexId != -1) {
    GL_OBJECT_ID x = currentTexId;
    glDeleteTextures(1, &x);
  }
  GL_OBJECT_ID texId = helper.createTexture2D(
      imagePlane, GL_RGBA, ImagePlaneWidth, ImagePlaneHeight, GL_FLOAT);
  currentTexId = texId;

  cout << endl
       << "# Ray intersect: " << coloringCount << endl
       << "# Ray cast: " << ImagePlaneSize << endl
       << "Time elapsed: " << toc - tic << " secs." << endl
       << endl
       << ">>> Start rendering..." << endl
       << endl;
  helper.prepareUniforms(vector<UPrepInfo>{{"uTexture", (int)texId, "1i"}});
}

void consoleLogWelcome() {
  cout << "################################\n"
          "# Viz Project 2 - Ray Casting  #\n"
          "#  by 212138 - Zhuo Xu         #\n"
          "# @ github.com/z0gSh1u/seu-viz #\n"
          "################################\n"
          "### Operation Guide ###\n"
          "########################\n";
}

bool shouldReCast = true;

int main() {
  // initialize
  consoleLogWelcome();
  loadConfigFile();

  GLFWwindow *window =
      initGLWindow("2-raycasting", WINDOW_WIDTH, WINDOW_HEIGHT);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glClearColor(0, 0, 0, 1);
  glfwSetKeyCallback(window, keyboardCallback);

  // take over management
  helper = ReNowHelper(window);
  // organize the program
  GL_SHADER_ID vShader =
                   helper.createShader(GL_VERTEX_SHADER, "./shader/vMain.glsl"),
               fShader = helper.createShader(GL_FRAGMENT_SHADER,
                                             "./shader/fMain.glsl");
  GL_PROGRAM_ID mainProgram = helper.createProgram(vShader, fShader);
  helper.switchProgram(mainProgram);

  // load volume data
  readFileBinary(VolumePath, BytesPerVoxel, VoxelCount, volumeData);

  // apply transfer function
  time_t tic = time(NULL);
  cout << ">>> Start applying transfer function..." << endl;
  cout << "[Transfer Function Name]: " << TransferFunctionName << endl;
  applyTransferFunction(TransferFunctionName);
  cout << endl;

  // render the image plane just as background
  const int nPoints = 4;
  float vBack[nPoints * 2] = {-1, -1, 1,  -1,
                              1,  1,  -1, 1}; // vertices of background
  float vtBack[nPoints * 2] = {0, 0, 1, 0, 1, 1, 0, 1}; // texture coords
  GL_OBJECT_ID vBackBuf = helper.createVBO(), vtBackBuf = helper.createVBO();

  // main loop
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (shouldReCast) {
      // resend vertices
      helper.prepareAttributes(vector<APrepInfo>{
          {vBackBuf, vBack, nPoints * 2, "aPosition", 2, GL_FLOAT},
          {vtBackBuf, vtBack, nPoints * 2, "aTexCoord", 2, GL_FLOAT},
      });
      // recalculate the rotate matrix
      currentRotateMatrix =
          UntranslatedLookAt(normalizedEyePos, vec3(0, 0, 0), vec3(0, 1, 0));
      // recast the rays
      rayCasting();
      shouldReCast = false;
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, nPoints);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  helper.freeAllocatedObjects();
  glfwTerminate();
  return 0;
}

// support observation
void keyboardCallback(GLFWwindow *window, int key, int _, int action, int __) {
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_LEFT) {
      if (normalizedEyePos.x >= -1) {
        normalizedEyePos.x -= ARROW_KEY_TRACEBALL_DELTA;
        shouldReCast = true;
      }
    } else if (key == GLFW_KEY_RIGHT) {
      if (normalizedEyePos.x <= 1) {
        normalizedEyePos.x += ARROW_KEY_TRACEBALL_DELTA;
        shouldReCast = true;
      }
    } else if (key == GLFW_KEY_UP) {
      if (normalizedEyePos.y >= -1) {
        normalizedEyePos.y -= ARROW_KEY_TRACEBALL_DELTA;
        shouldReCast = true;
      }
    } else if (key == GLFW_KEY_DOWN) {
      if (normalizedEyePos.y <= 1) {
        normalizedEyePos.y += ARROW_KEY_TRACEBALL_DELTA;
        shouldReCast = true;
      }
    }
  }
}