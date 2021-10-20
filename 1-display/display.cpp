#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

#include <iostream>
using std::cout;
using std::endl;

#include "../framework/OBJProcessor.hpp"
#include "../framework/ReNow.hpp"
#include "../framework/Utils.hpp"
#include <cassert>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600

int main() {
  // init canvas
  GLFWwindow *window = initGL("canvas", WINDOW_WIDTH, WINDOW_HEIGHT);

  // build shader
  // vertex shader
  GL_SHADER_ID vShader = glCreateShader(GL_VERTEX_SHADER);
  string vShaderContent_ = readFile("F:/seu-viz/1-display/shader/vMain.glsl");
  char *vShaderContent = new char[vShaderContent_.size() + 1];
  memcpy(vShaderContent, vShaderContent_.c_str(), vShaderContent_.size() + 1);
  glShaderSource(vShader, 1, &vShaderContent, NULL);
  glCompileShader(vShader);
  // fragment shader
  GL_SHADER_ID fShader = glCreateShader(GL_FRAGMENT_SHADER);
  string fShaderContent_ = readFile("F:/seu-viz/1-display/shader/fMain.glsl");
  char *fShaderContent = new char[fShaderContent_.size() + 1];
  memcpy(fShaderContent, fShaderContent_.c_str(), fShaderContent_.size() + 1);
  glShaderSource(fShader, 1, &fShaderContent, NULL);
  glCompileShader(fShader);

  // build program
  GL_PROGRAM_ID program = glCreateProgram();
  glAttachShader(program, vShader);
  glAttachShader(program, fShader);
  glLinkProgram(program);
  glUseProgram(program);
  glClearColor(0, 0, 0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT);

  // prepare data
  float cs[] = {0.0, 0.0, 1.0};

  OBJProcessor proc(readFile("F:/seu-viz/1-display/model/UnitSphere.obj"));
  vector<float> _vs = flattenVec3s(analyzeFtoV(proc));
  cout << "vs len = " << _vs.size() * sizeof(float) << endl;
  float *vs = &_vs[0];

  // generate arrays to store the data
  GL_OBJECT_ID vArray, cArray;
  glGenVertexArrays(1, &vArray);
  glGenVertexArrays(1, &cArray);
  // generate buffers to store the array temporarily
  GL_OBJECT_ID vBuffer, cBuffer;
  glGenBuffers(1, &vBuffer);
  glGenBuffers(1, &cBuffer);

  // unbind
  // glEnableVertexAttribArray(0);
  // glBindVertexArray(0);

  // cout << sizeof(vs) << endl;

  // main loop
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    // send data
    // bind these to the state machine
    // glBindVertexArray(cArray);
    // glBindBuffer(GL_ARRAY_BUFFER, cBuffer);
    // tell OpenGL how to explain the data sent later
    // auto aColorLoc = glGetAttribLocation(program, "aColor");
    // assert(aColorLoc != -1);
    // glVertexAttribPointer(aColorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    // glEnableVertexAttribArray(aColorLoc);
    // // send the data
    // glBufferData(GL_ARRAY_BUFFER, sizeof(cs), cs, GL_STATIC_DRAW);

    // send data
    // bind these to the state machine
    // glBindVertexArray(vArray);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // tell OpenGL how to explain the data sent later
    auto aPositionLoc = glGetAttribLocation(program, "aPosition");
    assert(aPositionLoc != -1);
    glVertexAttribPointer(aPositionLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(aPositionLoc);
    // send the data
    glBufferData(GL_ARRAY_BUFFER, _vs.size() * sizeof(float), vs,
                 GL_STATIC_DRAW);
    // glBindVertexArray(0);

    glDrawArrays(GL_TRIANGLES, 0, proc.getEffectiveVertexCount());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}