#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform uniform_buffer {
    mat4 view;
    mat4 projection;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 color = texture(texSampler, fragTexCoords);

    outColor = color * fragColor;
}