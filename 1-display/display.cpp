// ################################################
// Project 1 - 显示一个球面和正方体表面
// 数据结构、颜色(RGBA)、视见变换（改变视点观察正方体现实）、光照模型（改变镜面反射参数观察球面显示效果）
// by z0gSh1u @ https://github.com/z0gSh1u/seu-viz
// ################################################

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "../framework/ReNow.hpp"
#include "../framework/Camera.hpp"
#include "../framework/PhongLightModel.hpp"

using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

using namespace zx;

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

void consoleLogWelcome();

// handle mouse events
void mouseMoveCallback(GLFWwindow *window, double offsetX, double offsetY);
float mouseX = WINDOW_WIDTH / 2;
float mouseY = WINDOW_HEIGHT / 2;

// handle keyboard events
// handler is synchronized by mainloop clock
void keyboardHandler();
// while callback is only triggerd by keyboard pressing
void keyboardCallback(GLFWwindow *window, int key, int _, int action, int __);

// pespective parameters
const float fovy = 90;
const float aspect = WINDOW_WIDTH / WINDOW_HEIGHT;
const float near = 0.05;
const float far = 2.8;

// camera
Camera camera(vec3(-0.35, 0.15, -0.15));
const float cameraMoveSpeed = 0.001;
const float cameraRotateAnglePerPixel = 0.1;

// shining
vec3 lightPos(0.2, 0.2, 0.2);
unsigned short specularColorIndex = 0, specularMaterialIndex = 0;
vector<RGBColor> specularColorPresets = {
    RGBColor(255, 255, 255), RGBColor(255, 0, 0),   RGBColor(0, 255, 0),
    RGBColor(0, 0, 255),     RGBColor(255, 255, 0), RGBColor(255, 0, 255),
    RGBColor(255, 255, 0),
};
vector<RGBColor> specularMaterialPresets =
    vector<RGBColor>(specularColorPresets);
float shiness = 35.0;
PhongLightModel phong(RGBColor(240, 240, 240), RGBColor(230, 230, 230),
                      RGBColor(240, 240, 240), RGBColor(60, 60, 60),
                      specularColorPresets[specularColorIndex],
                      specularMaterialPresets[specularMaterialIndex], shiness);

// ReNow helper
ReNowHelper helper;

int main() {
  consoleLogWelcome();

  // initialize
  GLFWwindow *window = initGLWindow("1-display", WINDOW_WIDTH, WINDOW_HEIGHT);
  glEnable(GL_DEPTH_TEST);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

  // listen mouse and keyboard
  glfwSetCursorPosCallback(window, mouseMoveCallback);
  glfwSetKeyCallback(window, keyboardCallback);
  // take over management
  helper = ReNowHelper(window);

  // organize the program
  GL_SHADER_ID vShader =
                   helper.createShader(GL_VERTEX_SHADER, "./shader/vMain.glsl"),
               fShader = helper.createShader(GL_FRAGMENT_SHADER,
                                             "./shader/fMain.glsl");
  GL_PROGRAM_ID mainProgram = helper.createProgram(vShader, fShader);
  helper.switchProgram(mainProgram);

  // start loading objects
  GL_OBJECT_ID sphereVAO = helper.createVAO(), cubeVAO = helper.createVAO();
  GL_OBJECT_ID sphereVBuf = helper.createVBO(), sphereNBuf = helper.createVBO();
  GL_OBJECT_ID cubeVBuf = helper.createVBO(), cubeNBuf = helper.createVBO();

  std::cout << ">>> Start loading model .obj file, this might take a while..."
            << std::endl;
  // load the sphere and perpare to draw it
  helper.switchVAO(sphereVAO);
  OBJProcessor sphereOBJProc(readFileText("./model/UnitSphere.obj"));
  vector<float> sphereVs = analyzeFtoV(sphereOBJProc, "fs");
  vector<float> sphereVns = analyzeFtoV(sphereOBJProc, "fns");
  helper.prepareAttributes(vector<APrepInfo>{
      {sphereVBuf, &(sphereVs[0]), sphereVs.size(), "aPosition", 3, GL_FLOAT},
      {sphereNBuf, &(sphereVns[0]), sphereVns.size(), "aNormal", 3, GL_FLOAT},
  });

  // load the cube and perpare to draw it
  helper.switchVAO(cubeVAO);
  OBJProcessor cubeOBJProc(readFileText("./model/UnitCube.obj"));
  vector<float> cubeVs = analyzeFtoV(cubeOBJProc, "fs");
  vector<float> cubeVns = analyzeFtoV(cubeOBJProc, "fns");
  helper.prepareAttributes(vector<APrepInfo>{
      {cubeVBuf, &(cubeVs[0]), cubeVs.size(), "aPosition", 3, GL_FLOAT},
      {cubeNBuf, &(cubeVns[0]), cubeVns.size(), "aNormal", 3, GL_FLOAT},
  });

  std::cout << ">>> Start rendering." << std::endl;
  // main loop
  while (!glfwWindowShouldClose(window)) {
    // handler frame-level keyboard event
    keyboardHandler();

    // clear the canvas for rerender
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    helper.switchProgram(mainProgram);

    // transform matrics
    mat4 worldMat = camera.getLookAt();
    mat4 perspectiveMat = glm::perspective(radians(fovy), aspect, near, far);

    helper.prepareUniforms(vector<UPrepInfo>{
        // Phong light model things
        {"uLightPosition", lightPos, "3fv"},
        {"uAmbientProduct", phong.ambientProduct(), "4fv"},
        {"uDiffuseProduct", phong.diffuseProduct(), "4fv"},
        {"uSpecularProduct", phong.specularProduct(), "4fv"},
        {"uShiness", phong.materialShiness(), "1f"},
        // transform matrics
        {"uPerspectiveMatrix", perspectiveMat, "Matrix4fv"},
        {"uWorldMatrix", worldMat, "Matrix4fv"},
    });

    // draw the sphere
    helper.prepareUniforms(vector<UPrepInfo>{
        {"uColor", normalizeRGBColor(RGBColor(29, 156, 215)), "4fv"},
        {"uModelMatrix", scalem(vec3(0.2, 0.2, 0.2)) * translate(0, 0, 1.0),
         "Matrix4fv"},
    });
    helper.switchVAO(sphereVAO);
    glDrawArrays(GL_TRIANGLES, 0, sphereOBJProc.getEffectiveVertexCount());

    // draw the cube
    helper.prepareUniforms(vector<UPrepInfo>{
        {"uColor", normalizeRGBColor(RGBColor(127, 127, 127)), "4fv"},
        {"uModelMatrix",
         scalem(vec3(0.1, 0.1, 0.1)) * translate(0.5, 1.5, -1.0), "Matrix4fv"},
    });
    helper.switchVAO(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, cubeOBJProc.getEffectiveVertexCount());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // cleaning
  helper.freeAllocatedObjects();
  glfwTerminate();
  return 0;
}

// handle keyboard events
void keyboardHandler() {
  // W-A-S-D move front-left-back-right
  if (helper.nowPressing(GLFW_KEY_W)) {
    camera.move(vec3(0, 0, cameraMoveSpeed));
  }
  if (helper.nowPressing(GLFW_KEY_A)) {
    camera.move(vec3(-cameraMoveSpeed, 0, 0));
  }
  if (helper.nowPressing(GLFW_KEY_S)) {
    camera.move(vec3(0, 0, -cameraMoveSpeed));
  }
  if (helper.nowPressing(GLFW_KEY_D)) {
    camera.move(vec3(cameraMoveSpeed, 0, 0));
  }
  // space-shift move up-down
  if (helper.nowPressing(GLFW_KEY_SPACE)) {
    camera.move(vec3(0, cameraMoveSpeed, 0));
  }
  if (helper.nowPressing(GLFW_KEY_LEFT_SHIFT)) {
    camera.move(vec3(0, -cameraMoveSpeed, 0));
  }
  // camera.printPosition();
}

void keyboardCallback(GLFWwindow *window, int key, int _, int action, int __) {
  if (action == GLFW_PRESS) {
    // specular lighting control
    if (key == GLFW_KEY_O) {
      specularColorIndex++;
      specularColorIndex %= specularColorPresets.size();
      phong.setSpecularColor(specularColorPresets[specularColorIndex]);
      phong.recalcProducts();
      std::cout << "Changed specularColor to "
                << rgbColorToString(specularColorPresets[specularColorIndex])
                << "\n";
    }
    if (key == GLFW_KEY_K) {
      specularMaterialIndex++;
      specularMaterialIndex %= specularMaterialPresets.size();
      phong.setSpecularMaterial(specularMaterialPresets[specularMaterialIndex]);
      phong.recalcProducts();
      std::cout << "Changed specularMaterial to "
                << rgbColorToString(
                       specularMaterialPresets[specularMaterialIndex])
                << "\n";
    }
    // shiness changes
    if (key == GLFW_KEY_I || key == GLFW_KEY_J) {
      shiness += (key & 1 ? 1 : -1) * 2;
      shiness = zx::minmaxClip(shiness, 0.5, 30);
      phong.setMaterialShiness(shiness);
      phong.recalcProducts();
      std::cout << "Changed shiness to " << shiness << "\n";
    }
  }
}

// handle mouse movements
void mouseMoveCallback(GLFWwindow *window, double offsetX, double offsetY) {
  float dx = offsetX - mouseX, dy = -(offsetY - mouseY);
  camera.rotate(
      vec2(dy * cameraRotateAnglePerPixel, dx * cameraRotateAnglePerPixel));
  mouseX = offsetX, mouseY = offsetY;
}

void consoleLogWelcome() {
  std::cout << "################################\n"
               "# Viz Project 1 - Basic OpenGL #\n"
               "#  by 212138 - Zhuo Xu         #\n"
               "# @ github.com/z0gSh1u/seu-viz #\n"
               "################################\n"
               "### Operation Guide ###\n"
               "[View transforms]\n"
               "W - Move forward; A - Move leftward\n"
               "S - Move backward; D - Move rightward\n"
               "Space - Move upward; Shift - Move upward\n"
               "[Specular lighting]\n"
               "O - Switch light color between presets\n"
               "K - Switch material color between presets\n"
               "I - Higher Shiness; J - Lower shiness\n"
               "########################\n";
}