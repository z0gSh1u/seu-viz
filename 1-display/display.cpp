#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

#include <iostream>
using std::cout;
using std::endl;

#include <cassert>

#include "../framework/ReNow.hpp"
#include "../framework/OBJProcessor.hpp"
#include "../framework/Utils.hpp"
#include "../framework/PhongLightModel.hpp"

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>

using glm::mat2;
using glm::mat3;
using glm::mat4;
using glm::value_ptr;
using glm::vec2;
using glm::vec3;
using glm::vec4;

int main() {
  // init canvas
  GLFWwindow *window = initGL("canvas", WINDOW_WIDTH, WINDOW_HEIGHT);

  // build shader
  // vertex shader
  GL_SHADER_ID vShader =
      createShader(GL_VERTEX_SHADER, "F:/seu-viz/1-display/shader/vMain.glsl");

  // fragment shader
  GL_SHADER_ID fShader = createShader(GL_FRAGMENT_SHADER,
                                      "F:/seu-viz/1-display/shader/fMain.glsl");

  // build program
  GL_PROGRAM_ID program = glCreateProgram();
  glAttachShader(program, vShader);
  glAttachShader(program, fShader);
  glLinkProgram(program);
  glUseProgram(program);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT);

  // prepare data
  float cs[] = {0.0, 0.0, 1.0};

  OBJProcessor proc(readFile("F:/seu-viz/1-display/model/UnitSphere.obj"));
  vector<float> _vs = flattenVec3s(analyzeFtoVfs(proc));
  float *vs = &_vs[0];
  vector<float> _vns = flattenVec3s(analyzeFtoVfns(proc));
  float *vns = &_vns[0];

  // generate buffers to store the array temporarily
  GL_OBJECT_ID vBuffer, vnBuffer;
  glGenBuffers(1, &vBuffer);
  glGenBuffers(1, &vnBuffer);

  // set uniforms
  mat4 worldMatrix = eye4(); // eye
  mat4 modelMatrix = glm::scale(eye4(), vec3(0.8, 0.8, 0.8));
  vec3 lightPos = vec3(0.9, 0.9, 0);
  vec3 ambientColor(255, 255, 255);
  vec3 ambientMaterial(200, 200, 200);
  vec3 diffuseColor(255, 255, 255);
  vec3 diffuseMaterial(66, 66, 66);
  vec3 specularColor(255, 255, 255);
  vec3 specularMaterial(200, 200, 200);
  float materialShiness = 30.0;
  PhongLightModel colorModel(lightPos, ambientColor, ambientMaterial,
                             diffuseColor, diffuseMaterial, specularColor,
                             specularMaterial, materialShiness);

  glUniformMatrix4fv(getUniformLocation(program, "uWorldMatrix"), 1, GL_FALSE,
                     value_ptr(worldMatrix));
  glUniformMatrix4fv(getUniformLocation(program, "uModelMatrix"), 1, GL_FALSE,
                     value_ptr(modelMatrix));

  glUniform4fv(getUniformLocation(program, "uAmbientProduct"), 1,
               value_ptr(colorModel.ambientProduct()));
  glUniform4fv(getUniformLocation(program, "uDiffuseProduct"), 1,
               value_ptr(colorModel.diffuseProduct()));
  glUniform4fv(getUniformLocation(program, "uSpecularProduct"), 1,
               value_ptr(colorModel.specularProduct()));
  glUniform1f(getUniformLocation(program, "uShiness"), materialShiness);
  glUniformMatrix4fv(
      getUniformLocation(program, "uWorldMatrixTransInv"), 1, GL_FALSE,
      value_ptr(glm::inverse(glm::transpose(
          mat3(worldMatrix[0][0], worldMatrix[0][1], worldMatrix[0][2],
               worldMatrix[1][0], worldMatrix[1][1], worldMatrix[1][2],
               worldMatrix[2][0], worldMatrix[2][1], worldMatrix[2][2])))));
  float delta = 0.01;

  // main loop
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    // change light position
    float z = lightPos.z;
    if (z > 1 || z < -1) {
      delta *= -1;
    }
    lightPos.z = z + delta;

    glUniform4fv(getUniformLocation(program, "uLightPosition"), 1,
                 value_ptr(vec4(lightPos, 1.0)));

    // send data
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // tell OpenGL how to explain the data sent later
    auto aPositionLoc = glGetAttribLocation(program, "aPosition");
    assert(aPositionLoc != -1);
    glVertexAttribPointer(aPositionLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(aPositionLoc);
    // send the data
    glBufferData(GL_ARRAY_BUFFER, _vs.size() * sizeof(float), vs,
                 GL_STATIC_DRAW);

    // send data
    glBindBuffer(GL_ARRAY_BUFFER, vnBuffer);
    // tell OpenGL how to explain the data sent later
    auto aNormalLoc = glGetAttribLocation(program, "aNormal");
    assert(aNormalLoc != -1);
    glVertexAttribPointer(aNormalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(aNormalLoc);
    // send the data
    glBufferData(GL_ARRAY_BUFFER, _vns.size() * sizeof(float), vns,
                 GL_STATIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, proc.getEffectiveVertexCount());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}