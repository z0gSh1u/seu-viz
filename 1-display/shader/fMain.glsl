// Fragment Shader

precision mediump float;

// varying vec4 vColor;
varying vec4 vLight;

void main() {
  vec4 vColor = vec4(0.0, 0.2, 0.8, 1.0);

  gl_FragColor = vColor * vLight;

}