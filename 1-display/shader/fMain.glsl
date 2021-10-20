// Fragment Shader

precision mediump float;

varying vec4 vColor;
varying vec4 vLight;

void main() {

  gl_FragColor = vColor * vLight;

}