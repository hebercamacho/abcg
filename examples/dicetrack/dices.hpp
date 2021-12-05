#ifndef DICES_HPP_
#define DICES_HPP_

#include <vector>
#include <random>
#include "abcg.hpp"

struct Vertex {
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec2 texCoord{};
  glm::vec4 Ka;
  glm::vec4 Kd;
  glm::vec4 Ks;
  float shininess;

  bool operator==(const Vertex& other) const noexcept {
    static const auto epsilon{std::numeric_limits<float>::epsilon()};
    return glm::all(glm::epsilonEqual(position, other.position, epsilon)) &&
           glm::all(glm::epsilonEqual(normal, other.normal, epsilon)) &&
           glm::all(glm::epsilonEqual(texCoord, other.texCoord, epsilon)) &&
           glm::all(glm::epsilonEqual(Ka, other.Ka, epsilon)) &&
           glm::all(glm::epsilonEqual(Kd, other.Kd, epsilon)) &&
           glm::all(glm::epsilonEqual(Ks, other.Ks, epsilon));
  }
};

struct Dice {
  glm::mat4 modelMatrix{1.0f}; //a matriz do modelo do dado
  glm::vec3 position{0.0f}; //indica a posição tridimensional
  glm::vec3 rotationAngle{}; //indica o ângulo de rotação sobre cada um dos eixos X,Y,Z
  float timeLeft{0.0f}; //indica por quanto tempo o dado ainda continuará girando
  float spinSpeed{1.0f}; //define um ângulo para definir a velocidade do giro do dado
  bool dadoGirando{false}; //indica se o dado deve estar girando 
  glm::ivec3 DoRotateAxis{}; //indica se deve ou não girar nos eixos X,Y,Z
};

class Dices {
 public:
  void initializeGL(int quantity);
  void loadDiffuseTexture(std::string_view path);
  void loadObj(std::string_view path, bool standardize = true);
  void render() const;
  void setupVAO(GLuint program);
  void terminateGL();
  void update(float deltaTime);
  void jogarDado(Dice &);

  std::vector<Dice> dices;

 private:
  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_EBO{};

  GLuint m_diffuseTexture{};

  std::default_random_engine m_randomEngine; //gerador de números pseudo-aleatórios

  std::vector<Vertex> m_vertices;
  std::vector<GLuint> m_indices;

  bool m_hasNormals{false};
  bool m_hasTexCoords{false};

  Dice inicializarDado();
  void tempoGirandoAleatorio(Dice&);
  void eixoAlvoAleatorio(Dice&);
  void computeNormals();
  void createBuffers();
  void standardize();
};

#endif