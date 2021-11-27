#include "openglwindow.hpp"

#include <fmt/core.h>
#include <imgui.h>
#include <tiny_obj_loader.h>

#include <cppitertools/itertools.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtx/hash.hpp>
#include <unordered_map>

// Explicit specialization of std::hash for Vertex
namespace std {
template <>
//necessário para podermos usar Vertex como chave para pegar um valor de índice de uma tabela hash 
//isso ajuda a compactar bem nossa geometria indexada
struct hash<Vertex> {
  size_t operator()(Vertex const& vertex) const noexcept {
    const std::size_t h1{std::hash<glm::vec3>()(vertex.position)}; //como é 3D e hash já possui uma especialização para vec3, vamos usar isso por enquanto
    return h1;
  }
};
}  // namespace std

void OpenGLWindow::initializeGL() {
  abcg::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  // Enable depth buffering
  abcg::glEnable(GL_DEPTH_TEST); //descartar fragmentos dependendo da profundidade

  // Create program
  m_program = createProgramFromFile(getAssetsPath() + "dice.vert",
                                    getAssetsPath() + "dice.frag");

  // Load model
  loadModelFromFile(getAssetsPath() + "dice.obj"); //carregamento do .obj
  standardize();

  #if !defined(__EMSCRIPTEN__)
    abcg::glEnable(GL_PROGRAM_POINT_SIZE);
  #endif

  m_verticesToDraw = m_indices.size();
  //fmt::print("quantity: {}\n", quantity);
  m_dices.initializeGL(m_program, quantity, m_vertices, m_indices, m_verticesToDraw);
}

//carregar e ler o arquivo .obj, armazenar vertices e indices em m_vertices e m_indices.
void OpenGLWindow::loadModelFromFile(std::string_view path) {
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
      //se fizermos offset / 3 teremos o indice do triangulos?
      
      const auto material_id = shape.mesh.material_ids.at(offset/3);
      
      Vertex vertex{};
      vertex.position = {vx, vy, vz}; //a chave do vertex é sua posição
      vertex.color = {(float)material_id, (float)material_id, (float)material_id};
      // fmt::print("position x: {} color r: {}\n", vertex.position.x, vertex.color.r);

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
}

//função para centralizar o modelo na origem e aplicar escala, 
//normalizar as coordenadas de todos os vértices no intervalo [-1,1],
//modificando vertices carregados do .obj para que a geometria caiba no volume de visão do pipeline gráfico,
// que é o cubo de tamanho 2×2×2 centralizado em (0,0,0).
void OpenGLWindow::standardize() {
  // achar maiores e menores valores de x,y,z
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

  
  const auto center{(min + max) / 2.0f}; // calculo do centro da caixa
  const auto scaling{2.0f / glm::length(max - min)}; //calculo do fator de escala, de forma que a maior dimensão da caixa tenha comprimento 2
  //fmt::print("scaling: {}\n", scaling);
  for (auto& vertex : m_vertices) {
    vertex.position = (vertex.position - center) * scaling; //centralizar modelo na origem e aplicar escala
  }
}

void OpenGLWindow::paintGL() {
  
    // Clear color buffer and depth buffer
    abcg::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    abcg::glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    
    m_dices.paintGL(m_viewportWidth, m_viewportHeight, getDeltaTime());
}

void OpenGLWindow::paintUI() {
  abcg::OpenGLWindow::paintUI();
  //Janela de opções
  {
    ImGui::SetNextWindowPos(ImVec2(5,5));
    ImGui::SetNextWindowSize(ImVec2(128, 70));
    ImGui::Begin("Button window", nullptr, ImGuiWindowFlags_NoDecoration);

    ImGui::PushItemWidth(200);
    //Botão jogar dado
    if(ImGui::Button("Jogar!")){
      for(auto &dice : m_dices.m_dices){
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
      if(quantity != (int)currentIndex + 1){
        quantity = currentIndex + 1;
        initializeGL();
      }
    }

    
    ImGui::End();
  }
  
  //virar a face pra fora
  abcg::glFrontFace(GL_CW);
}

void OpenGLWindow::resizeGL(int width, int height) {
  m_viewportWidth = width;
  m_viewportHeight = height;
}

void OpenGLWindow::terminateGL() {
  abcg::glDeleteProgram(m_program);
  m_dices.terminateGL();
}