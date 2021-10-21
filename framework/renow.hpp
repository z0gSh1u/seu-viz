#pragma once
#ifndef RENOW_HPP_
#define RENOW_HPP_

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <regex>
#include <string>
#include <vector>
#include <cassert>

#include "OBJProcessor.hpp"
#include "Utils.hpp"

using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;

using std::string;
using std::vector;

typedef GLuint GL_SHADER_ID;
typedef GLuint GL_PROGRAM_ID;
typedef GLuint GL_OBJECT_ID;
typedef GLuint GL_SHADER_TYPE_;
typedef GLint GL_UNIFORM_LOC;

GL_UNIFORM_LOC getUniformLocation(GL_PROGRAM_ID program,
                                  const string &varName) {
  GL_UNIFORM_LOC loc = glGetUniformLocation(program, varName.c_str());
  assert(loc != -1);
  return loc;
}

GL_SHADER_ID createShader(GL_SHADER_TYPE_ shaderType, string shaderPath) {
  GL_SHADER_ID shader = glCreateShader(shaderType);
  string _shaderContent = readFile(shaderPath);
  char *shaderContent = new char[_shaderContent.size() + 1];
  memcpy(shaderContent, _shaderContent.c_str(), _shaderContent.size() + 1);
  glShaderSource(shader, 1, &shaderContent, NULL);
  glCompileShader(shader);

  GLint receiver = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &receiver);
  assert(receiver == GL_TRUE);
  return shader;
}

GLFWwindow *initGL(string windowName, int canvasWidth, int canvasHeight) {
  glfwInit();
  GLFWwindow *window = glfwCreateWindow(canvasWidth, canvasHeight,
                                        windowName.c_str(), NULL, NULL);
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  return window;
}

Vec3s analyzeFtoVfs(const OBJProcessor &proc) {
  Vec3s meshVertices; // [[x, y, z]]
  for (const vec3 &face : proc.fs()) {
    int v1 = face.x - 1, v2 = face.y - 1, v3 = face.z - 1;
    meshVertices.push_back(proc.vs().at(v1));
    meshVertices.push_back(proc.vs().at(v2));
    meshVertices.push_back(proc.vs().at(v3));
  }
  return meshVertices;
}

Vec3s analyzeFtoVfns(const OBJProcessor &proc) {
  Vec3s meshVertices; // [[x, y, z]]
  for (const vec3 &face : proc.fns()) {
    int v1 = face.x - 1, v2 = face.y - 1, v3 = face.z - 1;
    meshVertices.push_back(proc.vns().at(v1));
    meshVertices.push_back(proc.vns().at(v2));
    meshVertices.push_back(proc.vns().at(v3));
  }
  return meshVertices;
}

vector<float> flattenVec3s(const Vec3s &vec3s) {
  vector<float> res;
  for (const vec3 &vec : vec3s) {
    res.push_back(vec.x);
    res.push_back(vec.y);
    res.push_back(vec.z);
  }
  return res;
}

mat4 eye4() { return mat4(1.0f); }

mat3 eye3() { return mat3(1.0f); }

#endif