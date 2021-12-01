#ifndef DICES_HPP_
#define DICES_HPP_

#include <vector>
#include <random>
#include "abcg.hpp"

class OpenGLWindow;

struct Vertex {
  glm::vec3 position{};
  glm::vec3 color{};

  bool operator==(const Vertex& other) const noexcept {
    return position == other.position;
  }
};

struct Dice {
  glm::mat4 modelMatrix{1.0f};
  glm::vec3 position{0.0f};
  glm::vec3 rotationAxis{0.0f, 1.0f, 0.0f};
  float rotationAngle{};
  float timeLeft{0.0f}; //indica por quanto tempo o dado ainda continuará girando
  bool dadoGirando{false}; //indica se o dado deve estar girando 
};

class Dices {
 public:
  void initializeGL(GLuint program, int quantity, std::vector<Vertex> vertices, std::vector<GLuint> indices);
  void render();
  void terminateGL();
  void update(float deltaTime);

  [[nodiscard]] int getNumTriangles() const {
    return static_cast<int>(m_indices.size()) / 3;
  }

  std::vector<Dice> dices;

 private:
  friend OpenGLWindow;
  GLuint m_program{};
  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_EBO{};

  std::vector<Vertex> m_vertices;
  std::vector<GLuint> m_indices;
  
  std::default_random_engine m_randomEngine; //gerador de números pseudo-aleatórios
  
  void createBuffers();
  void setupVAO();

  Dice inicializarDado();
  void jogarDado(Dice &);
  void pousarDado(Dice&); 
  void velocidadeAngularAleatoria(Dice&); 
  void velocidadeDirecionalAleatoria(Dice&); 
  void tempoGirandoAleatorio(Dice&);
  void checkCollisions(Dice&);
};

#endif