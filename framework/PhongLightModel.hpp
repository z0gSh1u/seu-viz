#pragma once

// Phong Light Model of ReNow Framework
// by z0gSh1u @ https://github.com/z0gSh1u/seu-viz
// See:
// https://github.com/z0gSh1u/typed-webgl
// https://github.com/z0gSh1u/renow-ts
// https://github.com/z0gSh1u/renow-c

#ifndef PHONGLIGHTMODEL_HPP_
#define PHONGLIGHTMODEL_HPP_

#include <glm/glm.hpp>
#include "Utils.hpp"

using glm::vec3;
using glm::vec4;

namespace zx {
// Phong Light model wrapper class.
class PhongLightModel {
private:
  vec4 _ambientColor;
  vec4 _diffuseColor;
  vec4 _specularColor;

  vec4 _ambientMaterial;
  vec4 _diffuseMaterial;
  vec4 _specularMaterial;
  float _materialShiness;

  vec4 _ambientProduct;
  vec4 _diffuseProduct;
  vec4 _specularProduct;

public:
  PhongLightModel(RGBColor ambientColor, RGBColor ambientMaterial,
                  RGBColor diffuseColor, RGBColor diffuseMaterial,
                  RGBColor specularColor, RGBColor specularMaterial,
                  float materialShiness);
  ~PhongLightModel();

  void recalcProducts() {
    this->_ambientProduct = this->_ambientColor * this->_ambientMaterial;
    this->_diffuseProduct = this->_diffuseColor * this->_diffuseMaterial;
    this->_specularProduct = this->_specularColor * this->_specularMaterial;
  }

  // setters
  void setAmbientColor(RGBColor color) {
    this->_ambientColor = normalize8bitColor(color);
  }

  void setDiffuseColor(RGBColor color) {
    this->_diffuseColor = normalize8bitColor(color);
  }

  void setSpecularColor(RGBColor color) {
    this->_specularColor = normalize8bitColor(color);
  }

  void setAmbientMaterial(RGBColor color) {
    this->_ambientMaterial = normalize8bitColor(color);
  }

  void setDiffuseMaterial(RGBColor color) {
    this->_diffuseMaterial = normalize8bitColor(color);
  }

  void setSpecularMaterial(RGBColor color) {
    this->_specularMaterial = normalize8bitColor(color);
  }

  void setMaterialShiness(float shiness) { this->_materialShiness = shiness; }

  // getters
  const vec4 &ambientProduct() const { return _ambientProduct; }
  const vec4 &diffuseProduct() const { return _diffuseProduct; }
  const vec4 &specularProduct() const { return _specularProduct; }
  const float &materialShiness() const { return _materialShiness; }
};

PhongLightModel::PhongLightModel(
    RGBColor ambientColor, RGBColor ambientMaterial, RGBColor diffuseColor,
    RGBColor diffuseMaterial, RGBColor specularColor, RGBColor specularMaterial,
    float materialShiness) {
  this->_ambientColor = normalize8bitColor(ambientColor);
  this->_ambientMaterial = normalize8bitColor(ambientMaterial);
  this->_diffuseColor = normalize8bitColor(diffuseColor);
  this->_diffuseMaterial = normalize8bitColor(diffuseMaterial);
  this->_specularColor = normalize8bitColor(specularColor);
  this->_specularMaterial = normalize8bitColor(specularMaterial);
  this->_materialShiness = materialShiness;
  // compute Phong light model products
  this->recalcProducts();
}

PhongLightModel::~PhongLightModel() {}

} // namespace zx

#endif