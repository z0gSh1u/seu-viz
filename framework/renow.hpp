#pragma once
#ifndef RENOW_HPP_
#define RENOW_HPP_
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <regex>
#include <string>
#include <vector>

#include "OBJProcessor.hpp"
#include "Utils.hpp"
using glm::vec2;
using glm::vec3;
using std::string;
using std::vector;

typedef GLuint GL_SHADER_ID;
typedef GLuint GL_PROGRAM_ID;
typedef GLuint GL_OBJECT_ID;

GLFWwindow *initGL(string windowName, int canvasWidth, int canvasHeight) {
  glfwInit();
  GLFWwindow *window = glfwCreateWindow(canvasWidth, canvasHeight,
                                        windowName.c_str(), NULL, NULL);
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  return window;
}

Vec3s analyzeFtoV(const OBJProcessor &proc) {
  Vec3s meshVertices; // [[x, y, z]]
  for (const vec3 &face : proc.fs()) {
    int v1 = face.x - 1, v2 = face.y - 1, v3 = face.z - 1;
    meshVertices.push_back(proc.vs().at(v1));
    meshVertices.push_back(proc.vs().at(v2));
    meshVertices.push_back(proc.vs().at(v3));
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

#endif