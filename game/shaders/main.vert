#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoords;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragPos;
layout(location = 3) out vec2 fragTexCoords;

layout(push_constant) uniform constants {
    mat4 model;
} push_constants;

layout(binding = 0) uniform uniform_buffer {
    mat4 viewProjection;
    vec4 ambientColor;
    vec3 lightPos; 
    float ambientStrength;
    vec4 diffuseColor;
    vec3 viewPos;
    float specularStrength;
} ubo;


void main() {
    fragPos = vec4(inPosition, 1.0);
    gl_Position = ubo.viewProjection * push_constants.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragNormal = inNormal;
    fragTexCoords = inTexCoords;
}