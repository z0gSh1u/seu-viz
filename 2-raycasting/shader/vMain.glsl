// Vertex shader for background rendering

attribute vec2 aPosition;
attribute vec2 aTexCoord;

varying vec2 vTexCoord;

void main() {

  gl_Position.xy = aPosition.xy;
  gl_Position.zw = vec2(1.0, 1.0);
  
  vTexCoord = aTexCoord;

}