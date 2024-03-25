find_package(Vulkan REQUIRED)

add_library(katengine STATIC src/kat/vmaimpl.cpp
        src/kat/app.cpp
        src/kat/app.hpp
        src/kat/graphics/context.cpp
        src/kat/graphics/context.hpp
        src/kat/graphics/graphics_pipeline.cpp
        src/kat/graphics/graphics_pipeline.hpp
        src/kat/graphics/render_pass.cpp
        src/kat/graphics/render_pass.hpp
        src/kat/graphics/shader_cache.cpp
        src/kat/graphics/shader_cache.hpp
        src/kat/graphics/window.cpp
        src/kat/graphics/window.hpp)

target_include_directories(katengine PUBLIC src/)
target_link_libraries(katengine PUBLIC glfw Vulkan::Vulkan glm::glm vk-bootstrap::vk-bootstrap GPUOpen::VulkanMemoryAllocator)
target_compile_definitions(katengine PUBLIC -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_DEFAULT_ALIGNED_GENTYPES -DGLM_ENABLE_EXPERIMENTAL)

add_library(katengine::katengine ALIAS katengine)