#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragWorldPos;

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec4 lightDir;
    vec4 viewPos;
} scene;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-scene.lightDir.xyz);
    vec3 V = normalize(scene.viewPos.xyz - fragWorldPos);
    vec3 H = normalize(L + V);

    vec4 texColor = texture(texSampler, fragUV);

    float ambient = 0.28;
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.25;

    vec3 color = texColor.rgb * (ambient + diff) + vec3(spec);
    outColor = vec4(color, texColor.a);
}
