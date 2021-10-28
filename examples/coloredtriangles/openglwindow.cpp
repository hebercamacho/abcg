#include "openglwindow.hpp"

#include <imgui.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "abcg.hpp"

void OpenGLWindow::initializeGL() {
  const auto *vertexShader{R"gl(
    #version 410
    layout(location = 0) in vec2 inPosition;
    layout(location = 1) in vec4 inColor;

    out vec4 fragColor;

    void main() { 
      gl_Position = vec4(inPosition, 0, 1);
      fragColor = inColor; 
    }
  )gl"}; //Este vertex shader define dois atributos de entrada: inPosition, que recebe a posição 2D do vértice, e inColor que recebe a cor RGBA.
  //A saída, fragColor, é também uma cor RGBA.
  const auto *fragmentShader{R"gl(
    #version 410

    in vec4 fragColor;

    out vec4 outColor;
    
    void main() { outColor = fragColor; }
  )gl"}; // O atributo de entrada (fragColor) é copiado sem modificações para o atributo de saída (outColor).

  // Create shader program
  m_program = createProgramFromString(vertexShader, fragmentShader);

  // Clear window
  abcg::glClearColor(0, 0, 0, 1);
  abcg::glClear(GL_COLOR_BUFFER_BIT);

  // Start pseudo-random number generator
  auto seed{std::chrono::steady_clock::now().time_since_epoch().count()};
  m_randomEngine.seed(seed);

  //Habilitar modo de mistura de cores
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 
}


void OpenGLWindow::paintGL() {
  if(desenhar && cont == 0)
    setupModel();

  abcg::glViewport(0, 0, m_viewportWidth, m_viewportHeight);  // Set the viewport

  abcg::glUseProgram(m_program);  // Start using the shader program
  abcg::glBindVertexArray(m_vao);  // Start using VAO

  abcg::glDrawArrays(GL_TRIANGLES, 0, 3);  // A função de renderização, glDrawArrays, dessa vez usa GL_TRIANGLES e 3 vértices, sendo que o índice inicial dos vértices no arranjo é 0. Isso significa que o pipeline desenhará apenas um triângulo.

  abcg::glBindVertexArray(0);  // End using VAO
  abcg::glUseProgram(0);  // End using the shader program

  printf("cont %d delay %d\n", cont, delay);
  if(cont / delay >= 600)
   cont = 0;
  else cont++;
}

void OpenGLWindow::paintUI() {
  abcg::OpenGLWindow::paintUI();

  {
    auto widgetSize{ImVec2(250, 200)};
    ImGui::SetNextWindowPos(ImVec2(m_viewportWidth - widgetSize.x - 5
                                  ,m_viewportHeight - widgetSize.y - 5));
    ImGui::SetNextWindowSize(widgetSize); //definem a posição e tamanho da janela da ImGui que está prestes a ser criada
    auto windowFlags{ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar}; // flags para que ela não possa ser redimensionada e não tenha a barra de título
    ImGui::Begin(" ", nullptr, windowFlags);

    //caixa de opções
    ImGui::Checkbox("Play", &desenhar);
    ImGui::Checkbox("Cores sólidas", &flat_colors);
    ImGui::Checkbox("Cores aleatórias", &random_colors);

    //Edit vertex colors
    auto colorEditFlags{ImGuiColorEditFlags_NoTooltip | //Os controles ImGui::ColorEdit3 também são criados com flags para desabilitar o color picker (ImGuiColorEditFlags_NoPicker) e os tooltips (ImGuiColorEditFlags_NoTooltip), pois eles podem atrapalhar o desenho dos triângulos.
                        ImGuiColorEditFlags_NoPicker};
    ImGui::PushItemWidth(215);
    ImGui::ColorEdit3("v0", &m_vertexColors[0].x, colorEditFlags);
    ImGui::ColorEdit3("v1", &m_vertexColors[1].x, colorEditFlags);
    ImGui::ColorEdit3("v2", &m_vertexColors[2].x, colorEditFlags);
    ImGui::PopItemWidth();

    ImGui::SliderInt("Delay", &delay, 1, 600, "%d", ImGuiSliderFlags_None);

    ImGui::End();
  }
}

void OpenGLWindow::resizeGL(int width, int height) {
  m_viewportWidth = width;
  m_viewportHeight = height;
  abcg::glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLWindow::terminateGL() {
  // Release shader program, VBO and VAO
  abcg::glDeleteProgram(m_program);
  abcg::glDeleteBuffers(1, &m_vboPositions);
  abcg::glDeleteBuffers(1, &m_vboColors);
  abcg::glDeleteVertexArrays(1, &m_vao);
}

void OpenGLWindow::setupModel() {
  // Release previous VBO and VAO
  abcg::glDeleteBuffers(1, &m_vboPositions);
  abcg::glDeleteBuffers(1, &m_vboColors);
  abcg::glDeleteVertexArrays(1, &m_vao);
  //É importante fazer isso, pois a função setupModel é chamada continuamente em paintGL.
  //Create vertex positions
  std::uniform_real_distribution<float> rd(-1.5f, 1.5f); //Observe que as coordenadas das posições dos vértices são números pseudoaleatórios do intervalo  [−1.5,1.5] . Vimos no projeto anterior que, para uma primitiva ser vista no viewport, ela precisa ser especificada entre  [−1,−1]  e  [1,1] . Logo, nossos triângulos terão partes que ficarão para fora da janela. 
  std::array<glm::vec2, 3> positions{glm::vec2(rd(m_randomEngine), rd(m_randomEngine)), //ALTEREI AQUI, DIFERENTE DO PROFESSOR
                                    glm::vec2(rd(m_randomEngine), rd(m_randomEngine)),
                                    glm::vec2(rd(m_randomEngine), rd(m_randomEngine))};
                          
  //Cores aleatórias
  if(random_colors){
    std::uniform_real_distribution<float> rdc(0.0f, 1.0f);
    m_vertexColors[0] = glm::vec4(rdc(m_randomEngine), rdc(m_randomEngine), rdc(m_randomEngine), 0.8f);
    m_vertexColors[1] = glm::vec4(rdc(m_randomEngine), rdc(m_randomEngine), rdc(m_randomEngine), 0.8f);
    m_vertexColors[2] = glm::vec4(rdc(m_randomEngine), rdc(m_randomEngine), rdc(m_randomEngine), 0.8f);
  }
  
  //Cores sólidas
  if(flat_colors){
    m_vertexColors[0] = m_vertexColors[1] = m_vertexColors[2];
  }

  // Create vertex colors
  std::vector<glm::vec4> colors(0);
  colors.emplace_back(m_vertexColors[0]);
  colors.emplace_back(m_vertexColors[1]);
  colors.emplace_back(m_vertexColors[2]);

  // Generate a new VBO of positions and get the associated ID
  abcg::glGenBuffers(1, &m_vboPositions);
  // Bind VBO in order to use it
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_vboPositions);
  // Upload data to VBO
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions.data(), GL_STATIC_DRAW);
  // Unbinding the VBO is allowed (data can be released now)
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // cria o identificador de um objeto de buffer (buffer object). Um objeto de buffer é um arranjo de dados alocado pelo OpenGL, geralmente na memória da GPU.
  abcg::glGenBuffers(1, &m_vboColors);
  // com o argumento GL_ARRAY_BUFFER vincula o objeto de buffer a um buffer de atributos de vértices. Isso define o objeto de buffer como um objeto de buffer de vértice (VBO). 
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_vboColors);
  // aloca a memória e inicializa o buffer com o conteúdo copiado de um ponteiro alocado na CPU. O primeiro parâmetro indica o tipo de objeto de buffer utilizado. O segundo parâmetro é o tamanho do buffer em bytes. O terceiro parâmetro é um ponteiro para os dados que serão copiados, na quantidade de bytes correspondente ao tamanho do buffer. O quarto parâmetro é uma “dica” ao driver de vídeo de como o buffer será usado. GL_STATIC_DRAW significa que o buffer será modificado apenas uma vez, potencialmente será utilizado muitas vezes, e que os dados serão usados para renderizar algo no framebuffer.
  abcg::glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STATIC_DRAW);
  // O objeto de buffer pode ser desvinculado com glBindBuffer(0), ou vinculando outro objeto de buffer.
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Get location of attributes in the program
  GLint positionAttribute{abcg::glGetAttribLocation(m_program, "inPosition")};
  GLint colorAttribute{abcg::glGetAttribLocation(m_program, "inColor")};

  // Create VAO
  abcg::glGenVertexArrays(1, &m_vao);

  // Bind vertex attributes to current VAO
  abcg::glBindVertexArray(m_vao);

  abcg::glEnableVertexAttribArray(positionAttribute);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_vboPositions);
  abcg::glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  abcg::glEnableVertexAttribArray(colorAttribute);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_vboColors);
  abcg::glVertexAttribPointer(colorAttribute, 4, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // End of binding to current VAO
  abcg::glBindVertexArray(0);
}