cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

configure_file(index.html index.html COPYONLY)
configure_file(404.html 404.html COPYONLY)
configure_file(test.pdf test.pdf COPYONLY)
configure_file(test.gif test.gif COPYONLY)
configure_file(test.mp3 test.mp3 COPYONLY)
configure_file(test.jpeg test.jpeg COPYONLY)
configure_file(test.js test.js COPYONLY)
configure_file(test.css test.css COPYONLY)

project(wServer VERSION 1.0.0)
add_executable( wServer 
                main.cxx
)

add_subdirectory(core)

target_include_directories(wServer PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/core"
                          )

target_link_libraries(wServer PUBLIC wServer_core)