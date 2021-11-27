#ifndef DICES_HPP_
#define DICES_HPP_

#include "abcg.hpp"
#include <random>
#include <list>

class OpenGLWindow;

//atributos que definem um vértice: posição 3D, cor e operador == pra verificar se um vértice é igual a outro
struct Vertex {
  glm::vec3 position;
  glm::vec3 color;

  bool operator==(const Vertex& other) const {
    return position == other.position;
  }
};

class Dices {
  public:
    void initializeGL(GLuint program, int quantity, std::vector<Vertex>, std::vector<GLuint>,int);
    void paintGL(int m_viewportWidth, int m_viewportHeight, float deltaTime);
    void terminateGL();

  private:
    friend OpenGLWindow;

    GLuint m_program{};
    int m_verticesToDraw{}; //quantidade de vértices do VBO que será processada pela função de renderização, glDrawElements
    double m_deltaTime{};
    int m_viewportWidth{};
    int m_viewportHeight{};

    std::vector<Vertex> m_vertices; //arranjo de vértices lido do arquivo OBJ que será enviado ao VBO
    std::vector<GLuint> m_indices; //arranjo de indices lido do arquivo OBJ que será enviado ao EBO

    struct Dice {
      GLuint m_VAO{};
      GLuint m_VBO{};
      GLuint m_EBO{};

      glm::vec3 m_angle{}; // ângulo de rotação que será enviado à variável uniforme do vertex shader.
      glm::ivec3 m_rotation{}; // nos ajuda a decidir qual a direção da rotação
      glm::vec3 velocidadeAngular{}; //indica quantos graus/rad o dado deve girar por unidade de tempo, em cada um dos eixos x,y,z
      glm::vec2 velocidadeDirecional{};
      glm::vec3 translation{}; //indica a posição transladada do dado
      glm::bvec2 movimentoDado{true, true}; //false = irá pra esquerda/baixo, true = irá pra direita/cima

      float myTime{}; //auxiliar para conseguirmos não girar o tempo todo
      bool dadoGirando{false}; //indica se o dado deve estar girando 
      bool dadoColidindo{false};
      int quadros; //contador de quadros, auxilia no tempo que o dado fica girando
      int maxQuadros;
    };
    //lista de ângulos cuja face do dado fica virada para a tela. Poderia ser maior, mas o resultado final seria pouco diferente.
    std::array<glm::vec3, 7> angulosRetos{
      glm::vec3{0.0f,0.0f,0.0f}, //0 apenas pra manter o numero do dado igual ao numero do indice
      glm::vec3{125.0f,120.0f,45.0f}, //1
      glm::vec3{345.0f,170.0f,15.0f}, //2
      glm::vec3{75.0f,190.0f,13.0f}, //3
      glm::vec3{75.0f,20.0f,77.0f},//4
      glm::vec3{347.0f,342.0f,75.0f}, //5 
      glm::vec3{105.0f,300.0f,45.0f} //6
    };

    std::vector<Dice> m_dices;

    std::default_random_engine m_randomEngine; //gerador de números pseudo-aleatórios

    Dices::Dice inicializarDado();
    void jogarDado(Dice &); 
    void pousarDado(Dice&); 
    void velocidadeAngularAleatoria(Dice&); 
    void velocidadeDirecionalAleatoria(Dice&); 
    void tempoGirandoAleatorio(Dice&);
    void checkCollisions(Dice&);
};

#endif