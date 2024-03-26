find_package(Python REQUIRED)

add_executable(game src/game/game.cpp src/game/game.hpp src/game/main.cpp)
target_include_directories(game PRIVATE src/)
target_link_libraries(game katengine::katengine)

add_custom_target(compile_shaders ALL Python::Interpreter ${CMAKE_SOURCE_DIR}/shader_compile.py ${CMAKE_CURRENT_LIST_DIR}/shaders ${CMAKE_CURRENT_LIST_DIR}/resources/shaders ${Vulkan_GLSLC_EXECUTABLE})

add_dependencies(game compile_shaders)
