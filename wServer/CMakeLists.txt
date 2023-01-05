cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

configure_file(index.html index.html COPYONLY)
configure_file(404.html 404.html COPYONLY)

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