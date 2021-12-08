#pragma once

// Utilities of ReNow Framework
// by z0gSh1u (Zhuo Xu) @ https://github.com/z0gSh1u/seu-viz
// See:
// https://github.com/z0gSh1u/typed-webgl
// https://github.com/z0gSh1u/renow-ts
// https://github.com/z0gSh1u/renow-c

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <glm/glm.hpp>
#include <regex>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include "ReNow.hpp"

using glm::vec2;
using glm::vec3;
using glm::vec4;

using std::ifstream;
using std::ios;
using std::regex;
using std::string;
using std::stringstream;
using std::vector;
using std::runtime_error;

// type alias
typedef GLuint GL_SHADER_ID;
typedef GLuint GL_PROGRAM_ID;
typedef GLuint GL_OBJECT_ID;
typedef GLuint GL_SHADER_TYPE_;
typedef GLint GL_UNIFORM_LOC;
typedef GLint GL_ATTRIB_LOC;
typedef unsigned char Byte;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef vector<vec2> Vec2s;
typedef vector<vec3> Vec3s;
typedef vec3 RGBColor;
typedef vec4 RGBAColor;

namespace zx {

const RGBColor RGBWhite = RGBColor(255, 255, 255);
const RGBColor RGBBlack = RGBColor(0, 0, 0);
const RGBColor RGBGray = RGBColor(127, 127, 127);
const RGBAColor Transparent = RGBAColor(0, 0, 0, 0);
const double PI = 3.1415926535;

// Ensure `ensure`, else throw and output hint.
void ASSERT(bool ensure, const string &hint) {
  if (!ensure) {
    std::cerr << hint << std::endl;
    throw runtime_error(hint);
  }
}

// Normalize 8bit RGB color to RGBA (0~1), alpha=1.0
vec4 normalizeRGBColor(RGBColor rgb) {
  return vec4(rgb.r / 255.0, rgb.g / 255.0, rgb.b / 255.0, 1.0);
}

// Normalize 8bit RGBA color's RGB components, keep alpha
vec4 normalizeRGBAColor(RGBAColor rgba) {
  return vec4(rgba.r / 255.0, rgba.g / 255.0, rgba.b / 255.0, rgba.a);
}

// Read file content as std::string.
string readFileText(string filePath) {
  ifstream s(filePath);
  stringstream buf;
  buf << s.rdbuf();
  return buf.str();
}

// Read file as binary.
void readFileBinary(string filePath, int elementSize, int elementCount,
                    void *store) {
  FILE *fptr;
  fopen_s(&fptr, filePath.c_str(), "rb");
  fread((char *)store, elementSize, elementCount, fptr);
  fclose(fptr);
}

// stringStartsWith
bool stringStartsWith(const string &str, const string &prefix) {
  return str.find(prefix) == 0;
}

// Split string using `delim` to vector.
vector<string> stringSplit(const string &str, const std::regex &delim) {
  auto lIter = std::sregex_token_iterator(str.begin(), str.end(), delim, -1);
  auto rIter = std::sregex_token_iterator();
  auto res = vector<string>(lIter, rIter);
  return res;
}

// Batch parseInt.
vector<int> mapParseInt(const vector<string> &strs, size_t l = 0) {
  vector<int> res;
  for (size_t i = l; i < strs.size(); i++) {
    res.push_back(atoi(strs.at(i).c_str()));
  }
  return res;
}

// Batch parseFloat.
vector<float> mapParseFloat(const vector<string> &strs, size_t l = 0) {
  vector<float> res;
  for (size_t i = l; i < strs.size(); i++) {
    res.push_back(atof(strs.at(i).c_str()));
  }
  return res;
}

// Linear interp RGB color.
RGBColor colorInterpLinear(int v, int rangeL, int rangeR,
                           const RGBColor &startColor, const vec3 &colorRange) {
  float offset = v - rangeL;
  float ratio = offset / (rangeR - rangeL);
  RGBColor color = startColor + ratio * colorRange;
  color.r = int(color.r);
  color.g = int(color.g);
  color.b = int(color.b);
  return color;
}

// Convert DEG to RAD.
float radians(float deg) { return deg / 180 * PI; }

// Convert Vec3 to string.
string vec3ToString(const vec3 &vec) {
  stringstream ss;
  ss << "[" << vec.x << ", " << vec.y << ", " << vec.z << "]";
  return ss.str();
}

// MIN-MAX clip
float minmaxClip(float v, float min, float max) {
  return v < min ? min : v > max ? max : v;
}

// MIN-MAX clip vec3
void clipRGB(vec3 &rgb, float min = 0, float max = 1) {
  for (int i = 0; i < 3; i++) {
    rgb[i] = zx::minmaxClip(rgb[i], min, max);
  }
}

// MIN-MAX clip vec4
void clipRGBA(vec4 &rgba, float min = 0, float max = 1) {
  for (int i = 0; i < 4; i++) {
    rgba[i] = zx::minmaxClip(rgba[i], min, max);
  }
}

// Swap two values of type T.
template <typename T> void swap(T &a, T &b) {
  T tmp = a;
  a = b;
  b = tmp;
}

} // namespace zx

#endif