cmake_minimum_required(VERSION 3.16)

project(
  ai_operator_acceleration
  VERSION 0.1.0
  DESCRIPTION "operator optimization"
  LANGUAGES CXX
)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

include(CTest)

add_subdirectory(src)
