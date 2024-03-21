cmake_minimum_required(VERSION 3.28)
project(gaming2)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)

add_subdirectory(libs)


file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS src/*.c src/*.cpp src/*.h src/*.hpp)

add_executable(gaming2 ${GAME_SOURCES})
target_include_directories(gaming2 PRIVATE src/)
target_link_libraries(gaming2 glfw Vulkan::Vulkan glm::glm vk-bootstrap::vk-bootstrap)


find_package(Python REQUIRED)

add_custom_target(compile_shaders ALL Python::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/shader_compile.py ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders ${Vulkan_GLSLC_EXECUTABLE})

add_dependencies(gaming2 compile_shaders)