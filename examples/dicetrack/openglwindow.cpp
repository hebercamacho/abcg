#include "openglwindow.hpp"

#include <imgui.h>

#include <cppitertools/itertools.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "imfilebrowser.h"

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
        const auto P = m_projMatrix * (m_viewMatrix * m_modelMatrix * glm::vec4(dice.position, 1.0));
        const auto distanceX = glm::distance((mousePosition.x * (2.0f/m_viewportWidth) - 1), P.x / P.z);
        const auto distanceY = glm::distance((mousePosition.y * (-2.0f/m_viewportHeight) + 1), P.y / P.z);
        //fmt::print("distance: {} {}\n", distanceX, distanceY);
        if(distanceX <= (0.4f / P.z) && distanceY < (0.8f / P.z)) //números empíricos
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
  abcg::glClearColor(0, 0.392156f, 0, 1);
  abcg::glEnable(GL_DEPTH_TEST);

  // Create programs
  for (const auto& name : m_shaderNames) {
    const auto path{getAssetsPath() + "shaders/" + name};
    const auto program{createProgramFromFile(path + ".vert", path + ".frag")};
    m_programs.push_back(program);
  }

  // Load default model
  loadModel(getAssetsPath() + "dice.obj");
  m_mappingMode = 0;  // "Triplanar" option

  m_dices.initializeGL(quantity);
}

void OpenGLWindow::loadModel(std::string_view path) {
  m_dices.terminateGL();

  m_dices.loadDiffuseTexture(getAssetsPath() + "maps/laminado-cumaru.jpg");
  m_dices.loadObj(path);
  m_dices.setupVAO(m_programs.at(m_currentProgramIndex));
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
  const GLint IaLoc{abcg::glGetUniformLocation(program, "Ia")};
  const GLint IdLoc{abcg::glGetUniformLocation(program, "Id")};
  const GLint IsLoc{abcg::glGetUniformLocation(program, "Is")};
  const GLint diffuseTexLoc{abcg::glGetUniformLocation(program, "diffuseTex")};
  const GLint mappingModeLoc{abcg::glGetUniformLocation(program, "mappingMode")}; 

  // Set uniform variables used by every scene object
  abcg::glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, &m_viewMatrix[0][0]);
  abcg::glUniformMatrix4fv(projMatrixLoc, 1, GL_FALSE, &m_projMatrix[0][0]);

  const auto lightDirRotated{m_trackBallLight.getRotation() * m_lightDir};
  abcg::glUniform4fv(lightDirLoc, 1, &lightDirRotated.x);
  abcg::glUniform4fv(IaLoc, 1, &m_Ia.x);
  abcg::glUniform4fv(IdLoc, 1, &m_Id.x);
  abcg::glUniform4fv(IsLoc, 1, &m_Is.x);
  abcg::glUniform1i(diffuseTexLoc, 0); //candidato a virar 0
  abcg::glUniform1i(mappingModeLoc, m_mappingMode);
  
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
        m_dices.initializeGL(quantity);
      }
    }
    //Speed Slider 
    {
      ImGui::PushItemWidth(m_viewportWidth / 3);
      static float spinSpeed{1.0f};
      ImGui::SliderFloat("Speed", &spinSpeed, 0.01f, 10.0f,
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