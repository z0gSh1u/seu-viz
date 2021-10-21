#pragma once
#ifndef PHONGLIGHTMODEL_HPP_
#define PHONGLIGHTMODEL_HPP_

#include <glm/glm.hpp>
#include "Utils.hpp"

using glm::vec3;
using glm::vec4;

class PhongLightModel {
private:
  vec4 _lightPosition;

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
  PhongLightModel(vec3 lightPosition, RGBColor ambientColor,
                  RGBColor ambientMaterial, RGBColor diffuseColor,
                  RGBColor diffuseMaterial, RGBColor specularColor,
                  RGBColor specularMaterial, float materialShiness);
  ~PhongLightModel();

  void recalcProducts() {
    this->_ambientProduct = this->_ambientColor * this->_ambientMaterial;
    this->_diffuseProduct = this->_diffuseColor * this->_diffuseMaterial;
    this->_specularProduct = this->_specularColor * this->_specularMaterial;
  }

  // getters
  const vec4 &lightPosition() const { return _lightPosition; }
  const vec4 &ambientProduct() const { return _ambientProduct; }
  const vec4 &diffuseProduct() const { return _diffuseProduct; }
  const vec4 &specularProduct() const { return _specularProduct; }
};

PhongLightModel::PhongLightModel(
    vec3 lightPosition, RGBColor ambientColor, RGBColor ambientMaterial,
    RGBColor diffuseColor, RGBColor diffuseMaterial, RGBColor specularColor,
    RGBColor specularMaterial, float materialShiness) {
  this->_lightPosition = vec4(lightPosition, 1.0);
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

#endif