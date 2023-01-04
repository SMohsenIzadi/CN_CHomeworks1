cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

configure_file(config.json config.json COPYONLY)
configure_file(man man COPYONLY)

project(sFTP VERSION 1.0.0)
add_executable( sFTP 
                main.cxx
)

add_subdirectory(core)

target_include_directories(sFTP PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/core"
                          "${PROJECT_SOURCE_DIR}/dependencies"
                          )

target_link_libraries(sFTP PUBLIC sFTP_Core -luuid)