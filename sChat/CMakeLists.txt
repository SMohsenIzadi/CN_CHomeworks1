cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


project(sChat VERSION 1.0.0)
add_executable( sChat 
                main.cxx
)

add_subdirectory(core)

target_include_directories(sChat PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/core"
                          )

target_link_libraries(sChat PUBLIC sChat_core)