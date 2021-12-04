#include "dices.hpp"
#include <fmt/core.h>
#include <cppitertools/itertools.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtx/hash.hpp>
#include <unordered_map>

void Dices::initializeGL(GLuint program, int quantity, std::vector<Vertex> vertices, std::vector<GLuint> indices){
  terminateGL();
  // Inicializar gerador de números pseudo-aleatórios
  auto seed{std::chrono::steady_clock::now().time_since_epoch().count()};
  m_randomEngine.seed(seed);

  m_program = program;
  m_vertices = vertices;
  m_indices = indices;

  dices.clear();
  dices.resize(quantity);

  createBuffers();
  setupVAO();

  for(auto &dice : dices) {
    dice = inicializarDado();
  }
}

//função para começar o dado numa posição e número aleatório, além de inicializar algumas outras variáveis necessárias
Dice Dices::inicializarDado() {
  Dice dice;
  //define posição inicial completamente aleatória
  std::uniform_real_distribution<float> fdist(-1.0f,1.0f);
  dice.position = glm::vec3{fdist(m_randomEngine),fdist(m_randomEngine),fdist(m_randomEngine)};

  //jogarDado(dice); //começar com o dado sendo jogado 

  return dice;
}

void Dices::jogarDado(Dice &dice) {
  tempoGirandoAleatorio(dice);
  eixoAlvoAleatorio(dice);
  dice.dadoGirando = true;
}

void Dices::update(float deltaTime) {
  for(auto &dice : dices) {
    //se o dado ainda estiver girando, vamos decrementar do tempo dele
    if(dice.dadoGirando)
    {
      dice.timeLeft -= deltaTime;
      if(dice.DoRotateAxis.x)
        dice.rotationAngle.x = glm::wrapAngle(dice.rotationAngle.x + glm::radians(dice.spinSpeed) * dice.timeLeft); //definição da velocidade de rotação, grau por quadro
      if(dice.DoRotateAxis.y)
        dice.rotationAngle.y = glm::wrapAngle(dice.rotationAngle.y + glm::radians(dice.spinSpeed) * dice.timeLeft); //definição da velocidade de rotação, grau por quadro
      if(dice.DoRotateAxis.z)
        dice.rotationAngle.z = glm::wrapAngle(dice.rotationAngle.z + glm::radians(dice.spinSpeed) * dice.timeLeft); //definição da velocidade de rotação, grau por quadro
      // fmt::print("dice.rotationAngle: {}\n", dice.rotationAngle);
    }
    //se o tempo acabou, dado não está mais girando
    if(dice.dadoGirando && dice.timeLeft <= 0){
      dice.dadoGirando = false;
    }
  }
}

//função para definir tempo de giro do dado, algo entre 2 e 7 segundos 
void Dices::tempoGirandoAleatorio(Dice &dice){
  std::uniform_real_distribution<float> fdist(2.0f,7.0f);
  dice.timeLeft = fdist(m_randomEngine);
}

//função para definir um eixo de rotação aleatório para cada dado, considerando a partir do eixo atual
void Dices::eixoAlvoAleatorio(Dice &dice){
  //distribuição aleatória entre 0 e 2, para girar somente 1 eixo
  dice.DoRotateAxis = {0, 0, 0};
  std::uniform_int_distribution<int> idist(0,2);
  dice.DoRotateAxis[idist(m_randomEngine)] = 1;
}

void Dices::createBuffers() {

  // Delete previous buffers
  abcg::glDeleteBuffers(1, &m_EBO);
  abcg::glDeleteBuffers(1, &m_VBO);

  // VBO
  abcg::glGenBuffers(1, &m_VBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices[0]) * m_vertices.size(),
                     m_vertices.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // EBO
  abcg::glGenBuffers(1, &m_EBO);
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  abcg::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     sizeof(m_indices[0]) * m_indices.size(), m_indices.data(),
                     GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Dices::render(){
  abcg::glBindVertexArray(m_VAO);

  abcg::glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()),
                       GL_UNSIGNED_INT, nullptr);

  abcg::glBindVertexArray(0);
}

void Dices::setupVAO() {
  // Release previous VAO
  abcg::glDeleteVertexArrays(1, &m_VAO);

  // Create VAO
  abcg::glGenVertexArrays(1, &m_VAO);
  abcg::glBindVertexArray(m_VAO);

  // Bind EBO and VBO
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

  // Bind vertex attributes
  const GLint positionAttribute{
      abcg::glGetAttribLocation(m_program, "inPosition")};
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

  // End of binding
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
  abcg::glBindVertexArray(0);
}

void Dices::terminateGL() {
  abcg::glDeleteBuffers(1, &m_EBO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteVertexArrays(1, &m_VAO);
}