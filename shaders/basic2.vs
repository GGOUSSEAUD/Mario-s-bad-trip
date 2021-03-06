#version 330
layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec4 vsiColor;

uniform float cx;
uniform float cy;
uniform float cz;
uniform float cscale;
uniform float cred;
uniform float cgreen;
uniform float cblue;
uniform float calpha;

out vec4 vsoColor;

void main(void) {
  float red=vsiColor.r + cred ;
  float green=vsiColor.g + cgreen ;
  float blue=vsiColor.b + cblue ;
  float alpha=vsiColor.a + calpha ;
 
  float x=vsiPosition.x + cx ;
  float y=vsiPosition.y + cy ;
  float z=vsiPosition.z + cz ;
  float scale=1.0 + cscale ;
  
  gl_Position = vec4(x,y,z,scale);
  vsoColor= vec4(red,green,blue,alpha);
 
}
