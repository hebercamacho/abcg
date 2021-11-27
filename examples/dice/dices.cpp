#include "dices.hpp"
#include <imgui.h>
#include <glm/gtx/fast_trigonometry.hpp>
#include <fmt/core.h>

void Dices::initializeGL(GLuint program, int quantity, std::vector<Vertex> vertices, std::vector<GLuint> indices, int verticesToDraw){
  terminateGL();
  // Inicializar gerador de números pseudo-aleatórios
  auto seed{std::chrono::steady_clock::now().time_since_epoch().count()};
  m_randomEngine.seed(seed);

  m_program = program;
  m_vertices = vertices;
  m_indices = indices;
  m_verticesToDraw = verticesToDraw;

  m_dices.clear();
  m_dices.resize(quantity);

  //fmt::print("m_program: {}\n", m_program);
  //fmt::print("m_vertices.size(): {}\n", m_vertices.size());
  //fmt::print("m_indices.size(): {}\n", m_indices.size());
  //fmt::print("m_verticesToDraw: {}\n", m_verticesToDraw);

  for(auto &dice : m_dices) {
    dice = inicializarDado();
  }
  
}

void Dices::paintGL(int viewportWidth, int viewportHeight, float deltaTime){
  m_deltaTime = deltaTime;
  m_viewportWidth = viewportWidth;
  m_viewportHeight = viewportHeight;

  abcg::glUseProgram(m_program); //usar shaders
    
  for(auto &dice : m_dices){
    abcg::glBindVertexArray(dice.m_VAO); //usar vao

    //Dado sendo girado, temos que definir algumas variáveis para ilustrar seu giro de forma realista
    if(dice.dadoGirando){
      checkCollisions(dice);

      dice.quadros++;
      if(dice.translation.x >= 1.5f) {
        dice.movimentoDado.x = false;
        velocidadeAngularAleatoria(dice);
        velocidadeDirecionalAleatoria(dice);
      }
      else if (dice.translation.x <= -1.5f) {
        dice.movimentoDado.x = true;
        velocidadeAngularAleatoria(dice);
        velocidadeDirecionalAleatoria(dice);
      }

      if(dice.translation.y >= 1.5f) {
        dice.movimentoDado.y = false;
        velocidadeAngularAleatoria(dice);
        velocidadeDirecionalAleatoria(dice);
      }
      else if (dice.translation.y <= -1.5f) {
        dice.movimentoDado.y = true;
        velocidadeAngularAleatoria(dice);
        velocidadeDirecionalAleatoria(dice);
      }
      
      //ir pra direita
      if(dice.movimentoDado.x) {
        dice.translation.x += dice.velocidadeDirecional.x; 
      }
      //ir pra esquerda
      else{
        dice.translation.x -= dice.velocidadeDirecional.x;
      }
      //ir pra cima
      if(dice.movimentoDado.y) {
        dice.translation.y += dice.velocidadeDirecional.y; 
      }
      //ir pra baixo
      else{
        dice.translation.y -= dice.velocidadeDirecional.y;
      }

      ////fmt::print("q: {} dice.translation: {} {}\n", dice.quadros, dice.translation.x, dice.translation.y);
      
      //podemos finalizar o giro do dado e parar num número aleatório
      if(dice.quadros > dice.maxQuadros){
        pousarDado(dice);
      }
    }

    // angulo (em radianos) é incrementado se houver alguma rotação ativa
    if(dice.m_rotation.x || dice.m_rotation.y ||dice.m_rotation.z){
      //ajuste de velocidade de rotação, necessário para conseguirmos pausar
      dice.myTime = deltaTime;
      
      //incrementa ângulo de {x,y,z} se rotação em torno do eixo {x,y,z} estiver ativa
      if(dice.m_rotation.x)
        dice.m_angle.x = glm::wrapAngle(dice.m_angle.x + dice.velocidadeAngular.x * dice.myTime);

      if(dice.m_rotation.y)
        dice.m_angle.y = glm::wrapAngle(dice.m_angle.y + dice.velocidadeAngular.y * dice.myTime);

      if(dice.m_rotation.z)
        dice.m_angle.z = glm::wrapAngle(dice.m_angle.z + dice.velocidadeAngular.z * dice.myTime);
    }
    ////fmt::print("angle: {} {} {}\n", dice.m_angle.x, dice.m_angle.y, dice.m_angle.z);


    // atualizar variavel do angulo de rotação e posição de translação dentro do vertex shader
    const GLint rotationXLoc{abcg::glGetUniformLocation(m_program, "rotationX")};
    abcg::glUniform1f(rotationXLoc, dice.m_angle.x);
    const GLint rotationYLoc{abcg::glGetUniformLocation(m_program, "rotationY")};
    abcg::glUniform1f(rotationYLoc, dice.m_angle.y);
    const GLint rotationZLoc{abcg::glGetUniformLocation(m_program, "rotationZ")};
    abcg::glUniform1f(rotationZLoc, dice.m_angle.z);
    const GLint translationLoc{abcg::glGetUniformLocation(m_program, "translation")};
    abcg::glUniform3fv(translationLoc, 1, &dice.translation.x);

    // Draw triangles
    abcg::glDrawElements(GL_TRIANGLES, m_verticesToDraw, GL_UNSIGNED_INT,
                        nullptr);

    abcg::glBindVertexArray(0);
  }
  abcg::glUseProgram(0);
}

//função para começar o dado numa posição e número aleatório, além de inicializar algumas outras variáveis necessárias
Dices::Dice Dices::inicializarDado() {
  Dice dice;

  // Generate VBO
  abcg::glGenBuffers(1, &dice.m_VBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, dice.m_VBO);
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices[0]) * m_vertices.size(),
                     m_vertices.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
  ////fmt::print("vbo: {}\n", dice.m_VBO);

  // Generate EBO
  abcg::glGenBuffers(1, &dice.m_EBO);
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dice.m_EBO);
  abcg::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     sizeof(m_indices[0]) * m_indices.size(), m_indices.data(),
                     GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  ////fmt::print("ebo: {}\n", dice.m_EBO);

  // Create VAO
  abcg::glGenVertexArrays(1, &dice.m_VAO);

  // Bind vertex attributes to current VAO
  abcg::glBindVertexArray(dice.m_VAO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, dice.m_VBO);
  ////fmt::print("vAo: {}\n", dice.m_VAO);

  // Bind vertex attributes
  GLint positionAttribute{abcg::glGetAttribLocation(m_program, "inPosition")}; //layout(location = _)
  if (positionAttribute >= 0) {
    abcg::glEnableVertexAttribArray(positionAttribute);
    abcg::glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex), nullptr);
  }

  //aqui a gente passa a cor do vértice já pronta para o shader
  const GLint colorAttribute{abcg::glGetAttribLocation(m_program, "inColor")};
  if (colorAttribute >= 0) {
    abcg::glEnableVertexAttribArray(colorAttribute);
    GLsizei offset{sizeof(glm::vec3)};
    abcg::glVertexAttribPointer(colorAttribute, 3, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex),
                                reinterpret_cast<void*>(offset));
  }

  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dice.m_EBO);

  // End of binding to current VAO
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
  abcg::glBindVertexArray(0);
  
  //estado inicial de algumas variáveis
  dice.m_rotation = {0, 0, 0};  
  dice.velocidadeAngular = {0.0f, 0.0f, 0.0f};
  dice.myTime = 0.0f;
  dice.quadros=0;

  std::uniform_real_distribution<float> fdist(-1.5f,1.5f);
  dice.translation = {fdist(m_randomEngine),fdist(m_randomEngine),0.0f};
  pousarDado(dice); //começar num numero aleatorio

  return dice;
}

void Dices::jogarDado(Dice &dice){
  tempoGirandoAleatorio(dice);
  velocidadeAngularAleatoria(dice);
  velocidadeDirecionalAleatoria(dice);
  dice.dadoGirando = true;
}

//função para fazer o dado parar numa das faces retas aleatoriamente
void Dices::pousarDado(Dice &dice) {
  auto seed{std::chrono::steady_clock::now().time_since_epoch().count()};
  m_randomEngine.seed(seed);
  //reinicialização de variáveis para podermos parar o dado e jogar novamente
  dice.quadros = 0;
  dice.dadoGirando = false;
  dice.m_rotation = {0,0,0};

  //fmt::print("posicao final: {} {}\n", dice.translation.x, dice.translation.y);

  std::uniform_int_distribution<int> idist(1,6);
  const int numeroDoDado = idist(m_randomEngine);
  //fmt::print("numeroDoDado: {}\n", numeroDoDado);
  dice.m_angle.x = glm::radians(angulosRetos[numeroDoDado].x);
  dice.m_angle.y = glm::radians(angulosRetos[numeroDoDado].y);
}

//função para definir tempo de giro do dado, algo entre 2 e 5 segundos 
void Dices::tempoGirandoAleatorio(Dice &dice){
  const float FPS = ImGui::GetIO().Framerate;
  //distribuição aleatória para definir tempo de giro do dado, algo entre 2 e 5 segundos 
  std::uniform_int_distribution<int> idist((int)FPS * 2, (int)FPS * 5);
  dice.maxQuadros = idist(m_randomEngine); //número máximo de quadros/vezes que o dado irá girar
  //fmt::print("maxQuadros: {}\n", dice.maxQuadros);
}

//atualiza as velocidades de cada um dos eixos de forma aleatória
void Dices::velocidadeAngularAleatoria(Dice &dice){
  //distribuição aleatória entre 0 e 2, para girar somente 1 eixo
  dice.m_rotation = {0, 0, 0};
  std::uniform_int_distribution<int> idist(0,2);
  dice.m_rotation[idist(m_randomEngine)] = 1;
  ////fmt::print("m_rotation.x: {} m_rotation.y: {} m_rotation.z: {}\n", dice.m_rotation.x, dice.m_rotation.y, dice.m_rotation.z);

  const float FPS = ImGui::GetIO().Framerate; //(para calcular os segundos precisamos do número inteiro de FPS)
  //fmt::print("FPS: {}\n",FPS);

  //distribuição aleatória de velocidade angular, para girar em cada eixo numa velocidade
  std::uniform_real_distribution<float> fdist(FPS * 4, FPS * 8);
  dice.velocidadeAngular = {glm::radians(fdist(m_randomEngine))
                      ,glm::radians(fdist(m_randomEngine))
                      ,glm::radians(fdist(m_randomEngine))};
  ////fmt::print("velocidadeAngular.x: {} velocidadeAngular.y: {} velocidadeAngular.z: {}\n", dice.velocidadeAngular.x, dice.velocidadeAngular.y, dice.velocidadeAngular.z);
}

//recebe uma das dimensões da janela e retorna uma fração aleatória do seu tamanho
void Dices::velocidadeDirecionalAleatoria(Dice &dice){
  //distribuição aleatória de velocidade, para andar em cada eixo numa velocidade
  std::uniform_real_distribution<float> fdist(m_deltaTime / 200.0f, m_deltaTime / 100.0f);
  dice.velocidadeDirecional.x = fdist(m_randomEngine) * m_viewportWidth;
  dice.velocidadeDirecional.y = fdist(m_randomEngine) * m_viewportHeight;
  //fmt::print("m_deltaTime: {}\n", m_deltaTime);
  ////fmt::print("velocidadeDirecional.x: {} velocidadeDirecional.y: {}\n", dice.velocidadeDirecional.x, dice.velocidadeDirecional.y);
}

//a função retorna true se o dado passado como parâmetro está colidindo com algum outro e deveria voltar pra outra direção
void Dices::checkCollisions(Dice &current_dice) {
  // Check collision between ship and asteroids
  for(auto &dice : m_dices) {
    if(&dice != &current_dice)
    {
      const auto distance{
          glm::distance(current_dice.translation, dice.translation)};

      if (distance < 1.2f) {
        if(!current_dice.dadoColidindo) {
          current_dice.dadoColidindo = true;
          //dice.dadoColidindo = true;
          current_dice.movimentoDado.x = !current_dice.movimentoDado.x;
          current_dice.movimentoDado.y = !current_dice.movimentoDado.y;
          //jogarDado(current_dice);
        }
        return;
      }
    }
  }
  current_dice.dadoColidindo = false;
  return;
}

void Dices::terminateGL(){
  for(auto dice : m_dices){
    abcg::glDeleteBuffers(1, &dice.m_EBO);
    abcg::glDeleteBuffers(1, &dice.m_VBO);
    abcg::glDeleteVertexArrays(1, &dice.m_VAO);
    //fmt::print("Dice terminated.\n");
  }
}