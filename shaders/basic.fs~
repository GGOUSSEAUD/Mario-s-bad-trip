#version 330
uniform sampler2D myTexture;

in  vec2 vsoTexCoord;

out vec4 fragColor;

void main(void) {
  if(texture(myTexture, vsoTexCoord).a > 0.5){
    fragColor = texture(myTexture, vsoTexCoord).rgba;// + vsoColor;
  }else{
    discard;
  }
}

