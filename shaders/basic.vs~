#version 330


layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec2 vsiTexCoord;

uniform float cx;
uniform float cy;
uniform float cz;
uniform float cscale;

out vec2 vsoTexCoord;

void main(void) {

  float x=vsiPosition.x + cx ;
  float y=vsiPosition.y + cy ;
  float z=vsiPosition.z + cz ;
  float scale=1.0 + cscale ;
  
  gl_Position = vec4(x,y,z,scale);
  vsoTexCoord = vec2(vsiTexCoord.s, 1.0 - vsiTexCoord.t);
}
