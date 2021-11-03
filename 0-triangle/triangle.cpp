// ################################################
// 三色三角形绘制
// 用于熟悉OpenGL的基本使用
// by z0gSh1u @ https://github.com/z0gSh1u/seu-viz
// ################################################

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include <iostream>

using std::cout;
using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600

typedef GLuint GL_SHADER_ID;
typedef GLuint GL_PROGRAM_ID;
typedef GLuint GL_OBJECT_ID;

GLFWwindow *initGL(int canvasWidth, int canvasHeight) {
  glfwInit();
  GLFWwindow *window =
      glfwCreateWindow(canvasWidth, canvasHeight, "canvas", NULL, NULL);
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  return window;
}

string readFile(string filePath) {
  ifstream s(filePath);
  stringstream buf;
  buf << s.rdbuf();
  return buf.str();
}

int main() {
  // init canvas
  GLFWwindow *window = initGL(WINDOW_WIDTH, WINDOW_HEIGHT);

  // build shader
  // vertex shader
  GL_SHADER_ID vShader = glCreateShader(GL_VERTEX_SHADER);
  string vShaderContent_ = readFile("./shader/vMain.glsl");
  char *vShaderContent = new char[vShaderContent_.size() + 1];
  memcpy(vShaderContent, vShaderContent_.c_str(), vShaderContent_.size() + 1);
  glShaderSource(vShader, 1, &vShaderContent, NULL);
  glCompileShader(vShader);
  // fragment shader
  GL_SHADER_ID fShader = glCreateShader(GL_FRAGMENT_SHADER);
  string fShaderContent_ = readFile("./shader/fMain.glsl");
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
  glClearColor(0.9, 0.9, 0.9, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  // triangle data
  float vs[] = {-0.5, -0.5, 0.5, -0.5, 0, 0.5};
  float cs[] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};

  // generate buffers to store the array temporarily
  GL_OBJECT_ID vBuffer, cBuffer;
  glGenBuffers(1, &vBuffer);
  glGenBuffers(1, &cBuffer);

  // main loop
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    // send color data
    // bind buffer to the state machine
    glBindBuffer(GL_ARRAY_BUFFER, cBuffer);
    // tell OpenGL how to explain the data sent later
    auto aColorLoc = glGetAttribLocation(program, "aColor");
    assert(aColorLoc != -1);
    glVertexAttribPointer(aColorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(aColorLoc);
    // send the data
    glBufferData(GL_ARRAY_BUFFER, sizeof(cs), cs, GL_STATIC_DRAW);

    // the same
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    auto aPositionLoc = glGetAttribLocation(program, "aPosition");
    assert(aPositionLoc != -1);
    glVertexAttribPointer(aPositionLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(aPositionLoc);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vs), vs, GL_STATIC_DRAW);

    // draw
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // loop
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}