#version 450

layout(binding = 1) uniform sampler2D fontTexture;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUv;
layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(fontTexture, fragUv);
    outColor = fragColor * texColor;
}