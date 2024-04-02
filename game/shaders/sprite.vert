#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoords;

layout(location = 0) out vec2 fragTexCoords;

layout(push_constant) uniform constants {
    mat4 model;
} pc;

layout(binding = 0) uniform uniform_buffer {
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    gl_Position = ubo.projection * ubo.view * pc.model * vec4(inPosition, 0.0, 1.0);
    fragTexCoords = inTexCoords;
}
