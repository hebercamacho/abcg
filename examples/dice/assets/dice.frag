#version 410

in vec4 fragColor;

out vec4 outColor;

void main() {
  //i é a intensidade da cor
  float i = 1.3 - gl_FragCoord.z;
  //i depende de z para que um fragmento mais no fundo fique mais escuro, dando aparência de sombreamento
  
  //gl_FrontFacing é true se esta é a face da frente
  if (gl_FrontFacing) {
    outColor = vec4(fragColor.r*i,fragColor.g*i,fragColor.b*i,fragColor.a); //por fora, tom entre branco e cinza
    // outColor = fragColor;
  } else {
    outColor = vec4(0.5f, 0.5f, 0.5f, 1.0f); //por dentro, tom de cinza escuro
  }
}