#pragma once

// OBJ File processor of ReNow Framework
// by z0gSh1u @ https://github.com/z0gSh1u/seu-viz
// See:
// https://github.com/z0gSh1u/typed-webgl
// https://github.com/z0gSh1u/renow-ts
// https://github.com/z0gSh1u/renow-c

#ifndef OBJPROCESSOR_HPP_
#define OBJPROCESSOR_HPP_

#include <glm/glm.hpp>
#include <regex>
#include <string>
#include <vector>

#include "Utils.hpp"

using glm::vec2;
using glm::vec3;
using std::regex;
using std::string;
using std::vector;

namespace zx {

// .obj File Processor
class OBJProcessor {
private:
  string _objFileContent;
  vector<string> _splitFileContent;

  Vec3s _vs;  // vertices of mesh
  Vec3s _fs;  // faces of mesh
  Vec2s _vts; // vertices of texture
  Vec3s _fts; // faces of texture
  Vec3s _vns; // vertices of normal
  Vec3s _fns; // faces of normal

public:
  OBJProcessor(string objFileContent);
  ~OBJProcessor();

  // getters
  const Vec3s &vs() const { return _vs; }
  const Vec3s &fs() const { return _fs; }
  const Vec2s &vts() const { return _vts; }
  const Vec3s &fts() const { return _fts; }
  const Vec3s &vns() const { return _vns; }
  const Vec3s &fns() const { return _fns; }

  const int getEffectiveVertexCount() {
    return this->_fs.size() * 3; // as triangle
  }
};

OBJProcessor::OBJProcessor(string objFileContent) {
  this->_objFileContent =
      std::regex_replace(string(objFileContent), std::regex("\r\n"), "\n");
  // split the file into lines
  this->_splitFileContent =
      stringSplit(this->_objFileContent, std::regex("\n"));

  // regex to match various face line format
  regex f123Regex = regex(R"(^f (\d+/\d+/\d+ ){2}(\d+/\d+/\d+)$)");
  regex f12Regex = regex(R"(^f (\d+/\d+ ){2}(\d+/\d+)$)");
  regex f13Regex = regex(R"("^f (\d+/\d+/\d+ ){2}(\d+/\d+/\d+)$)");

  vector<string> lineSplit;
  vector<float> vParsed;
  vector<int> fParsed;

  for (auto &line : this->_splitFileContent) {
    lineSplit = stringSplit(line, regex(R"(\s+)"));
    // mesh vertices
    if (stringStartsWith(line, "v ")) {
      vParsed = mapParseFloat(lineSplit, 1);
      this->_vs.push_back(vec3(vParsed[0], vParsed[1], vParsed[2]));
    }
    // texture vertices
    else if (stringStartsWith(line, "vt ")) {
      vParsed = mapParseFloat(lineSplit, 1);
      this->_vts.push_back(vec2(vParsed[0], vParsed[1]));
    }
    // normal vertices
    else if (stringStartsWith(line, "vn ")) {
      vParsed = mapParseFloat(lineSplit, 1);
      this->_vns.push_back(vec3(vParsed[0], vParsed[1], vParsed[2]));
    }
    // faces
    else if (stringStartsWith(line, "f")) {
      // f 1 1 1
      if (line.find("/") == string::npos) {
        fParsed = mapParseInt(lineSplit, 1);
        this->_fs.push_back(vec3(fParsed[0], fParsed[1], fParsed[2]));
      } else {
        string newLine = std::regex_replace(line.substr(2), regex("/"), " ");
        lineSplit = stringSplit(newLine, regex(R"(\s+)"));
        fParsed = mapParseInt(lineSplit);
        // f 3/3/3 3/3/3 3/3/3
        if (std::regex_match(line, f123Regex)) {
          this->_fs.push_back(vec3(fParsed[0], fParsed[3], fParsed[6]));
          this->_fts.push_back(vec3(fParsed[1], fParsed[4], fParsed[7]));
          this->_fns.push_back(vec3(fParsed[2], fParsed[5], fParsed[8]));
        }
        // f 2/2 2/2 2/2
        else if (std::regex_match(line, f12Regex)) {
          this->_fs.push_back(vec3(fParsed[0], fParsed[2], fParsed[4]));
          this->_fts.push_back(vec3(fParsed[1], fParsed[3], fParsed[5]));

        }
        // f x//x x//x x//x
        else if (std::regex_match(line, f13Regex)) {
          this->_fs.push_back(vec3(fParsed[0], fParsed[2], fParsed[4]));
          this->_fns.push_back(vec3(fParsed[1], fParsed[3], fParsed[5]));
        }
      }
    }
    // do nothing for `usemtl`, `o` and so on
    else {
      {} // noop
    }
  }
}

OBJProcessor::~OBJProcessor() {}

} // namespace zx

#endif