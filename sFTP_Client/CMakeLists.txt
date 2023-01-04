cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(sFTP_Client VERSION 1.0.0)
add_executable( sFTP_Client 
                main.cxx
)

add_subdirectory(core)

target_include_directories(sFTP_Client PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/core"
                          "${PROJECT_SOURCE_DIR}/dependencies"
                          )

target_link_libraries(sFTP_Client PUBLIC sFTP_Client_Core)