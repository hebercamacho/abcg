#include "openglwindow.hpp"

#include <imgui.h>
#include <tiny_obj_loader.h>
#include <fmt/core.h>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <cppitertools/itertools.hpp>

// Explicit specialization of std::hash for Vertex
namespace std {
template <>
struct hash<Vertex> {
  size_t operator()(Vertex const& vertex) const noexcept {
    const std::size_t h1{std::hash<glm::vec3>()(vertex.position)};
    return h1;
  }
};
}  // namespace std

void OpenGLWindow::handleEvent(SDL_Event& event) {
  glm::ivec2 mousePosition;
  SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

  if (event.type == SDL_MOUSEMOTION) {
    m_trackBall.mouseMove(mousePosition);
  }
  if (event.type == SDL_MOUSEBUTTONDOWN &&
      event.button.button == SDL_BUTTON_LEFT) {
    m_trackBall.mousePress(mousePosition);
  }
  if (event.type == SDL_MOUSEBUTTONUP &&
      event.button.button == SDL_BUTTON_LEFT) {
    m_trackBall.mouseRelease(mousePosition);
  }
  if (event.type == SDL_MOUSEWHEEL) {
    m_zoom += (event.wheel.y > 0 ? 1.0f : -1.0f) / 5.0f;
    m_zoom = glm::clamp(m_zoom, -1.5f, 1.0f);
  }
}

void OpenGLWindow::initializeGL() {
  abcg::glClearColor(0, 0, 0, 1);

  // Enable depth buffering
  abcg::glEnable(GL_DEPTH_TEST);

  // Create program
  m_program = createProgramFromFile(getAssetsPath() + "depth.vert",
                                    getAssetsPath() + "depth.frag");

  // Load model
  loadObj(getAssetsPath() + "dice.obj");

  m_dices.initializeGL(m_program, quantity, m_vertices, m_indices);
}

void OpenGLWindow::loadObj(std::string_view path,bool standardize) {
  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(path.data())) {
    if (!reader.Error().empty()) {
      throw abcg::Exception{abcg::Exception::Runtime(
          fmt::format("Failed to load model {} ({})", path, reader.Error()))};
    }
    throw abcg::Exception{
        abcg::Exception::Runtime(fmt::format("Failed to load model {}", path))};
  }

  if (!reader.Warning().empty()) {
    fmt::print("Warning: {}\n", reader.Warning());
  }

  const auto& attrib{reader.GetAttrib()}; //conjunto de vertices
  const auto& shapes{reader.GetShapes()}; //conjunto de objetos (só tem 1)

  m_vertices.clear();
  m_indices.clear();

  // A key:value map with key=Vertex and value=index
  std::unordered_map<Vertex, GLuint> hash{};

  // ler todos os triangulos e vertices
  for (const auto& shape : shapes) { 
    // pra cada um dos indices
    for (const auto offset : iter::range(shape.mesh.indices.size())) { //122112 indices = numero de triangulos * 3
      // Access to vertex
      const tinyobj::index_t index{shape.mesh.indices.at(offset)}; //offset vai ser de 0 a 122112, index vai acessar cada vertice nessas posições offset

      // Vertex position
      const int startIndex{3 * index.vertex_index}; //startIndex vai encontrar o indice exato de cada vertice
      const float vx{attrib.vertices.at(startIndex + 0)};
      const float vy{attrib.vertices.at(startIndex + 1)};
      const float vz{attrib.vertices.at(startIndex + 2)};

      //são 40704 triangulos, dos quais 27264 brancos.
      //se fizermos offset / 3 teremos o indice do triangulos
      
      const auto material_id = shape.mesh.material_ids.at(offset/3);
      
      Vertex vertex{};
      vertex.position = {vx, vy, vz}; //a chave do vertex é sua posição
      vertex.color = {(float)material_id, (float)material_id, (float)material_id};

      // If hash doesn't contain this vertex
      if (hash.count(vertex) == 0) {
        // Add this index (size of m_vertices)
        hash[vertex] = m_vertices.size(); //o valor do hash é a ordem que esse vertex foi lido
        // Add this vertex
        m_vertices.push_back(vertex); //o vértice é adicionado ao arranjo de vértices, se ainda não existir
      }
      //no arranjo de índices, podem haver posições duplicadas, pois os vértices podem ser compartilhados por triangulos diferentes
      m_indices.push_back(hash[vertex]); //o valor do hash deste vértice (suua ordem) é adicionado ao arranjo de indices
    }
  }

  if (standardize) {
    this->standardize();
  }
}

void OpenGLWindow::standardize() {
  // Center to origin and normalize largest bound to [-1, 1]

  // Get bounds
  glm::vec3 max(std::numeric_limits<float>::lowest());
  glm::vec3 min(std::numeric_limits<float>::max());
  for (const auto& vertex : m_vertices) {
    max.x = std::max(max.x, vertex.position.x);
    max.y = std::max(max.y, vertex.position.y);
    max.z = std::max(max.z, vertex.position.z);
    min.x = std::min(min.x, vertex.position.x);
    min.y = std::min(min.y, vertex.position.y);
    min.z = std::min(min.z, vertex.position.z);
  }

  // Center and scale
  const auto center{(min + max) / 2.0f};
  const auto scaling{2.0f / glm::length(max - min)};
  for (auto& vertex : m_vertices) {
    vertex.position = (vertex.position - center) * scaling;
  }
}

void OpenGLWindow::paintGL() {
  update();

  // Clear color buffer and depth buffer
  abcg::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  abcg::glViewport(0, 0, m_viewportWidth, m_viewportHeight);

  abcg::glUseProgram(m_program);

  // Get location of uniform variables (could be precomputed)
  const GLint viewMatrixLoc{
      abcg::glGetUniformLocation(m_program, "viewMatrix")};
  const GLint projMatrixLoc{
      abcg::glGetUniformLocation(m_program, "projMatrix")};
  const GLint modelMatrixLoc{
      abcg::glGetUniformLocation(m_program, "modelMatrix")};

  // Set uniform variables used by every scene object
  abcg::glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, &m_viewMatrix[0][0]);
  abcg::glUniformMatrix4fv(projMatrixLoc, 1, GL_FALSE, &m_projMatrix[0][0]);

  for(auto &dice : m_dices.dices){
    // fmt::print("dice.modelMatrix.xyzw: {} {} {} {}\n", dice.modelMatrix[0][0], dice.modelMatrix[1][1], dice.modelMatrix[2][2], dice.modelMatrix[3][3]);
    //dice.modelMatrix = m_modelMatrix;
    dice.modelMatrix = glm::translate(m_modelMatrix, dice.position);
    dice.modelMatrix = glm::scale(dice.modelMatrix, glm::vec3(0.5f));
    dice.modelMatrix = glm::rotate(dice.modelMatrix, dice.rotationAngle.x, glm::vec3(1.0f, 0.0f, 0.0f));
    dice.modelMatrix = glm::rotate(dice.modelMatrix, dice.rotationAngle.y, glm::vec3(0.0f, 1.0f, 0.0f));
    dice.modelMatrix = glm::rotate(dice.modelMatrix, dice.rotationAngle.z, glm::vec3(0.0f, 0.0f, 1.0f));
    //debug
    // fmt::print("dice.modelMatrix.xyzw: {} {} {} {}\n", dice.modelMatrix[0][0], dice.modelMatrix[1][1], dice.modelMatrix[2][2], dice.modelMatrix[3][3]);

    // Set uniform variables of the current object
    abcg::glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &dice.modelMatrix[0][0]);

    m_dices.render();
  }

  abcg::glUseProgram(0);
}

void OpenGLWindow::paintUI() {
  abcg::OpenGLWindow::paintUI();
  //Janela de opções
  {
    ImGui::SetNextWindowPos(ImVec2(m_viewportWidth / 3, 5));
    ImGui::SetNextWindowSize(ImVec2(-1, -1));
    ImGui::Begin("Button window", nullptr, ImGuiWindowFlags_NoDecoration);

    ImGui::PushItemWidth(200);
    //Botão jogar dado
    if(ImGui::Button("Jogar!")){
      for(auto &dice : m_dices.dices){
        m_dices.jogarDado(dice);
      }
    }
    ImGui::PopItemWidth();
    // Number of dices combo box
    {
      static std::size_t currentIndex{};
      const std::vector<std::string> comboItems{"1", "2", "3"};

      ImGui::PushItemWidth(70);
      if (ImGui::BeginCombo("Dados",
                            comboItems.at(currentIndex).c_str())) {
        for (const auto index : iter::range(comboItems.size())) {
          const bool isSelected{currentIndex == index};
          if (ImGui::Selectable(comboItems.at(index).c_str(), isSelected))
            currentIndex = index;
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();
      if(quantity != (int)currentIndex + 1){ //se mudou
        quantity = currentIndex + 1;
        m_dices.initializeGL(m_program, quantity, m_vertices, m_indices);
      }
    }
    //Speed Slider 
    {
      ImGui::PushItemWidth(m_viewportWidth / 2);

      ImGui::SliderFloat("", &spinSpeed, 0.01f, 45.0f,
                       "%1f Degrees");
      for(auto &dice : m_dices.dices){
        dice.spinSpeed = spinSpeed;
      }
      ImGui::PopItemWidth();
    }

    ImGui::End();
  }
}

void OpenGLWindow::resizeGL(int width, int height) {
  m_viewportWidth = width;
  m_viewportHeight = height;

  m_trackBall.resizeViewport(width, height);
}

void OpenGLWindow::terminateGL() {
  m_dices.terminateGL();
  abcg::glDeleteProgram(m_program);
}

void OpenGLWindow::update() {
  // Animate angle by 90 degrees per second
  const float deltaTime{static_cast<float>(getDeltaTime())};

  m_dices.update(deltaTime);

  m_angle = glm::wrapAngle(m_angle + glm::radians(90.0f) * deltaTime);

  m_modelMatrix = m_trackBall.getRotation();

  m_viewMatrix =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f + m_zoom),
                  glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

  //define perspective projection
  const auto aspect{static_cast<float>(m_viewportWidth) /
                        static_cast<float>(m_viewportHeight)};
  m_projMatrix =
      glm::perspective(glm::radians(45.0f), aspect, 0.1f, 5.0f);

  //interior não é invisível
  abcg::glDisable(GL_CULL_FACE);

  //virar a face pra fora
  abcg::glFrontFace(GL_CCW);
}