#pragma once

// Camera Class of ReNow Framework
// by z0gSh1u (Zhuo Xu) @ https://github.com/z0gSh1u/seu-viz
// See:
// https://github.com/z0gSh1u/typed-webgl
// https://github.com/z0gSh1u/renow-ts
// https://github.com/z0gSh1u/renow-c

#ifndef CAMERA_HPP_
#define CAMERA_HPP_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ostream>

#include "Utils.hpp"

using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;
using std::ostream;

using namespace zx;

const vec3 WORLD_ORIGIN = vec3(0.0, 0.0, 0.0);
const vec3 VEC_UP = vec3(0.0, 1.0, 0.0);

namespace zx {

// Camera Wrapper class
class Camera {
private:
  // determines lookAt matrix
  vec3 _position;
  vec3 _front;
  // for translation
  vec3 _right;

  // Pitch-Yaw angles (DEG)
  float _pitch;
  float _yaw;
  // we dont care `_roll` for camera, since it is not a plane-driving game

public:
  // getters
  const vec3 &position() const { return _position; }

  // get lookAt matrix
  mat4 getLookAt() {
    return glm::lookAt(_position, _position + _front, VEC_UP);
  }

  // move the camera by vec (LR-UD-FB).
  void move(vec3 xyz, bool _reCalc = true) {
    _position += _right * xyz[0] + VEC_UP * xyz[1] + _front * xyz[2];
    if (_reCalc) {
      reCalc();
    }
  }

  // 3D rotate (DEG)
  void rotate(vec2 pitchYaw, bool _reCalc = true) {
    _pitch += pitchYaw[0];
    _yaw += pitchYaw[1];
    // clip pitch
    _pitch = zx::minmaxClip(_pitch, -89, 89);
    if (_reCalc) {
      reCalc();
    }
  }

  // update relevant vectors
  void reCalc() {
    // update front
    vec3 front;
    float pitch = radians(_pitch), yaw = radians(_yaw);
    front.x = cos(pitch) * cos(yaw);
    front.y = sin(pitch);
    front.z = cos(pitch) * sin(yaw);
    _front = glm::normalize(front);
    // update right
    _right = glm::normalize(glm::cross(_front, VEC_UP));
  }

  void printPosition() {
    std::cout << "[" << this->_position.x << "," << this->_position.y << ","
              << this->_position.z << "]"
              << "\n";
  }

  Camera(vec3 position);
  ~Camera();
};

Camera::Camera(vec3 position) : _position(position) {
  _pitch = _yaw = 0;
  reCalc();
}

Camera::~Camera() {}

} // namespace zx

#endif