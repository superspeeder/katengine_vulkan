#pragma once

#include <array>
#include <optional>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace kat {
    struct fatal_exc {};

    constexpr fatal_exc fatal{};

    template <typename T>
    const T *ptr_to_optional(const std::optional<T> &opt) {
        return opt ? std::to_address(opt) : nullptr;
    };

    template <typename T>
    T *ptr_to_optional(std::optional<T> &opt) {
        return opt ? std::to_address(opt) : nullptr;
    };

    struct color {
        float r, g, b, a;

        [[nodiscard]] inline vk::ClearValue to_clear_value() const { return vk::ClearColorValue(std::array<float, 4>{r, g, b, a}); };

        constexpr operator glm::vec4() const { return {r, g, b, a}; };

        constexpr operator glm::vec3() const { return {r, g, b}; };

        color &operator=(const color &other) = default;

        color &operator=(const glm::vec4 &other) {
            r = other.r;
            g = other.g;
            b = other.b;
            a = other.a;
            return *this;
        };

        color &operator=(const glm::vec3 &other) {
            r = other.r;
            g = other.g;
            b = other.b;
            a = 1.0f;
            return *this;
        };

        constexpr color(const glm::vec4 &other) : r(other.r), g(other.g), b(other.b), a(other.a){};
        constexpr color(const glm::vec3 &other) : r(other.r), g(other.g), b(other.b), a(1.0f){};

        constexpr color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_){};
    };

    constexpr color BLACK{0., 0., 0.};
    constexpr color WHITE{1., 1., 1.};
    constexpr color RED{1., 0., 0.};
    constexpr color GREEN{0., 1., 0.};
    constexpr color BLUE{0., 0., 1.};
    constexpr color YELLOW{1., 1., 0.};
    constexpr color CYAN{0., 1., 1.};
    constexpr color MAGENTA{1., 0., 1.};

    constexpr vk::ColorComponentFlags COLOR_COMPONENT_FLAGS_RGBA =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    constexpr vk::ColorComponentFlags COLOR_COMPONENT_FLAGS_RGB = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;

    constexpr vk::PipelineColorBlendAttachmentState STANDARD_BLEND_STATE = vk::PipelineColorBlendAttachmentState{
        true,
        vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        kat::COLOR_COMPONENT_FLAGS_RGBA,
    };

} // namespace kat
