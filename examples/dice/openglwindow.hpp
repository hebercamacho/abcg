#ifndef OPENGLWINDOW_HPP_
#define OPENGLWINDOW_HPP_

#include <vector>
#include <random>
#include "abcg.hpp"
#include "dices.hpp"

class OpenGLWindow : public abcg::OpenGLWindow {
 protected:
  void initializeGL() override;
  void paintGL() override;
  void paintUI() override;
  void resizeGL(int width, int height) override;
  void terminateGL() override;

 private:
  GLuint m_program{};
  std::vector<Vertex> m_vertices; //arranjo de vértices lido do arquivo OBJ que será enviado ao VBO
  std::vector<GLuint> m_indices; //arranjo de indices lido do arquivo OBJ que será enviado ao EBO
  int m_verticesToDraw{}; //quantidade de vértices do VBO que será processada pela função de renderização, glDrawElements

  Dices m_dices;
  int quantity{1};

  int m_viewportWidth{};
  int m_viewportHeight{};

  void loadModelFromFile(std::string_view path);
  void standardize();
};

#endif