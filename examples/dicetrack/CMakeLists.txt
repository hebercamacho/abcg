project(dicetrack)
add_executable(${PROJECT_NAME} main.cpp dices.cpp openglwindow.cpp
                               trackball.cpp)
enable_abcg(${PROJECT_NAME})
