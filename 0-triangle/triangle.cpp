#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>

#include <iostream>
using std::cout;
using std::endl;

#include <cassert>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600

using std::ifstream;
using std::string;
using std::stringstream;

typedef GLuint GL_SHADER_ID;
typedef GLuint GL_PROGRAM_ID;
typedef GLuint GL_OBJECT_ID;

GLFWwindow* initGL(int canvasWidth, int canvasHeight) {
  glfwInit();
  GLFWwindow* window =
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
  GLFWwindow* window = initGL(WINDOW_WIDTH, WINDOW_HEIGHT);

  // build shader
  // vertex shader
  GL_SHADER_ID vShader = glCreateShader(GL_VERTEX_SHADER);
  string vShaderContent_ = readFile("F:/seu-viz/0-triangle/shader/vMain.glsl");
  char* vShaderContent = new char[vShaderContent_.size() + 1];
  memcpy(vShaderContent, vShaderContent_.c_str(), vShaderContent_.size() + 1);
  glShaderSource(vShader, 1, &vShaderContent, NULL);
  glCompileShader(vShader);
  // fragment shader
  GL_SHADER_ID fShader = glCreateShader(GL_FRAGMENT_SHADER);
  string fShaderContent_ = readFile("F:/seu-viz/0-triangle/shader/fMain.glsl");
  char* fShaderContent = new char[fShaderContent_.size() + 1];
  memcpy(fShaderContent, fShaderContent_.c_str(), fShaderContent_.size() + 1);
  glShaderSource(fShader, 1, &fShaderContent, NULL);
  glCompileShader(fShader);

  // build program
  GL_PROGRAM_ID program = glCreateProgram();
  glAttachShader(program, vShader);
  glAttachShader(program, fShader);
  glLinkProgram(program);
  // glDeleteShader(vShader);
  // glDeleteShader(fShader);
  glUseProgram(program);
  glClearColor(0.9, 0.9, 0.9, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  // glEnable(GL_DEPTH_TEST);

  // triangle data
  float vs[] = { -0.5, -0.5, 0.5, -0.5, 0, 0.5 };
  float cs[] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };

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

  // main loop
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    // send data
    // bind these to the state machine
    // glBindVertexArray(cArray);
    glBindBuffer(GL_ARRAY_BUFFER, cBuffer);
    // tell OpenGL how to explain the data sent later
    auto aColorLoc = glGetAttribLocation(program, "aColor");
    assert(aColorLoc != -1);
    glVertexAttribPointer(aColorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(aColorLoc);
    // send the data
    glBufferData(GL_ARRAY_BUFFER, sizeof(cs), cs, GL_STATIC_DRAW);

    // send data
    // bind these to the state machine
    // glBindVertexArray(vArray);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // tell OpenGL how to explain the data sent later
    auto aPositionLoc = glGetAttribLocation(program, "aPosition");
    assert(aPositionLoc != -1);
    glVertexAttribPointer(aPositionLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(aPositionLoc);
    // send the data
    glBufferData(GL_ARRAY_BUFFER, sizeof(vs), vs, GL_STATIC_DRAW);
    // glBindVertexArray(0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}