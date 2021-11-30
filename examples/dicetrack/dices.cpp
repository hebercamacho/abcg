#include "dices.hpp"

#include <cppitertools/itertools.hpp>
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
  std::uniform_real_distribution<float> fdist(-1.0f,1.0f);
  dice.position = glm::vec3{fdist(m_randomEngine),fdist(m_randomEngine),fdist(m_randomEngine)}; //posição inicial completamente aleatória
  dice.rotation = glm::normalize(glm::vec3{fdist(m_randomEngine),fdist(m_randomEngine),fdist(m_randomEngine)});

  //estado inicial de algumas variáveis
  // dice.m_rotation = {0, 0, 0};  
  // dice.velocidadeAngular = {0.0f, 0.0f, 0.0f};
  // dice.myTime = 0.0f;
  // dice.quadros=0;

  // std::uniform_real_distribution<float> fdist(-1.5f,1.5f);
  // dice.translation = {fdist(m_randomEngine),fdist(m_randomEngine),0.0f};
  // pousarDado(dice); //começar num numero aleatorio

  return dice;
}

void Dices::jogarDado(Dice &dice) {
  std::uniform_real_distribution<float> fdist(-1.0f,1.0f);
  dice.rotation = glm::normalize(glm::vec3{fdist(m_randomEngine),fdist(m_randomEngine),fdist(m_randomEngine)});
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