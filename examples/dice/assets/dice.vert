#version 410 core

layout(location = 0) in vec3 inPosition; //posição (x,y,z) do vértice
layout(location = 1) in vec3 inColor;

uniform vec3 translation;
uniform float rotationX;
uniform float rotationY;
uniform float rotationZ;

out vec4 fragColor;

void main() {

  vec3 newPosition = inPosition;
  if(rotationX > 0.0f){
    //rotação em torno do eixo x
    newPosition = vec3( newPosition.x,
                    newPosition.y * cos(rotationX) - newPosition.z * sin(rotationX),
                    newPosition.z * cos(rotationX) + newPosition.y * sin(rotationX));
  }
  
  if(rotationY > 0.0f){
    //rotação em torno do eixo y
    newPosition = vec3( newPosition.x * cos(rotationY) + newPosition.z * sin(rotationY),
                            newPosition.y,
                            newPosition.z * cos(rotationY) - newPosition.x * sin(rotationY));
  }
  if(rotationZ > 0.0f){
    //rotação em torno do eixo z
    newPosition = vec3( newPosition.x * cos(rotationZ) - newPosition.y * sin(rotationZ),
                            newPosition.y * cos(rotationZ) + newPosition.x * sin(rotationZ),
                            newPosition.z);
  }
  
  gl_Position = vec4(newPosition + translation, 2.0f); //jogar ele um pouquinho pra trás, pra ficar menor
  fragColor = vec4(inColor, 1.0f);
}