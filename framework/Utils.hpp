#pragma once
#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <regex>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using std::ifstream;
using std::string;
using std::stringstream;

using glm::vec2;
using glm::vec3;
using glm::vec4;
using std::regex;
using std::string;
using std::vector;

typedef vector<vec2> Vec2s;
typedef vector<vec3> Vec3s;
typedef vec3 RGBColor;

vec4 normalize8bitColor(RGBColor rgb) {
  return vec4(rgb.r / 255.0, rgb.g / 255.0, rgb.b / 255.0, 1.0);
}

string readFile(string filePath) {
  ifstream s(filePath);
  stringstream buf;
  buf << s.rdbuf();
  return buf.str();
}

bool stringStartsWith(const string &str, const string &prefix) {
  return str.find(prefix) == 0;
}

vector<string> stringSplit(const string &str, const std::regex &delim) {
  auto lIter = std::sregex_token_iterator(str.begin(), str.end(), delim, -1);
  auto rIter = std::sregex_token_iterator();
  auto res = vector<string>(lIter, rIter);
  return res;
}

vector<float> mapParseFloat(const vector<string> &strs, size_t l = 0) {
  vector<float> res;
  for (size_t i = l; i < strs.size(); i++) {
    res.push_back(atof(strs.at(i).c_str()));
  }
  return res;
}

vector<int> mapParseInt(const vector<string> &strs, size_t l = 0) {
  vector<int> res;
  for (size_t i = l; i < strs.size(); i++) {
    res.push_back(atoi(strs.at(i).c_str()));
  }
  return res;
}

#endif