attribute vec2 aPosition;
attribute vec3 aColor;

varying vec4 vColor;

void main() {
  // convey vertex position
  gl_Position = vec4(aPosition.xy, 0.0, 1.0);
  
  // convey color to fragment shader
  vColor = vec4(aColor.rgb, 1.0);
}