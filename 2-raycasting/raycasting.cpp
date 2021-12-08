// ################################################
// Project 2 - Ray Casting 体绘制
// 实现体积绘制的光线跟踪（光线投射）算法
// by z0gSh1u @ https://github.com/z0gSh1u/seu-viz
// ################################################

//                [Some Instructions for the Code]               //
// [compile]
//   Compile in `Release` mode, or rendering will be slow.
// [geometry]
//   Refer to the document for geometry definitions.
// [bounding box]
//   The left-bottom-front point of bounding box sits at (0, 0, 0).
//   So we only need the right-top-back point `bboxRTB` to determine the
//   bounding box.
// [data]
//   Required volume data is .raw file with 16-bit Unsigned LittleEndian
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
#include <climits>
#include <thread>
#include <map>
#include <functional>
#include <algorithm>
#include <mutex>
#include <algorithm>
#include <cstdio>

#include "../framework/ReNow.hpp"
#include "../framework/Utils.hpp"
#include "../framework/Camera.hpp"

using namespace zx;

using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;
using rapidjson::Document;
using std::cerr;
using std::cout;
using std::endl;
using std::function;
using std::map;
using std::mutex;
using std::sort;
using std::thread;
using std::vector;

// ### Here are parameters that should be set, read, or calculated. ###
const string CONFIG_FILE = "./config.json";
// [Display Window]
int WINDOW_WIDTH, WINDOW_HEIGHT;

// [Volume Data Metainfo]
string VolumePath;                           // volume raw file path
int VolumeWidth, VolumeHeight, VolumeZCount; // x, y, z (thickness)
vec3 bboxRTB;                                // bounding box Right-Top-Back
int PixelPerSlice, VoxelCount;
uint16 *volumeData;           // volume data itself
RGBAColor *coloredVolumeData; // after coloring using transfer function (TF)

// [Image Plane]
int ImagePlaneWidth, ImagePlaneHeight, ImagePlaneSize;
RGBAColor *imagePlane; // image plane itself
int MedianFilterKSize;

// [Transfer Function]
string TransferFunctionName;
// Register your transfer function here.
map<string, function<void(const uint16 *vol, int vCount, RGBAColor *res)>>
    transferFunctionMap = {
        {"TF_CT_Bone", TF_CT_Bone},
        {"TF_CT_MuscleAndBone", TF_CT_MuscleAndBone},
        {"TF_CT_Skin", TF_CT_Skin},
};

// [Multi Thread]
int multiThread;      // num_workers
float sharePerThread; // proportion of workload per thread
mutex multiThreadMutex;

// [Progress Bar]
float progress = 0.0;
float progressPerThreadPerRow;

// [Ray Casting]
int intersectCount = 0; // # ray intersects with bounding box
int currentTexId = -1;  // casting result image plane is stored as texture
#define INTERSECT_EPSILON 1e-6 // error control for intersect test
float SamplingDelta;           // step of voxel sampling, coarse: 1, finer: 0.5

// [Observation]
vec3 eyePos;                           // current eye position
vec3 normalizedEyePos(0, 0, 1);        // for lookAt matrix calculation
const vec3 initEyeDirection(0, 0, -1); // look towards -z by default
mat3 currentRotateMatrix;              // current rotate matrix
#define ARROW_KEY_TRACEBALL_DELTA 0.2  // increment for arrow key pressing
#define WS_KEY_FRONTBACK_DELTA 16 // increment for W/S key pressing at Z-axis
void keyboardCallback(GLFWwindow *window, int key, int _, int action, int __);

// [Lighting]
bool EnableLighting;                 // whether to apply lighting to volume data
const vec3 lightDirection(0, 0, -1); // same as default look-toward
const RGBColor
    ambientColor(normalizeRGBColor(RGBWhite)); // white light by default
const RGBColor
    diffuseColor(normalizeRGBColor(RGBWhite)); // white light by default
float KAmbient;                                // weight for ambient component

// [ReNow Helper]
ReNowHelper helper;
// ### Here are parameters that should be set, read, or calculated. ###

// Truncate the translation vector (i.e. the last column) in lookAt matrix,
// since we handle the translation manually with x'=xR+t
mat3 UntranslatedLookAt(vec3 eye, vec3 at, vec3 up) {
  return mat3(glm::lookAt(eye, at, up));
}

// Load config file from CONFIG_FILE, and calculate some parameters.
void loadConfigFileAndInitialize() {
  Document d;
  d.Parse(readFileText(CONFIG_FILE).c_str());

  WINDOW_WIDTH = d["WindowWidth"].GetInt();
  WINDOW_HEIGHT = d["WindowHeight"].GetInt();

  VolumePath = d["VolumePath"].GetString();
  VolumeWidth = d["VolumeWidth"].GetInt();
  VolumeHeight = d["VolumeHeight"].GetInt();
  VolumeZCount = d["VolumeZCount"].GetInt();
  // far enough to hold the whole volume in view
  eyePos = vec3(0, 0, VolumeZCount + 1);

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
  MedianFilterKSize = d["MedianFilterKSize"].GetInt();
  SamplingDelta = d["SamplingDelta"].GetFloat();
  EnableLighting = d["EnableLighting"].GetBool();
  KAmbient = d["KAmbient"].GetFloat();

  multiThread = d["MultiThread"].GetInt();
  sharePerThread = 1.0 / multiThread;
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
  defaultValue = 0;
  float x1 = x > 0 ? getVoxel(x - 1, y, z) : defaultValue,
        x2 = x < VolumeWidth - 1 ? getVoxel(x + 1, y, z) : defaultValue,
        y1 = y > 0 ? getVoxel(x, y - 1, z) : defaultValue,
        y2 = y < VolumeHeight - 1 ? getVoxel(x, y + 1, z) : defaultValue,
        z1 = z > 0 ? getVoxel(x, y, z - 1) : defaultValue,
        z2 = z < VolumeZCount - 1 ? getVoxel(x, y, z + 1) : defaultValue;
  // normalized normal equal to normalized gradient
  vec3 gradient(x1 - x2, y1 - y2, z1 - z2);
  vec3 gradientNorm = glm::normalize(gradient);
  // handle numerical precision error
  if (isnan(gradientNorm.x) || isnan(gradientNorm.y) || isnan(gradientNorm.z)) {
    return gradient;
  }
  // we prefer a normalized normal
  return gradientNorm;
}

// Apply transfer function to fill `coloredVolumeData`.
void applyTransferFunction(const string &name) {
  ASSERT(transferFunctionMap.count(name) != 0,
         "[ERROR] Invalid transfer function: " + TransferFunctionName);
  transferFunctionMap[name](volumeData, VoxelCount, coloredVolumeData);
}

// Get interpolated color of pos using TriLinear method.
RGBAColor colorInterpTriLinear(const vec3 &pos, const vec3 &bboxRTB) {
  int x0, y0, z0, x1, y1, z1; // integer positions
  float xd, yd, zd;           // remainders
  RGBAColor res;

  x0 = int(pos.x);
  xd = pos.x - x0;
  x1 = x0 + 1;
  x1 = x1 > bboxRTB.x ? x1 - 1 : x1;

  y0 = int(pos.y);
  yd = pos.y - y0;
  y1 = y0 + 1;
  y1 = y1 > bboxRTB.y ? y1 - 1 : y1;

  z0 = int(pos.z);
  zd = pos.z - z0;
  z1 = z0 + 1;
  z1 = z1 > bboxRTB.z ? z1 - 1 : z1;

  res =
      (1 - xd) * (1 - yd) * (1 - zd) *
          coloredVolumeData[getVoxelIndex(x0, y0, z0)] +
      xd * (1 - yd) * (1 - zd) * coloredVolumeData[getVoxelIndex(x1, y0, z0)] +
      (1 - xd) * yd * (1 - zd) * coloredVolumeData[getVoxelIndex(x0, y1, z0)] +
      (1 - xd) * (1 - yd) * zd * coloredVolumeData[getVoxelIndex(x0, y0, z1)] +
      xd * yd * (1 - zd) * coloredVolumeData[getVoxelIndex(x1, y1, z0)] +
      xd * (1 - yd) * zd * coloredVolumeData[getVoxelIndex(x1, y0, z1)] +
      (1 - xd) * yd * zd * coloredVolumeData[getVoxelIndex(x0, y1, z1)] +
      xd * yd * zd * coloredVolumeData[getVoxelIndex(x1, y1, z1)];

  zx::clipRGBA(res);
  return res;
}

// Fusion `sample` color into `accumalted` using front-to-back iteration method.
void fusionColorFrontToBack(RGBAColor &accmulated, const RGBAColor &sample) {
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

// Helper function for `bool intersectTest`.
bool _intersectTestOnePlane(const float origin, const float direction,
                            const float bboxRTB, double &t0Final,
                            double &t1Final) {
  double bMin = 0, bMax = bboxRTB;
  double t0, t1;

  if (fabs(direction) > INTERSECT_EPSILON) { // direction != 0
    t0 = (bMin - origin) / direction;
    t1 = (bMax - origin) / direction;
    if (t0 > t1) {
      zx::swap<double>(t0, t1);
    }
    t0Final = std::max(t0Final, t0);
    t1Final = std::min(t1Final, t1);

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
// the entry point (or first intersect point exactly) is returned using &entry
// with parameter &t
bool intersectTest(const vec3 &origin, const vec3 &direction,
                   const vec3 &bboxRTB, vec3 &entry, float &t) {
  // t0 is the parameter of entry, while t1 is of exit
  double t0Final = -(DBL_MAX - 1), t1Final = DBL_MAX - 1;

  if (!_intersectTestOnePlane(origin.x, direction.x, bboxRTB.x, t0Final,
                              t1Final) ||
      !_intersectTestOnePlane(origin.y, direction.y, bboxRTB.y, t0Final,
                              t1Final) ||
      !_intersectTestOnePlane(origin.z, direction.z, bboxRTB.z, t0Final,
                              t1Final)) {
    return false;
  }

  // If t0Final > 0, there are two intersect points. So t0Final is for exact
  // entry. But if t0Final < 0, then there is only exit, which means we are
  // currently inside the bounding box. So we return source point instead.
  // Then that we can cast the ray from here.
  t = t0Final >= 0 ? t0Final : 0;
  entry = vec3(origin + t * direction);

  return true;
}

// Implement of a simple progressbar.
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

// Apply lighting at sample point.
void applyLighting(RGBAColor &src, const vec3 &pos) {
  // using simplified Phong (without specular)
  vec3 normal = calcNormal(pos.x, pos.y, pos.z);
  vec3 rgb(src.r, src.g, src.b);
  float kDiffuse = std::max(glm::dot(normal, lightDirection), 0.0f);
  vec3 colored = (kDiffuse * diffuseColor + KAmbient * ambientColor) * rgb;
  zx::clipRGB(colored);
  for (int i = 0; i < 3; i++) {
    src[i] = colored[i];
  }
}

// Cast one ray corresponding to (u, v) at image plane
// according to `coloredVolumeData`. Returns the fused (blended) RGBAColor.
void castOneRay(int u, int v, const vec3 &bboxRTB, const mat3 &rotateMat,
                const vec3 &translateVec,
                const RGBAColor &defaultColor = RGBAColor(RGBBlack, 1.0)) {
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
  float paramT; // parameter t of entry or exit point (first intersect point)

  if (intersectTest(source, direction, bboxRTB, entry, paramT)) {
    // initialize samplePos
    samplePos = entry;
    // march the ray
    while (inBBox(samplePos, bboxRTB) && accumulated.a < 1.0) {
      // get sampleColor via interpolation
      sampleColor = colorInterpTriLinear(samplePos, bboxRTB);
      // light it
      if (EnableLighting) {
        applyLighting(sampleColor, samplePos);
      }
      // Cout = Cin + (1-ain)aiCi
      fusionColorFrontToBack(accumulated, sampleColor);
      // go forward
      samplePos += SamplingDelta * direction;
    }
    // fill the image plane
    zx::clipRGBA(accumulated);
    imagePlane[getPixelIndex(v, u)] = RGBAColor(accumulated);
    // record intersect count
    multiThreadMutex.lock();
    intersectCount++;
    multiThreadMutex.unlock();
  } else {
    imagePlane[getPixelIndex(v, u)] = RGBAColor(defaultColor);
  }
}

// Written in this form to support multi-thread processing.
void castSomeRays(int rowLow, int rowHigh) {
  for (int r = rowLow; r < rowHigh; r++) {
    for (int c = 0; c < ImagePlaneWidth; c++) {
      castOneRay(c, r, bboxRTB, currentRotateMatrix, eyePos);
    }
    if (rowLow == 0) { // the first thread
      updateProgressBar(progress = progress + progressPerThreadPerRow);
    }
  }
}

// Multi-thread Ray Casting
void castAllRays() {
  vector<thread> threadPool;
  for (int i = 0; i < multiThread; i++) {
    int low = int(sharePerThread * i * ImagePlaneHeight);
    int high = i == multiThread - 1
                   ? ImagePlaneHeight
                   : int(sharePerThread * (i + 1) * ImagePlaneHeight);
    threadPool.push_back(thread(castSomeRays, low, high));
  }
  for_each(threadPool.begin(), threadPool.end(), [=](thread &t) { t.join(); });
}

// Single-thread Ray Casting (for debug only)
void castAllRaysSingleThread() { castSomeRays(0, ImagePlaneHeight); }

// Median filtering the image plane.
void medianFilter(int ksize) {
  RGBAColor *tempRes = new RGBAColor[ImagePlaneSize]; // alloc on demand
  vector<vec4> vs;
  int pad = (ksize - 1) / 2;
  int middle = (ksize * ksize - 1) / 2;
  for (int i = pad; i < ImagePlaneHeight - pad; i++) {
    for (int j = pad; j < ImagePlaneWidth - pad; j++) {
      vs.clear();
      for (int k = -pad; k <= pad; k++) {
        for (int l = -pad; l <= pad; l++) {
          vs.push_back(RGBAColor(imagePlane[getPixelIndex(i + k, j + l)]));
        }
      }
      sort(vs.begin(), vs.end(), [=](const RGBAColor &a, const RGBAColor &b) {
        return (a.r + a.g + a.b) < (b.r + b.g + b.b);
      });
      tempRes[getPixelIndex(i, j)] = vs.at(middle);
    }
  }
  std::copy(tempRes, tempRes + ImagePlaneSize, imagePlane);
  delete tempRes;
}

// The very main
void rayCasting() {
  // prepare for a new rendering
  intersectCount = 0;
  progress = 0.0;

  cout << ">>> Restart ray casting using " << multiThread << " threads..."
       << endl;
  time_t tic = time(NULL);
  castAllRays();
  time_t toc = time(NULL);

  // release previous texture
  if (currentTexId != -1) {
    GL_OBJECT_ID x = currentTexId;
    glDeleteTextures(1, &x);
  }
  // check if we need to perform median filtering
  if (MedianFilterKSize > 0) {
    ASSERT(MedianFilterKSize % 2 == 1, "MedianFilterKSize should be odd.");
    cout << endl << "Performing Median Filtering..." << endl;
    medianFilter(MedianFilterKSize);
  }
  // store image plane in a new texture
  currentTexId = helper.createTexture2D(imagePlane, GL_RGBA, ImagePlaneWidth,
                                        ImagePlaneHeight, GL_FLOAT);

  // If we need to save image plane as file: (for debug only)
  // FILE *fp;
  // fopen_s(&fp, "./ImagePlane.raw", "wb");
  // for (int i = 0; i < ImagePlaneSize; i++) {
  //   fwrite(&(imagePlane[i].r), sizeof(float), 1, fp);
  //   fwrite(&(imagePlane[i].g), sizeof(float), 1, fp);
  //   fwrite(&(imagePlane[i].b), sizeof(float), 1, fp);
  //   // discard alpha channel
  // }
  // fclose(fp);

  cout << endl
       << "# Ray intersect: " << intersectCount << endl
       << "# Ray cast: " << ImagePlaneSize << endl
       << "Time elapsed: " << toc - tic << " secs." << endl
       << endl
       << ">>> Start rendering..." << endl
       << endl;
  helper.prepareUniforms(vector<UPrepInfo>{{"uTexture", currentTexId, "1i"}});
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
  loadConfigFileAndInitialize();

  GLFWwindow *window =
      initGLWindow("Project 2 - Ray Casting / Zhuo Xu 212138 SEU", WINDOW_WIDTH,
                   WINDOW_HEIGHT);
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
      // recalculate rotate matrix
      currentRotateMatrix =
          UntranslatedLookAt(normalizedEyePos, WORLD_ORIGIN, VEC_UP);
      // recast rays
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

// Support observation
void keyboardCallback(GLFWwindow *window, int key, int _, int action, int __) {
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_LEFT) {
      // go left
      if (normalizedEyePos.x >= -1) {
        normalizedEyePos.x -= ARROW_KEY_TRACEBALL_DELTA;
        shouldReCast = true;
      }
    } else if (key == GLFW_KEY_RIGHT) {
      // go right
      if (normalizedEyePos.x <= 1) {
        normalizedEyePos.x += ARROW_KEY_TRACEBALL_DELTA;
        shouldReCast = true;
      }
    } else if (key == GLFW_KEY_UP) {
      // go top
      if (normalizedEyePos.y >= -1) {
        normalizedEyePos.y -= ARROW_KEY_TRACEBALL_DELTA;
        shouldReCast = true;
      }
    } else if (key == GLFW_KEY_DOWN) {
      // go bottom
      if (normalizedEyePos.y <= 1) {
        normalizedEyePos.y += ARROW_KEY_TRACEBALL_DELTA;
        shouldReCast = true;
      }
    } else if (key == GLFW_KEY_W) {
      // go forward
      if (eyePos.z > WS_KEY_FRONTBACK_DELTA) {
        eyePos.z -= WS_KEY_FRONTBACK_DELTA;
        shouldReCast = true;
      }
    } else if (key == GLFW_KEY_S) {
      // go backward
      if (eyePos.z < VolumeZCount + 1) {
        eyePos.z += WS_KEY_FRONTBACK_DELTA;
        shouldReCast = true;
      }
    }
  }
}