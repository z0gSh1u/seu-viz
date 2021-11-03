#pragma once

// ReNow Framework
// by z0gSh1u @ https://github.com/z0gSh1u/seu-viz
// See:
// https://github.com/z0gSh1u/typed-webgl
// https://github.com/z0gSh1u/renow-ts
// https://github.com/z0gSh1u/renow-c

#ifndef RENOW_HPP_
#define RENOW_HPP_

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <regex>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>

#include "OBJProcessor.hpp"
#include "Utils.hpp"

using glm::mat2;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

using std::string;
using std::vector;

namespace zx {

// type alias
typedef GLuint GL_SHADER_ID;
typedef GLuint GL_PROGRAM_ID;
typedef GLuint GL_OBJECT_ID;
typedef GLuint GL_SHADER_TYPE_;
typedef GLint GL_UNIFORM_LOC;
typedef GLint GL_ATTRIB_LOC;

// Attribute preparation metainfo.
struct APrepInfo {
  GL_OBJECT_ID VBO;
  float *data;
  size_t elemCount; // narrow to GLsizeiptr when use
  string varName;
  int attrPer;
  GLenum elemType;

  APrepInfo() {}
  APrepInfo(const GL_OBJECT_ID _VBO, float *_data, size_t _elemCount,
            const string &_varName, int _attrPer, GLenum _elemType)
      : VBO(_VBO), data(_data), elemCount(_elemCount), varName(_varName),
        attrPer(_attrPer), elemType(_elemType) {}
};

// Uniform preparation metainfo.
struct UPrepInfo {
  string varName;

  union UData {
    float f1;     // 1f
    int i1;       // 1i
    glm::vec1 v1; // 1fv
    glm::vec2 v2; // 2fv
    glm::vec3 v3; // 3fv
    glm::vec4 v4; // 4fv
    glm::mat2 m2; // Matrix2fv
    glm::mat3 m3; // Matrix3fv
    glm::mat4 m4; // Matrix4fv
  } data;

  string method;

  UPrepInfo(){};

  // inflexible c++. I love JS.
  UPrepInfo(const string &_varName, const float _data, const string &_method)
      : varName(_varName), method(_method) {
    data.f1 = _data;
  };
  UPrepInfo(const string &_varName, const int _data, const string &_method)
      : varName(_varName), method(_method) {
    data.i1 = _data;
  };
  UPrepInfo(const string &_varName, const glm::vec1 &_data,
            const string &_method)
      : varName(_varName), method(_method) {
    data.v1 = _data;
  };
  UPrepInfo(const string &_varName, const glm::vec2 &_data,
            const string &_method)
      : varName(_varName), method(_method) {
    data.v2 = _data;
  };
  UPrepInfo(const string &_varName, const glm::vec3 &_data,
            const string &_method)
      : varName(_varName), method(_method) {
    data.v3 = _data;
  };
  UPrepInfo(const string &_varName, const glm::vec4 &_data,
            const string &_method)
      : varName(_varName), method(_method) {
    data.v4 = _data;
  };
  UPrepInfo(const string &_varName, const glm::mat2 &_data,
            const string &_method)
      : varName(_varName), method(_method) {
    data.m2 = _data;
  };
  UPrepInfo(const string &_varName, const glm::mat3 &_data,
            const string &_method)
      : varName(_varName), method(_method) {
    data.m3 = _data;
  };
  UPrepInfo(const string &_varName, const glm::mat4 &_data,
            const string &_method)
      : varName(_varName), method(_method) {
    data.m4 = _data;
  };
};

// Initialize GL window, return the handle.
GLFWwindow *initGLWindow(const string &windowName, int canvasWidth,
                         int canvasHeight) {
  glfwInit();
  GLFWwindow *window = glfwCreateWindow(canvasWidth, canvasHeight,
                                        windowName.c_str(), NULL, NULL);
  ASSERT(window != NULL, "Window creation failed: " + windowName + ".");
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  return window;
}

// Helper class from ReNow framework.
class ReNowHelper {

private:
  // maintainings
  GLFWwindow *_window;
  GL_PROGRAM_ID _currentProgram;
  vector<GL_OBJECT_ID> _allocatedVAOs;
  vector<GL_OBJECT_ID> _allocatedVBOs;

public:
  ReNowHelper() : _window(nullptr), _currentProgram(-1) {}
  ReNowHelper(GLFWwindow *window) : _window(window), _currentProgram(-1) {}

  // Create a program.
  GL_PROGRAM_ID
  createProgram(GL_SHADER_ID vShader, GL_SHADER_ID fShader) {
    // create program
    GL_PROGRAM_ID program = glCreateProgram();

    // attach shader to program
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);

    // compile the program
    glLinkProgram(program);
    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    ASSERT(status == GL_TRUE, "Program link failed.");

    return program;
  }

  // Create a shader of `shaderType`.
  GL_SHADER_ID createShader(GL_SHADER_TYPE_ shaderType, string shaderPath) {
    // create shader
    GL_SHADER_ID shader = glCreateShader(shaderType);

    // read shader content
    string _shaderContent = readFile(shaderPath);
    char *shaderContent = new char[_shaderContent.size() + 1];
    memcpy(shaderContent, _shaderContent.c_str(), _shaderContent.size() + 1);
    glShaderSource(shader, 1, &shaderContent, NULL);

    // compile shader
    glCompileShader(shader);

    // check shader status
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    ASSERT(status == GL_TRUE, "Shader creation failed.");
    return shader;
  }

  // Switch to a program.
  void switchProgram(GL_PROGRAM_ID program) {
    this->_currentProgram = program;
    glUseProgram(program);
  }

  // Get Uniform location in shader.
  GL_UNIFORM_LOC
  getUniformLocation(const string &varName) {
    GL_UNIFORM_LOC loc =
        glGetUniformLocation(this->_currentProgram, varName.c_str());
    ASSERT(loc != -1, "Invalid Uniform location for `" + varName + "`.");
    return loc;
  }

  // Get attribute location in shader.
  GL_ATTRIB_LOC getAttributeLocation(const string &varName) {
    GL_ATTRIB_LOC loc =
        glGetAttribLocation(this->_currentProgram, varName.c_str());
    ASSERT(loc != -1, "Invalid Attribute location for `" + varName + "`.");
    return loc;
  }

  // Set Uniforms in shader.
  void prepareUniforms(const vector<UPrepInfo> &uniforms) {
    for (const auto &uniform : uniforms) {
      GL_UNIFORM_LOC loc = getUniformLocation(uniform.varName);
      // call corresponding method
      const string &method = uniform.method;
      auto &data = uniform.data;
      if (method == "1f") {
        glUniform1f(loc, data.f1);
      } else if (method == "1i") {
        glUniform1i(loc, data.i1);
      } else if (method == "1fv") {
        glUniform1fv(loc, 1, &(data.v1[0]));
      } else if (method == "2fv") {
        glUniform2fv(loc, 1, &(data.v2[0]));
      } else if (method == "3fv") {
        glUniform3fv(loc, 1, &(data.v3[0]));
      } else if (method == "4fv") {
        glUniform4fv(loc, 1, &(data.v4[0]));
      } else if (method == "Matrix2fv") {
        glUniformMatrix2fv(loc, 1, GL_FALSE, &(data.m2[0][0]));
      } else if (method == "Matrix3fv") {
        glUniformMatrix3fv(loc, 1, GL_FALSE, &(data.m3[0][0]));
      } else if (method == "Matrix4fv") {
        glUniformMatrix4fv(loc, 1, GL_FALSE, &(data.m4[0][0]));
      } else {
        ASSERT(false, "Invalid method in prepareUniform: " + method + ".");
      }
    }
  }

  // Check if user is pressing `keyCode` key.
  bool nowPressing(int keyCode) {
    return glfwGetKey(_window, keyCode) == GLFW_PRESS;
  }

  // Switch current VAO.
  void switchVAO(GL_OBJECT_ID VAO) { glBindVertexArray(VAO); }

  // Switch current VBO.
  void switchVBO(GL_OBJECT_ID VBO) { glBindBuffer(GL_ARRAY_BUFFER, VBO); }

  // Create new VAO.
  GL_OBJECT_ID createVAO() {
    GL_OBJECT_ID res;
    glGenVertexArrays(1, &res);
    this->_allocatedVAOs.push_back(res);
    return res;
  }

  // Create new VBO.
  GL_OBJECT_ID createVBO() {
    GL_OBJECT_ID res;
    glGenBuffers(1, &res);
    this->_allocatedVBOs.push_back(res);
    return res;
  }

  // Free allocated VAOs and VBOs.
  void freeAllocatedObjects() {
    for (const auto &id : _allocatedVAOs) {
      glDeleteVertexArrays(1, &id);
    }
    for (const auto &id : _allocatedVBOs) {
      glDeleteBuffers(1, &id);
    }
  }

  // Set Attributes in shader.
  void prepareAttributes(const vector<APrepInfo> &attributes,
                         bool useVAO = false, GL_OBJECT_ID VAO = -1) {
    if (useVAO) {
      glBindVertexArray(VAO);
    }
    for (const auto &attribute : attributes) {
      // change buffer
      switchVBO(attribute.VBO);
      // buffer data
      glBufferData(GL_ARRAY_BUFFER,
                   ((GLsizeiptr)attribute.elemCount) * sizeof(float),
                   attribute.data, GL_STATIC_DRAW);
      if (!useVAO) {
        // config meta info
        auto loc = getAttributeLocation(attribute.varName);
        glVertexAttribPointer(loc, attribute.attrPer, attribute.elemType,
                              GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(loc);
      }
    }
  }
};

// Convert faces to vectices sequence.
vector<float> analyzeFtoV(const OBJProcessor &proc, const string &kind) {
  vector<float> mesh; // xyzxyz
  Vec3s faces, vertices;
  if (kind == "fs") {
    faces = proc.fs();
    vertices = proc.vs();
  } else if (kind == "fns") {
    faces = proc.fns();
    vertices = proc.vns();
  } else {
    ASSERT(false, "Unsupported face kind.");
  }
  // FIXME: Rule out texture faces temporarily.
  // } else if (kind == "fts") {
  //   faces = proc.fts();
  //   vertices = proc.vts();
  // }
  for (const vec3 &face : faces) {
    auto &v1 = vertices.at(face.x - 1), &v2 = vertices.at(face.y - 1),
         &v3 = vertices.at(face.z - 1);
    mesh.insert(mesh.end(),
                {v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z});
  }
  return mesh;
}

// Alias some useful matrices

// I_4 matrix.
mat4 eye4() { return mat4(1.0); }

// I_3 matrix.
mat3 eye3() { return mat3(1.0); }

// scaling matrix
mat4 scalem(vec3 vec) { return glm::scale(eye4(), vec); }

// rotateX matrix
mat4 rotateX(float rad) {
  return glm::rotate(eye4(), rad, vec3(1.0, 0.0, 0.0));
}

// rotateY matrix
mat4 rotateY(float rad) {
  return glm::rotate(eye4(), rad, vec3(0.0, 1.0, 0.0));
}

// rotateZ matrix
mat4 rotateZ(float rad) {
  return glm::rotate(eye4(), rad, vec3(0.0, 0.0, 1.0));
}

// translation matrix
mat4 translate(float x, float y, float z) {
  return glm::translate(eye4(), vec3(x, y, z));
}

} // namespace zx

#endif