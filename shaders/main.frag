#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragPos;

layout(location = 0) out vec4 outColor;


layout(binding = 0) uniform uniform_buffer {
    mat4 viewProjection;

    vec4 ambientColor;
    vec4 lightColor;

    vec4 lightPos; 
    vec4 viewPos;

    float ambientStrength;
    float specularStrength;
} ubo;

void main() {
    vec4 objectColor = vec4(0.2, 0.2, 0.2, 1.0);

    vec3 ambient = ubo.ambientStrength * ubo.ambientColor.rgb;

    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(ubo.lightPos.xyz - fragPos.xyz);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor.rgb;

    vec3 viewDir = normalize(ubo.viewPos.xyz - fragPos.xyz);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = ubo.specularStrength * spec * ubo.lightColor.rgb;

    outColor = vec4(ambient + diffuse + specular, 1.0) * objectColor;
    // outColor = vec4(min(max(diff, 0.0), 1.0));
    // outColor.rgb = vec3(diff);
    // outColor.b = 1.0;
    // outColor.rgb = fragPos.xyz;
    // outColor.rgb = 1.0 - vec3(ubo.ambientStrength);
    // outColor.a = 1.0;
}