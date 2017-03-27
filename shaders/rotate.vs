#version 330

layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec2 vsiTexCoord;

uniform float cx;
uniform float cy;
uniform float cz;
uniform float cscale;
uniform float teta;

out vec2 vsoTexCoord;

void main(void) {
  //Rotation Z
  float x = (cos(teta)*vsiPosition.x - sin(teta)*vsiPosition.y*0.83 + 0) + cx;
  float y = (sin(teta)*vsiPosition.x + cos(teta)*vsiPosition.y + 0)+cy;
  float z = 0.0 + 0.0 + vsiPosition.z + cz;

  float scale= 1.0+ cscale ;

  //Rotation Y

  /* float x = cos(teta)*vsiPosition.x - 0 + sin(teta)*vsiPosition.z*0.35;//+cos(teta)/2; */
  /* float y = 0 + vsiPosition.y + 0 ;//+sin(teta)/2; */
  /* float z = -sin(teta)*vsiPosition.x + 0.0 + cos(teta)*vsiPosition.z ; */

  //Rotation X

  /* float x=vsiPosition.x + 0 + 0; */
  /* float y=(0 + cos(teta)*vsiPosition.y-sin(teta)*vsiPosition.z*0.45); */
  /* float z=(0 + sin(teta)*vsiPosition.y+cos(teta)*vsiPosition.z); */

  gl_Position = vec4(x,y,z, scale);
  vsoTexCoord = vec2(vsiTexCoord.s, 1.0 - vsiTexCoord.t);
}
