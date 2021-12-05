#include "dices.hpp"

#include <fmt/core.h>
#include <tiny_obj_loader.h>
#include <glm/gtx/fast_trigonometry.hpp>
#include <cppitertools/itertools.hpp>
#include <filesystem>
#include <glm/gtx/hash.hpp>
#include <unordered_map>

// Explicit specialization of std::hash for Vertex
namespace std {
template <>
struct hash<Vertex> {
  size_t operator()(Vertex const& vertex) const noexcept {
    const std::size_t h1{std::hash<glm::vec3>()(vertex.position)};
    const std::size_t h2{std::hash<glm::vec3>()(vertex.normal)};
    const std::size_t h3{std::hash<glm::vec2>()(vertex.texCoord)};
    return h1 ^ h2 ^ h3;
  }
};
}  // namespace std

void Dices::initializeGL(int quantity){
  // Inicializar gerador de números pseudo-aleatórios
  auto seed{std::chrono::steady_clock::now().time_since_epoch().count()};
  m_randomEngine.seed(seed);

  dices.clear();
  dices.resize(quantity);

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

void Dices::computeNormals() {
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

void Dices::loadDiffuseTexture(std::string_view path) {
  if (!std::filesystem::exists(path)) return;

  abcg::glDeleteTextures(1, &m_diffuseTexture);
  m_diffuseTexture = abcg::opengl::loadTexture(path);
}

void Dices::loadObj(std::string_view path, bool standardize) {
  const auto basePath{std::filesystem::path{path}.parent_path().string() + "/"};

  tinyobj::ObjReaderConfig readerConfig;
  readerConfig.mtl_search_path = basePath;  // Path to material files

  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(path.data(), readerConfig)) {
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
  const auto& materials{reader.GetMaterials()};

  m_vertices.clear();
  m_indices.clear();

  m_hasNormals = false;
  m_hasTexCoords = false;

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

      // Vertex texture coordinates
      float tu{};
      float tv{};
      if (index.texcoord_index >= 0) {
        m_hasTexCoords = true;
        const int texCoordsStartIndex{2 * index.texcoord_index};
        tu = attrib.texcoords.at(texCoordsStartIndex + 0);
        tv = attrib.texcoords.at(texCoordsStartIndex + 1);
      }

      // Vertex material
      glm::vec4 Ka{1.0f}; // Default value
      glm::vec4 Kd{0.7f, 0.7f, 0.7f, 1.0f}; // Default value
      glm::vec4 Ks{0.5f, 0.5f, 0.5f, 1.0f}; // Default value
      float shininess{25.0f}; // Default value
      if (!materials.empty()) {
        const int id_material = shape.mesh.material_ids.at(offset / 3);
        const auto& mat{materials.at(id_material)};
        Ka = glm::vec4(mat.ambient[0], mat.ambient[1], mat.ambient[2], 1);
        Kd = glm::vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1);
        Ks = glm::vec4(mat.specular[0], mat.specular[1], mat.specular[2], 1);
        shininess = mat.shininess;
        if (m_diffuseTexture == 0 && !mat.diffuse_texname.empty())
          loadDiffuseTexture(basePath + mat.diffuse_texname);
      }

      Vertex vertex{};
      vertex.position = {vx, vy, vz};
      vertex.normal = {nx, ny, nz};
      vertex.texCoord = {tu, tv};
      vertex.Ka = Ka;
      vertex.Kd = Kd;
      vertex.Ks = Ks;
      vertex.shininess = shininess;

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

  createBuffers();
}

void Dices::render() const {
  abcg::glBindVertexArray(m_VAO);

  abcg::glActiveTexture(GL_TEXTURE0);
  abcg::glBindTexture(GL_TEXTURE_2D, m_diffuseTexture);

  // Set minification and magnification parameters
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Set texture wrapping parameters
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  abcg::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  abcg::glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()),
                       GL_UNSIGNED_INT, nullptr);

  abcg::glBindVertexArray(0);
}

void Dices::setupVAO(GLuint program) {
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
      abcg::glGetAttribLocation(program, "inPosition")};
  if (positionAttribute >= 0) {
    abcg::glEnableVertexAttribArray(positionAttribute);
    abcg::glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex), nullptr);
  }
  //aqui a gente passa a normal do vértice já pronta para o shader
  const GLint normalAttribute{abcg::glGetAttribLocation(program, "inNormal")};
  if (normalAttribute >= 0) {
    abcg::glEnableVertexAttribArray(normalAttribute);
    GLsizei offset{sizeof(glm::vec3)};
    abcg::glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex),
                                reinterpret_cast<void*>(offset));
  }
  //passar textura se tiver
  const GLint texCoordAttribute{
      abcg::glGetAttribLocation(program, "inTexCoord")};
  if (texCoordAttribute >= 0) {
    abcg::glEnableVertexAttribArray(texCoordAttribute);
    GLsizei offset{sizeof(glm::vec3) + sizeof(glm::vec3)};
    abcg::glVertexAttribPointer(texCoordAttribute, 2, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex),
                                reinterpret_cast<void*>(offset));
  }
  //passar propriedades do material
  const GLint KaAttribute{
      abcg::glGetAttribLocation(program, "inKa")};
  if (KaAttribute >= 0) {
    abcg::glEnableVertexAttribArray(KaAttribute);
    GLsizei offset{sizeof(glm::vec3) * 2 + sizeof(glm::vec2)};
    abcg::glVertexAttribPointer(KaAttribute, 4, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex),
                                reinterpret_cast<void*>(offset));
  }
  const GLint KdAttribute{
      abcg::glGetAttribLocation(program, "inKd")};
  if (KdAttribute >= 0) {
    abcg::glEnableVertexAttribArray(KdAttribute);
    GLsizei offset{sizeof(glm::vec3) * 2 + sizeof(glm::vec2) + sizeof(glm::vec4)};
    abcg::glVertexAttribPointer(KdAttribute, 4, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex),
                                reinterpret_cast<void*>(offset));
  }
  const GLint KsAttribute{
      abcg::glGetAttribLocation(program, "inKs")};
  if (KsAttribute >= 0) {
    abcg::glEnableVertexAttribArray(KsAttribute);
    GLsizei offset{sizeof(glm::vec3) * 2 + sizeof(glm::vec2) + sizeof(glm::vec4) * 2};
    abcg::glVertexAttribPointer(KsAttribute, 4, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex),
                                reinterpret_cast<void*>(offset));
  }
  const GLint ShininessAttribute{
      abcg::glGetAttribLocation(program, "inShininess")};
  if (ShininessAttribute >= 0) {
    abcg::glEnableVertexAttribArray(ShininessAttribute);
    GLsizei offset{sizeof(glm::vec3) * 2 + sizeof(glm::vec2) + sizeof(glm::vec4) * 3};
    abcg::glVertexAttribPointer(ShininessAttribute, 1, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex),
                                reinterpret_cast<void*>(offset));
  }

  // End of binding
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);
  abcg::glBindVertexArray(0);
}

void Dices::standardize() {
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

void Dices::terminateGL() {
  abcg::glDeleteTextures(1, &m_diffuseTexture);
  abcg::glDeleteBuffers(1, &m_EBO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteVertexArrays(1, &m_VAO);
}