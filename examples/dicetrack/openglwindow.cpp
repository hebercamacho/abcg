#include "openglwindow.hpp"

#include <imgui.h>
#include "imfilebrowser.h"
#include <tiny_obj_loader.h>
#include <fmt/core.h>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <cppitertools/itertools.hpp>

// Explicit specialization of std::hash for Vertex
namespace std {
template <>
struct hash<Vertex> {
  size_t operator()(Vertex const& vertex) const noexcept {
    const std::size_t h1{std::hash<glm::vec3>()(vertex.position)};
    const std::size_t h2{std::hash<glm::vec3>()(vertex.normal)};
    return h1 ^ h2;
  }
};
}  // namespace std

void OpenGLWindow::handleEvent(SDL_Event& event) {
  glm::ivec2 mousePosition;
  SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

  if (event.type == SDL_MOUSEMOTION) {
    m_trackBallModel.mouseMove(mousePosition);
    m_trackBallLight.mouseMove(mousePosition);
  }
  if (event.type == SDL_MOUSEBUTTONDOWN) {
    if (event.button.button == SDL_BUTTON_LEFT) {
      m_trackBallModel.mousePress(mousePosition);
    }
    if (event.button.button == SDL_BUTTON_RIGHT) {
      m_trackBallLight.mousePress(mousePosition);
    }
  }
  if (event.type == SDL_MOUSEBUTTONUP) {
    if (event.button.button == SDL_BUTTON_LEFT) {
      m_trackBallModel.mouseRelease(mousePosition);
      //fmt::print("mouse position: {} {}\n", mousePosition.x * (2.0f/m_viewportWidth) - 1, mousePosition.y * (-2.0f/m_viewportHeight) + 1);
      for(auto &dice : m_dices.dices){
          const auto distanceX = glm::distance((mousePosition.x * (2.0f/m_viewportWidth) - 1), dice.modelMatrix[3].x);
          const auto distanceY = glm::distance((mousePosition.y * (-2.0f/m_viewportHeight) + 1), dice.modelMatrix[3].y);
          //fmt::print("distance: {} {}\n", distanceX, distanceY);
          if(distanceX < 0.4f && distanceY < 0.4f)
            m_dices.jogarDado(dice);
      }
    }
    if (event.button.button == SDL_BUTTON_RIGHT) {
      m_trackBallLight.mouseRelease(mousePosition);
    }
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

  // Create programs
  for (const auto& name : m_shaderNames) {
    const auto program{createProgramFromFile(getAssetsPath() + name + ".vert",
                                             getAssetsPath() + name + ".frag")};
    m_programs.push_back(program);
  }

  // Load model
  loadObj(getAssetsPath() + "dice.obj");

  m_dices.initializeGL(m_programs.at(m_currentProgramIndex), quantity, m_vertices, m_indices);
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

  const auto& attrib{reader.GetAttrib()};
  const auto& shapes{reader.GetShapes()};

  m_vertices.clear();
  m_indices.clear();

  m_hasNormals = false;

  // A key:value map with key=Vertex and value=index
  std::unordered_map<Vertex, GLuint> hash{};

  // Loop over shapes
  for (const auto& shape : shapes) {
    // Loop over indices
    for (const auto offset : iter::range(shape.mesh.indices.size())) {
      // Access to vertex
      const tinyobj::index_t index{shape.mesh.indices.at(offset)};

      // Vertex position
      const int startIndex{3 * index.vertex_index};
      const float vx{attrib.vertices.at(startIndex + 0)};
      const float vy{attrib.vertices.at(startIndex + 1)};
      const float vz{attrib.vertices.at(startIndex + 2)};

      // Vertex normal
      float nx{};
      float ny{};
      float nz{};
      if (index.normal_index >= 0) {
        m_hasNormals = true;
        const int normalStartIndex{3 * index.normal_index};
        nx = attrib.normals.at(normalStartIndex + 0);
        ny = attrib.normals.at(normalStartIndex + 1);
        nz = attrib.normals.at(normalStartIndex + 2);
      }

      Vertex vertex{};
      vertex.position = {vx, vy, vz};
      vertex.normal = {nx, ny, nz};

      // If hash doesn't contain this vertex
      if (hash.count(vertex) == 0) {
        // Add this index (size of m_vertices)
        hash[vertex] = m_vertices.size();
        // Add this vertex
        m_vertices.push_back(vertex);
      }

      m_indices.push_back(hash[vertex]);
    }
  }

  if (standardize) {
    this->standardize();
  }

  if (!m_hasNormals) {
    computeNormals();
  }
}

void OpenGLWindow::computeNormals() {
  // Clear previous vertex normals
  for (auto& vertex : m_vertices) {
    vertex.normal = glm::zero<glm::vec3>();
  }

  // Compute face normals
  for (const auto offset : iter::range<int>(0, m_indices.size(), 3)) {
    // Get face vertices
    Vertex& a{m_vertices.at(m_indices.at(offset + 0))};
    Vertex& b{m_vertices.at(m_indices.at(offset + 1))};
    Vertex& c{m_vertices.at(m_indices.at(offset + 2))};

    // Compute normal
    const auto edge1{b.position - a.position};
    const auto edge2{c.position - b.position};
    const glm::vec3 normal{glm::cross(edge1, edge2)};

    // Accumulate on vertices
    a.normal += normal;
    b.normal += normal;
    c.normal += normal;
  }

  // Normalize
  for (auto& vertex : m_vertices) {
    vertex.normal = glm::normalize(vertex.normal);
  }

  m_hasNormals = true;
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

  // Use currently selected program
  const auto program{m_programs.at(m_currentProgramIndex)};
  abcg::glUseProgram(program);

  // Get location of uniform variables (could be precomputed)
  const GLint viewMatrixLoc{abcg::glGetUniformLocation(program, "viewMatrix")};
  const GLint projMatrixLoc{abcg::glGetUniformLocation(program, "projMatrix")};
  const GLint modelMatrixLoc{abcg::glGetUniformLocation(program, "modelMatrix")};
  const GLint normalMatrixLoc{abcg::glGetUniformLocation(program, "normalMatrix")};
  const GLint lightDirLoc{abcg::glGetUniformLocation(program, "lightDirWorldSpace")};
  const GLint shininessLoc{abcg::glGetUniformLocation(program, "shininess")};
  const GLint IaLoc{abcg::glGetUniformLocation(program, "Ia")};
  const GLint IdLoc{abcg::glGetUniformLocation(program, "Id")};
  const GLint IsLoc{abcg::glGetUniformLocation(program, "Is")};
  const GLint KaLoc{abcg::glGetUniformLocation(program, "Ka")};
  const GLint KdLoc{abcg::glGetUniformLocation(program, "Kd")};
  const GLint KsLoc{abcg::glGetUniformLocation(program, "Ks")};

  // Set uniform variables used by every scene object
  abcg::glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, &m_viewMatrix[0][0]);
  abcg::glUniformMatrix4fv(projMatrixLoc, 1, GL_FALSE, &m_projMatrix[0][0]);

  const auto lightDirRotated{m_trackBallLight.getRotation() * m_lightDir};
  abcg::glUniform4fv(lightDirLoc, 1, &lightDirRotated.x);
  abcg::glUniform1f(shininessLoc, m_shininess);
  abcg::glUniform4fv(IaLoc, 1, &m_Ia.x);
  abcg::glUniform4fv(IdLoc, 1, &m_Id.x);
  abcg::glUniform4fv(IsLoc, 1, &m_Is.x);
  abcg::glUniform4fv(KaLoc, 1, &m_Ka.x);
  abcg::glUniform4fv(KdLoc, 1, &m_Kd.x);
  abcg::glUniform4fv(KsLoc, 1, &m_Ks.x);
  
  // Set uniform variables of the current object
  for(auto &dice : m_dices.dices){
    // fmt::print("dice.modelMatrix.xyzw: {} {} {} {}\n", dice.modelMatrix[0][0], dice.modelMatrix[1][1], dice.modelMatrix[2][2], dice.modelMatrix[3][3]);
    //dice.modelMatrix = m_dicesMatrix;
    dice.modelMatrix = glm::translate(m_modelMatrix, dice.position);
    dice.modelMatrix = glm::scale(dice.modelMatrix, glm::vec3(0.5f));
    dice.modelMatrix = glm::rotate(dice.modelMatrix, dice.rotationAngle.x, glm::vec3(1.0f, 0.0f, 0.0f));
    dice.modelMatrix = glm::rotate(dice.modelMatrix, dice.rotationAngle.y, glm::vec3(0.0f, 1.0f, 0.0f));
    dice.modelMatrix = glm::rotate(dice.modelMatrix, dice.rotationAngle.z, glm::vec3(0.0f, 0.0f, 1.0f));
    //debug
    //fmt::print("dice.modelMatrix.xyzw: {} {} {} {}\n", dice.modelMatrix[0][0], dice.modelMatrix[1][1], dice.modelMatrix[2][2], dice.modelMatrix[3][3]);

    abcg::glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &dice.modelMatrix[0][0]);

    const auto modelViewMatrix{glm::mat3(m_viewMatrix * dice.modelMatrix)};
    glm::mat3 normalMatrix{glm::inverseTranspose(modelViewMatrix)};
    abcg::glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

    m_dices.render();
  }

  abcg::glUseProgram(0);
}

void OpenGLWindow::paintUI() {
  abcg::OpenGLWindow::paintUI();

  static ImGui::FileBrowser fileDialog;
  fileDialog.SetTitle("Load 3D Model");
  fileDialog.SetTypeFilters({".obj"});
  fileDialog.SetWindowSize(m_viewportWidth * 0.8f, m_viewportHeight * 0.8f);

  // Only in WebGL
#if defined(__EMSCRIPTEN__)
  fileDialog.SetPwd(getAssetsPath());
#endif

  // Create a window for the other widgets
  {
    const auto widgetSize{ImVec2(222, 168)};
    ImGui::SetNextWindowPos(ImVec2(m_viewportWidth - widgetSize.x - 5, 5));
    ImGui::SetNextWindowSize(widgetSize);
    ImGui::Begin("Widget window", nullptr, ImGuiWindowFlags_NoDecoration);

    // // Slider will be stretched horizontally
    // ImGui::PushItemWidth(widgetSize.x - 16);
    // ImGui::SliderInt("", &m_trianglesToDraw, 0, m_dices.getNumTriangles(),
    //                  "%d triangles");
    // ImGui::PopItemWidth();

    static bool faceCulling{};
    ImGui::Checkbox("Back-face culling", &faceCulling);

    if (faceCulling) {
      abcg::glEnable(GL_CULL_FACE);
    } else {
      abcg::glDisable(GL_CULL_FACE);
    }

    // CW/CCW combo box
    {
      static std::size_t currentIndex{};
      std::vector<std::string> comboItems{"CCW", "CW"};

      ImGui::PushItemWidth(120);
      if (ImGui::BeginCombo("Front face",
                            comboItems.at(currentIndex).c_str())) {
        for (auto index : iter::range(comboItems.size())) {
          const bool isSelected{currentIndex == index};
          if (ImGui::Selectable(comboItems.at(index).c_str(), isSelected))
            currentIndex = index;
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      if (currentIndex == 0) {
        abcg::glFrontFace(GL_CCW);
      } else {
        abcg::glFrontFace(GL_CW);
      }
    }

    // Projection combo box
    {
      static std::size_t currentIndex{};
      std::vector<std::string> comboItems{"Perspective", "Orthographic"};

      ImGui::PushItemWidth(120);
      if (ImGui::BeginCombo("Projection",
                            comboItems.at(currentIndex).c_str())) {
        for (auto index : iter::range(comboItems.size())) {
          const bool isSelected{currentIndex == index};
          if (ImGui::Selectable(comboItems.at(index).c_str(), isSelected))
            currentIndex = index;
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      if (currentIndex == 0) {
        const auto aspect{static_cast<float>(m_viewportWidth) /
                          static_cast<float>(m_viewportHeight)};
        m_projMatrix =
            glm::perspective(glm::radians(45.0f), aspect, 0.1f, 5.0f);

      } else {
        m_projMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 5.0f);
      }
    }

    // Shader combo box
    {
      static std::size_t currentIndex{};

      ImGui::PushItemWidth(120);
      if (ImGui::BeginCombo("Shader", m_shaderNames.at(currentIndex))) {
        for (const auto index : iter::range(m_shaderNames.size())) {
          const bool isSelected{currentIndex == index};
          if (ImGui::Selectable(m_shaderNames.at(index), isSelected))
            currentIndex = index;
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      // Set up VAO if shader program has changed
      if (static_cast<int>(currentIndex) != m_currentProgramIndex) {
        m_currentProgramIndex = currentIndex;
        m_dices.initializeGL(m_programs.at(m_currentProgramIndex), quantity, m_vertices, m_indices);
      }
    }

    if (ImGui::Button("Load 3D Model...", ImVec2(-1, -1))) {
      fileDialog.Open();
    }

    ImGui::End();
  }

  // Create window for light sources
  if (m_currentProgramIndex < 3) {
    const auto widgetSize{ImVec2(222, 244)};
    ImGui::SetNextWindowPos(ImVec2(m_viewportWidth - widgetSize.x - 5,
                                   m_viewportHeight - widgetSize.y - 5));
    ImGui::SetNextWindowSize(widgetSize);
    ImGui::Begin(" ", nullptr, ImGuiWindowFlags_NoDecoration);

    ImGui::Text("Light properties");

    // Slider to control light properties
    ImGui::PushItemWidth(widgetSize.x - 36);
    ImGui::ColorEdit3("Ia", &m_Ia.x, ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Id", &m_Id.x, ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Is", &m_Is.x, ImGuiColorEditFlags_Float);
    ImGui::PopItemWidth();

    ImGui::Spacing();

    ImGui::Text("Material properties");

    // Slider to control material properties
    ImGui::PushItemWidth(widgetSize.x - 36);
    ImGui::ColorEdit3("Ka", &m_Ka.x, ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Kd", &m_Kd.x, ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Ks", &m_Ks.x, ImGuiColorEditFlags_Float);
    ImGui::PopItemWidth();

    // Slider to control the specular shininess
    ImGui::PushItemWidth(widgetSize.x - 16);
    ImGui::SliderFloat("", &m_shininess, 0.0f, 500.0f, "shininess: %.1f");
    ImGui::PopItemWidth();

    ImGui::End();
  }

  fileDialog.Display();

  if (fileDialog.HasSelected()) {
    // Load model
    loadObj(fileDialog.GetSelected().string());
    m_dices.initializeGL(m_programs.at(m_currentProgramIndex), quantity, m_vertices, m_indices);
    fileDialog.ClearSelected();
  }
  //Janela de opções
  {
    ImGui::SetNextWindowPos(ImVec2(m_viewportWidth / 3, m_viewportHeight - 100));
    ImGui::SetNextWindowSize(ImVec2(-1, -1));
    ImGui::Begin("Button window", nullptr, ImGuiWindowFlags_NoDecoration);

    //Botão jogar dado
    if(ImGui::Button("Jogar todos!")){
      for(auto &dice : m_dices.dices){
        m_dices.jogarDado(dice);
      }
    }
    // Number of dices combo box
    {
      static std::size_t currentIndex{};
      const std::vector<std::string> comboItems{"1", "2", "3", "4", "5", "6"};

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
        m_dices.initializeGL(m_programs.at(m_currentProgramIndex), quantity, m_vertices, m_indices);
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

  m_trackBallModel.resizeViewport(width, height);
  m_trackBallLight.resizeViewport(width, height);
}

void OpenGLWindow::terminateGL() {
  m_dices.terminateGL();
  for (const auto& program : m_programs) {
    abcg::glDeleteProgram(program);
  }
}

void OpenGLWindow::update() {
  // Animate angle by 90 degrees per second
  const float deltaTime{static_cast<float>(getDeltaTime())};

  m_dices.update(deltaTime);

  m_modelMatrix = m_trackBallModel.getRotation();

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