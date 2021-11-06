#pragma once

// Utilities of ReNow Framework
// by z0gSh1u @ https://github.com/z0gSh1u/seu-viz
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

using glm::vec2;
using glm::vec3;
using glm::vec4;

using std::ifstream;
using std::regex;
using std::string;
using std::stringstream;
using std::vector;

typedef vector<vec2> Vec2s;
typedef vector<vec3> Vec3s;
typedef vec3 RGBColor;

namespace zx {

const RGBColor RGBWhite = RGBColor(255, 255, 255);
const double PI = 3.1415926535;

// Ensure `ensure`, else terminate and output hint.
void ASSERT(bool ensure, const string &hint) {
  if (!ensure) {
    std::cerr << hint << std::endl;
    assert(0);
  }
}

// Normalize 8bit color to RGBA (0~1).
vec4 normalize8bitColor(RGBColor rgb) {
  return vec4(rgb.r / 255.0, rgb.g / 255.0, rgb.b / 255.0, 1.0);
}

// Read file content as std::string.
string readFile(string filePath) {
  ifstream s(filePath);
  stringstream buf;
  buf << s.rdbuf();
  return buf.str();
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

// Convert DEG to RAD.
float radians(float deg) { return deg / 180 * PI; }

// Convert RGBColor to string for debug.
string rgbColorToString(const RGBColor &color) {
  stringstream ss;
  ss << "[" << color.r << ", " << color.g << ", " << color.b << "]";
  return ss.str();
}

// MIN-MAX clip
float minmaxClip(float v, float min, float max) {
  return v < min ? min : v > max ? max : v;
}

} // namespace zx

#endif