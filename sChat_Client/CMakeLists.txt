cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


project(sChat_Client VERSION 1.0.0)
add_executable( sChat_Client 
                main.cxx
)

add_subdirectory(core)

target_include_directories(sChat_Client PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/core"
                          )

target_link_libraries(sChat_Client PUBLIC sChat_Client_core)                          