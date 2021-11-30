#version 410

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

out vec4 fragColor;

void main() {
  vec4 posEyeSpace = viewMatrix * modelMatrix * vec4(inPosition, 1);

  float i = 1.0 - (-posEyeSpace.z / 5.0); //1 se estiver em cima da camera, 0 se estiver a 5 ou + de distancia da camera
  fragColor = vec4(i, i, i, 1) * vec4(inColor, 1);

  gl_Position = projMatrix * posEyeSpace;
}